/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "InfluxDataLogger.hh"

#include <openssl/sha.h>

#include <chrono>
#include <cmath>
#include <future>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string_view>

#include "karabo/data/time/TimeDuration.hh"
#include "karabo/net/InfluxDbClient.hh"
#include "karabo/util/DataLogUtils.hh"

#define SECONDS_PER_YEAR 365 * 24 * 60 * 60

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device, karabo::devices::DataLogger,
                                  karabo::devices::InfluxDataLogger)
KARABO_REGISTER_IN_FACTORY_1(karabo::devices::DeviceData, karabo::devices::InfluxDeviceData, karabo::data::Hash)

namespace karabo {
    namespace devices {

        namespace nl = nlohmann;
        using namespace karabo::io;
        using namespace karabo::net;
        using namespace karabo::data;
        using namespace karabo::util;
        using namespace std::placeholders;


        const unsigned int InfluxDataLogger::k_httpResponseTimeoutMs = 1500u;

        InfluxDeviceData::InfluxDeviceData(const karabo::data::Hash& input)
            : DeviceData(input),
              m_dbClientRead(input.get<karabo::net::InfluxDbClient::Pointer>("dbClientReadPointer")),
              m_dbClientWrite(input.get<karabo::net::InfluxDbClient::Pointer>("dbClientWritePointer")),
              m_serializer(karabo::io::BinarySerializer<karabo::data::Hash>::create("Bin")),
              m_maxTimeAdvance(input.get<int>("maxTimeAdvance")),
              m_maxVectorSize(input.get<unsigned int>("maxVectorSize")),
              m_maxValueStringSize(input.get<unsigned int>("maxValueStringSize")),
              m_secsOfLogOfRejectedData(0ull),
              m_maxPropLogRateBytesSec(input.get<unsigned int>("maxPropLogRateBytesSec")),
              m_propLogRatePeriod(input.get<unsigned int>("propLogRatePeriod")),
              m_maxSchemaLogRateBytesSec(input.get<unsigned int>("maxSchemaLogRateBytesSec")),
              m_schemaLogRatePeriod(input.get<unsigned int>("schemaLogRatePeriod")),
              m_loggingStartStamp(Epochstamp(0ull, 0ull), 0ull),
              m_safeSchemaRetentionDuration(
                    TimeDuration{static_cast<karabo::data::TimeValue>(
                                       std::round(input.get<double>("safeSchemaRetentionPeriod") * SECONDS_PER_YEAR)),
                                 0ull}) {}


        InfluxDeviceData::~InfluxDeviceData() {}


        void InfluxDeviceData::stopLogging() {
            if (m_initLevel != InitLevel::COMPLETE) {
                // We have not yet started logging this device, so nothing to mark about being done.
                return;
            }

            const std::string& deviceId = m_deviceToBeLogged;
            std::stringstream ss;
            {
                // Timestamp shall be the one of the most recent update - this ensures that all stamps come from
                // the device and cannot be screwed up if clocks of logger and device are off from each other.
                // But we store the local time of the logger as well.
                std::lock_guard<std::mutex> lock(m_lastTimestampMutex);
                const unsigned long long ts = m_lastDataTimestamp.toTimestamp() * INFLUX_PRECISION_FACTOR;
                ss << deviceId << "__EVENTS,type=\"-LOG\" karabo_user=\"" << m_user << "\",logger_time=\""
                   << Epochstamp().toIso8601Ext() << "\" " << ts << "\n";
            }
            m_dbClientWrite->enqueueQuery(ss.str());
            m_dbClientWrite->flushBatch();

            KARABO_LOG_FRAMEWORK_INFO << "Proxy for \"" << deviceId << "\" is destroyed ...";
        }


        unsigned int InfluxDeviceData::newPropLogRate(const std::string& propPath, Epochstamp currentStamp,
                                                      std::size_t currentSize) {
            Epochstamp now;
            if (currentStamp - now > 120.0) { // Epochstamp "-" operator returns the interval length (always positive).
                // This assumes that the backend can stand some seconds, the maximum interval in the test above, of
                // a too high rate. If the difference goes beyond that maximum tolerance, the current system time is
                // used.
                currentStamp = now;
            }

            // Advances the log rating window using the current timestamp reference
            std::deque<LoggingRecord>& propLogRecs = m_propLogRecs[propPath];
            const TimeDuration ratingWinDuration{m_propLogRatePeriod, 0ull};
            while (!propLogRecs.empty() && (currentStamp - propLogRecs.back().epoch >= ratingWinDuration)) {
                propLogRecs.pop_back();
            }

            std::size_t bytesWritten = currentSize;
            for (const LoggingRecord& rec : m_propLogRecs[propPath]) {
                bytesWritten += rec.sizeChars;
            }

            unsigned int newRate = bytesWritten / m_propLogRatePeriod;
            if (newRate <= m_maxPropLogRateBytesSec) {
                // There's room for logging the data; keep track of the saving.
                propLogRecs.push_front(LoggingRecord(currentSize, currentStamp));
            }

            return newRate;
        }


        unsigned int InfluxDeviceData::newSchemaLogRate(std::size_t schemaSize) {
            // Advances the log rating window using the current timing reference
            Epochstamp now;
            const TimeDuration ratingWinDuration{m_schemaLogRatePeriod, 0ull};
            while (!m_schemaLogRecs.empty() && (now - m_schemaLogRecs.back().epoch >= ratingWinDuration)) {
                m_schemaLogRecs.pop_back();
            }

            std::size_t bytesWritten = schemaSize;
            for (const LoggingRecord& rec : m_schemaLogRecs) {
                bytesWritten += rec.sizeChars;
            }

            unsigned int newRate = bytesWritten / m_schemaLogRatePeriod;
            if (newRate <= m_maxSchemaLogRateBytesSec) {
                // There's room for logging the data; keep track of the saving.
                m_schemaLogRecs.push_front(LoggingRecord(schemaSize, now));
            }

            return newRate;
        }


        void InfluxDeviceData::handleChanged(const karabo::data::Hash& configuration, const std::string& user) {
            m_dbClientWrite->startDbConnectIfDisconnected();

            if (user.empty()) {
                m_user = ".";
            } else {
                m_user = user; // set under m_strand protection
            }
            const std::string& deviceId = m_deviceToBeLogged;

            // store the local unix timestamp to compare the time difference w.r.t. incoming data.
            Epochstamp nowish;
            std::vector<RejectedData> rejectedPaths; // path and reason
            // To write log I need schema - but that has arrived before connecting signal[State]Changed to
            // slotChanged and thus before any data can arrive here in handleChanged.
            std::vector<std::string> paths;
            getPathsForConfiguration(configuration, m_currentSchema, paths);
            std::stringstream query;
            Timestamp lineTimestamp(Epochstamp(0ull, 0ull), Trainstamp(0ull));

            for (size_t i = 0; i < paths.size(); ++i) {
                const std::string& path = paths[i];

                // Skip those elements which should not be archived
                const bool noArchive = (!m_currentSchema.has(path) ||
                                        (m_currentSchema.hasArchivePolicy(path) &&
                                         (m_currentSchema.getArchivePolicy(path) == Schema::NO_ARCHIVING)));

                const Hash::Node& leafNode = configuration.getNode(path);

                // Check for timestamp ...
                if (!Timestamp::hashAttributesContainTimeInformation(leafNode.getAttributes())) {
                    if (!noArchive) { // Lack of timestamp for non-archived properties does not harm logging
                        KARABO_LOG_FRAMEWORK_WARN << "Skip '" << path << "' of '" << deviceId
                                                  << "' - it lacks time information attributes.";
                    }
                    continue;
                }

                if (m_pendingLogin) {
                    login(configuration, paths);
                    m_pendingLogin = false;
                }

                Timestamp t = Timestamp::fromHashAttributes(leafNode.getAttributes());
                if (t.getEpochstamp() < m_loggingStartStamp.getEpochstamp()) {
                    // Stamp is older than logging start time. To avoid confusion, especially for properties with no
                    // default value i.e. which may not exist at some points in time) we overwrite the stamp with
                    // time when device logging started. See discussions at
                    // https://git.xfel.eu/Karabo/config-and-recovery/-/issues/20 and
                    // https://in.xfel.eu/redmine/projects/karabo-library/wiki/InfluxNoDefaultValue
                    t = m_loggingStartStamp;
                }
                {
                    // Update time stamp for updates of property "lastUpdatesUtc"
                    // Since the former is accessing it when not posted on m_strand, need mutex protection:
                    std::lock_guard<std::mutex> lock(m_lastTimestampMutex);
                    if (t.getEpochstamp() != m_lastDataTimestamp.getEpochstamp()) {
                        // If mixed timestamps in single message (or arrival in wrong order), always take last
                        // received.
                        m_updatedLastTimestamp = true;
                        m_lastDataTimestamp = t;
                    }
                }

                if (noArchive) continue; // Bail out after updating time stamp!

                // no check needed if the maxTimeDifference is negative or 0
                if (m_maxTimeAdvance > 0 && t.getEpochstamp() > nowish) {
                    // substract the 2 Epochstamp to get a TimeDuration.
                    const double dt = t.getEpochstamp() - nowish;
                    if (dt > m_maxTimeAdvance) {
                        rejectedPaths.push_back(
                              RejectedData{RejectionType::FAR_AHEAD_TIME, path, "from far future " + t.toIso8601Ext()});
                        // timestamp seems unreliable, so we bail out before
                        continue;
                    }
                }
                std::string value;    // "value" should be a string, so convert depending on type ...
                bool isFinite = true; // false for nan and inf DOUBLE/FLOAT
                Types::ReferenceType type = leafNode.getType();
                size_t vectorSize = 0;
                if (type == Types::VECTOR_HASH) {
                    // Represent any vector<Hash> as Base64 string
                    std::vector<char> archive;
                    vectorSize = leafNode.getValue<std::vector<Hash>>().size() * 10u; // scale up vector hash size!
                    m_serializer->save(leafNode.getValue<std::vector<Hash>>(), archive);
                    const unsigned char* uarchive = reinterpret_cast<const unsigned char*>(archive.data());
                    value = base64Encode(uarchive, archive.size());
                } else if (leafNode.getType() == Types::CHAR) {
                    const unsigned char* uarchive = reinterpret_cast<const unsigned char*>(&leafNode.getValue<char>());
                    value = base64Encode(uarchive, 1ul);
                } else if (leafNode.getType() == Types::VECTOR_CHAR) {
                    const std::vector<char>& v = leafNode.getValue<std::vector<char>>();
                    vectorSize = v.size();
                    const unsigned char* uarchive = reinterpret_cast<const unsigned char*>(v.data());
                    value = base64Encode(uarchive, v.size());
                } else if (type == Types::VECTOR_UINT8) {
                    // The generic vector code below uses toString(vector<unsigned char>) which
                    // erroneously uses base64 encoding.
                    // We do not dare to fix that now, but workaround it here to have a human readable string
                    // in the DB to ease the use of the data outside Karabo:
                    const std::vector<unsigned char>& vec = leafNode.getValue<std::vector<unsigned char>>();
                    vectorSize = vec.size();
                    if (!vec.empty()) {
                        std::ostringstream s;
                        s << static_cast<unsigned int>(vec[0]);
                        for (size_t i = 1ul; i < vec.size(); ++i) {
                            s << "," << static_cast<unsigned int>(vec[i]);
                        }
                        value = s.str();
                    }
                } else if (type == Types::VECTOR_STRING) {
                    // Special case: convert to JSON and then base64 ...
                    const std::vector<std::string>& vecstr = leafNode.getValue<std::vector<std::string>>();
                    vectorSize = vecstr.size();       // or use overall base64 encoded length below?
                    nl::json j(vecstr);               // convert to JSON
                    const std::string str = j.dump(); // JSON as a string
                    const unsigned char* encoded = reinterpret_cast<const unsigned char*>(str.c_str());
                    const size_t length = str.length();
                    value = base64Encode(encoded, length); // encode to base64
                } else if (Types::isVector(type)) {
                    // ... and treat  any other vectors as a comma separated text string of vector elements
                    const std::vector<std::string>& asVecStr = leafNode.getValueAs<std::string, std::vector>();
                    vectorSize = asVecStr.size();
                    value = toString(asVecStr);
                } else if (type == Types::DOUBLE) {
                    const double v = leafNode.getValue<double>();
                    isFinite = std::isfinite(v);
                    value = toString(v);
                } else if (type == Types::FLOAT) {
                    // bail out if Nan or Infinite
                    const float v = leafNode.getValue<float>();
                    isFinite = std::isfinite(v);
                    value = toString(v);
                } else if (type == Types::UINT64) {
                    const unsigned long long uv = leafNode.getValue<unsigned long long>();
                    // behavior on simple casting is implementation defined. We memcpy instead to be sure of the
                    // results
                    long long sv;
                    memcpy(&sv, &uv, sizeof(long long));
                    value = toString(sv);
                } else if (type == Types::STRING) {
                    value = leafNode.getValueAs<std::string>();
                    // Line breaks violate the line protocol, so we mangle newlines... :-(.
                    boost::algorithm::replace_all(value, "\n", karabo::util::DATALOG_NEWLINE_MANGLE);
                } else {
                    value = leafNode.getValueAs<std::string>();
                }

                if (lineTimestamp.getEpochstamp().getSeconds() == 0ull) {
                    // first non-skipped value
                    lineTimestamp = t;
                } else if (t.getEpochstamp() != lineTimestamp.getEpochstamp()) {
                    // new timestamp! flush the previous query
                    terminateQuery(query, lineTimestamp, rejectedPaths);
                    lineTimestamp = t;
                }

                if (vectorSize > m_maxVectorSize) {
                    std::ostringstream oss;
                    if (type == Types::VECTOR_HASH) {
                        oss << "table of " << toString(vectorSize / 10u) << " rows";
                    } else {
                        oss << "vector of size " << toString(vectorSize);
                    }
                    rejectedPaths.push_back(RejectedData{RejectionType::TOO_MANY_ELEMENTS, path, oss.str()});
                    // All stamp manipulations done, we just skip logValue
                    continue;
                }

                if (value.size() > m_maxValueStringSize) {
                    rejectedPaths.push_back(RejectedData{RejectionType::VALUE_STRING_SIZE, path,
                                                         std::string{"Metric value length, "} + toString(value.size()) +
                                                               ", exceeds the maximum length allowed in Influx, " +
                                                               toString(m_maxValueStringSize)});
                    continue;
                }

                const Epochstamp& currentStamp = lineTimestamp.getEpochstamp();
                const unsigned int newRate = newPropLogRate(path, currentStamp, value.size());
                if (newRate <= m_maxPropLogRateBytesSec) {
                    logValue(query, deviceId, path, value, leafNode.getType(),
                             isFinite); // isFinite matters only for FLOAT/DOUBLE
                } else {
                    rejectedPaths.push_back(RejectedData{
                          RejectionType::PROPERTY_WRITE_RATE, deviceId,
                          "Update of property '" + path + "' timestamped at '" + currentStamp.toIso8601Ext() +
                                "' would reach a logging rate of '" + toString(newRate / 1024) + " Kb/sec'."});
                }
            }
            terminateQuery(query, lineTimestamp, rejectedPaths);
        }


        void InfluxDeviceData::login(const karabo::data::Hash& configuration,
                                     const std::vector<std::string>& sortedPaths) {
            // TRICK: 'configuration' is the one requested at the beginning. For devices which have
            // properties with older timestamps than the time of their instantiation (as e.g. read from
            // hardware), we can claim that logging is active only from the most recent update we receive here.
            const auto& attrsOfPathWithMostRecentStamp = configuration.getAttributes(sortedPaths.back());
            m_loggingStartStamp = Timestamp::fromHashAttributes(attrsOfPathWithMostRecentStamp);
            const unsigned long long ts = m_loggingStartStamp.toTimestamp() * INFLUX_PRECISION_FACTOR;
            std::stringstream ss;
            ss << m_deviceToBeLogged << "__EVENTS,type=\"+LOG\" karabo_user=\"" << m_user << "\",logger_time=\""
               << Epochstamp().toIso8601Ext() << "\",format=1i";  // Older data (where timestamps were not ensured to
                                                                  // be not older than 'ts') has no format specified.
            auto deviceIdNode = configuration.find("_deviceId_"); // _deviceId_ as in DataLogger::slotChanged
            if (deviceIdNode) {                                   // Should always exist in case of m_pendingLogin
                const Epochstamp devStartStamp = Epochstamp::fromHashAttributes(deviceIdNode->getAttributes());
                // Difference between when device instantiated and when logging starts - precision as defined by
                // PRECISION_FACTOR
                const long long diff = static_cast<double>(m_loggingStartStamp.getEpochstamp() - devStartStamp) *
                                       INFLUX_PRECISION_FACTOR;
                ss << ",deviceAge=" << diff << "i";
            } else { // Should never happen!
                KARABO_LOG_FRAMEWORK_WARN << "Cannot store device age of '" << m_deviceToBeLogged
                                          << "', device lacks key '_deviceId_'.";
            }
            ss << " " << ts << "\n";
            m_dbClientWrite->enqueueQuery(ss.str());
        }


        void InfluxDeviceData::logValue(std::stringstream& query, const std::string& deviceId, const std::string& path,
                                        const std::string& value, karabo::data::Types::ReferenceType type,
                                        bool isFinite) {
            std::string field_value;
            switch (type) {
                case Types::BOOL: {
                    if (value.empty()) {
                        // Should never happen! We try to save the line protocol by skipping
                        KARABO_LOG_FRAMEWORK_ERROR << "Empty value for property '" << path << "' on device '"
                                                   << deviceId << "'";
                        return;
                    }
                    field_value = path + "-BOOL=" + ((value == "1") ? "t" : "f");
                    break;
                }
                case Types::INT8:
                case Types::UINT8:
                case Types::INT16:
                case Types::UINT16:
                case Types::INT32:
                case Types::UINT32:
                case Types::INT64:
                case Types::UINT64: {
                    if (value.empty()) {
                        // Should never happen! We try to save the line protocol by skipping
                        KARABO_LOG_FRAMEWORK_ERROR << "Empty value for property '" << path << "' on device '"
                                                   << deviceId << "'";
                        return;
                    }
                    field_value = path + "-" + Types::to<ToLiteral>(type) + "=" + value + "i";
                    break;
                }
                case Types::FLOAT:
                case Types::DOUBLE: {
                    if (value.empty()) {
                        // Should never happen! We try to save the line protocol by skipping
                        KARABO_LOG_FRAMEWORK_ERROR << "Empty value for property '" << path << "' on device '"
                                                   << deviceId << "'";
                        return;
                    }
                    field_value = path + "-" + Types::to<ToLiteral>(type);
                    if (!isFinite) {
                        // InfluxDB does not support nan and inf - so we store them as strings as another field
                        // whose name is extended by "_INF":
                        ((field_value += "_INF=\"") += value) += "\"";
                    } else {
                        (field_value += "=") += value;
                    }
                    break;
                }
                case Types::BYTE_ARRAY:
                case Types::COMPLEX_FLOAT:
                case Types::COMPLEX_DOUBLE:
                case Types::VECTOR_BOOL:
                case Types::VECTOR_INT8:
                case Types::VECTOR_UINT8:
                case Types::VECTOR_INT16:
                case Types::VECTOR_UINT16:
                case Types::VECTOR_INT32:
                case Types::VECTOR_UINT32:
                case Types::VECTOR_INT64:
                case Types::VECTOR_UINT64:
                case Types::VECTOR_FLOAT:
                case Types::VECTOR_DOUBLE:
                case Types::VECTOR_COMPLEX_FLOAT:
                case Types::VECTOR_COMPLEX_DOUBLE: {
                    // empty strings shall be saved. They do not spoil the line protocol since they are between
                    // quotes
                    field_value = path + "-" + Types::to<ToLiteral>(type) + "=\"" + value + "\"";
                    break;
                }
                case Types::VECTOR_CHAR:
                case Types::VECTOR_HASH:
                case Types::CHAR: {
                    if (value.empty()) {
                        // Should never happen! These types are base64 encoded
                        KARABO_LOG_FRAMEWORK_ERROR << "Empty value for property '" << path << "' on device '"
                                                   << deviceId << "'";
                        return;
                    }
                    field_value = path + "-" + Types::to<ToLiteral>(type) + "=\"" + value + "\"";
                    break;
                }
                case Types::STRING:
                case Types::VECTOR_STRING: {
                    std::string v = boost::replace_all_copy(value, "\\", "\\\\");
                    boost::replace_all(v, "\"", "\\\"");
                    field_value = path + "-" + Types::to<ToLiteral>(type) + "=\"" + v + "\"";
                    break;
                }
                default:
                    return;
            }

            if (query.str().empty()) {
                query << deviceId << ",karabo_user=\"" << m_user << "\" " << field_value;
            } else {
                query << "," << field_value;
            }
        }


        void InfluxDeviceData::terminateQuery(std::stringstream& query, const karabo::data::Timestamp& stamp,
                                              std::vector<RejectedData>& rejectedPathReasons) {
            unsigned long long ts = stamp.toTimestamp() * INFLUX_PRECISION_FACTOR;
            if (!query.str().empty()) {
                // There's data to be output to Influx.

                const unsigned long long tid = stamp.getTrainId();
                // influxDB integers are signed 64 bits. here we check that the we are within such limits
                // Assuming a trainId rate of 10 Hz this limit will be surpassed in about 29 billion years
                if (0 < tid && tid <= static_cast<unsigned long long>(std::numeric_limits<long long>::max())) {
                    query << ",_tid=" << tid << "i";
                }
                if (ts > 0) {
                    query << " " << ts;
                }
                query << "\n";
                m_dbClientWrite->enqueueQuery(query.str());
                query.str("");
            }

            logRejectedData(rejectedPathReasons, ts);
            rejectedPathReasons.clear();
        }


        void InfluxDeviceData::logRejectedDatum(const RejectedData& reject) {
            unsigned long long ts = karabo::data::Timestamp().toTimestamp() * INFLUX_PRECISION_FACTOR;
            const auto rejects = std::vector<RejectedData>({reject});
            logRejectedData(rejects, ts);
        }


        void InfluxDeviceData::logRejectedData(const std::vector<RejectedData>& rejects, unsigned long long ts) {
            if (rejects.empty()) return;

            std::stringstream textSs;
            textSs << "Skipping " << rejects.size() << " log metric(s) for device '" << m_deviceToBeLogged << "'";
            for (const RejectedData& reject : rejects) {
                textSs << " >> [" << static_cast<int>(reject.type) << "] '" << reject.dataPath << "' ("
                       << reject.details << ") ";
            }
            std::string text(textSs.str());
            const Epochstamp now;
            if (now.getSeconds() > 30ull + m_secsOfLogOfRejectedData) {
                // Blame device only every 30 seconds to avoid log file spamming
                KARABO_LOG_FRAMEWORK_WARN << text;
                m_secsOfLogOfRejectedData = now.getSeconds();
            }
            boost::algorithm::replace_all(text, "\n", " "); // better no line breaks
            std::stringstream badDataQuery;
            if (ts == 0.) {
                // Far future data without any "decent" data in same update Hash: setting 'stamp' was skipped and it
                // stays at the start of unix epoch. The best realistic stamp is in fact 'now':
                ts = now.toTimestamp() * INFLUX_PRECISION_FACTOR;
            }
            // Bad data is logged in a device independent measurement to simplify retrieval of all bad data.
            // DeviceId is the field name.
            // NOTES:
            //       1. There is a potential name clash of this measurement and a potential device with
            //          deviceId = "__BAD__DATA__".
            //       2. Since the rejected data is itself a string value, we truncate it to stay within the
            //          limit imposed by Influx.

            // TODO: use StringView when the Framework migrates to C++ 17 or later.
            const std::string& textToLog =
                  (text.size() <= m_maxValueStringSize ? text : text.substr(m_maxValueStringSize));
            badDataQuery << "__BAD__DATA__  " << m_deviceToBeLogged << "=\"" << textToLog << "\" " << ts << "\n";
            m_dbClientWrite->enqueueQuery(badDataQuery.str());
        }


        void InfluxDeviceData::handleSchemaUpdated(const karabo::data::Schema& schema,
                                                   const karabo::data::Timestamp& stamp) {
            // Before checking client status: enables buffering of property updates in handleChanged:
            m_currentSchema = schema;

            karabo::io::BinarySerializer<karabo::data::Schema>::Pointer serializer =
                  karabo::io::BinarySerializer<karabo::data::Schema>::create("Bin");
            auto archive(std::make_shared<std::vector<char>>());
            // Avoid re-allocations - small devices need around 10'000 bytes, DataLoggerManager almost 20'000
            archive->reserve(20'000);
            serializer->save(schema, *archive);
            const unsigned char* uarchive = reinterpret_cast<const unsigned char*>(archive->data());

            // Calculate 'digest' of serialized schema
            const std::size_t DIGEST_LENGTH = 20;
            unsigned char obuf[DIGEST_LENGTH];
            SHA1(uarchive, archive->size(), obuf);
            std::ostringstream dss;
            for (size_t i = 0; i < DIGEST_LENGTH; ++i) dss << std::hex << int(obuf[i]);
            std::string schDigest(dss.str());

            KARABO_LOG_FRAMEWORK_DEBUG << "Digest for schema of device '" << m_deviceToBeLogged << "': '" << schDigest
                                       << "'";

            // Only consider schemas with the same digest and within the safe retention time window.
            const Epochstamp safeRetentionLimit{Epochstamp() - m_safeSchemaRetentionDuration};
            std::ostringstream oss;
            oss << "SELECT COUNT(*) FROM \"" << m_deviceToBeLogged << "__SCHEMAS\" WHERE digest='\"" << schDigest
                << "\"' AND time >= " << epochAsMicrosecString(safeRetentionLimit)
                << toInfluxDurationUnit(TIME_UNITS::MICROSEC);
            m_dbClientRead->queryDb(
                  oss.str(), bind_weak(&InfluxDeviceData::onCheckSchemaInDb, this, stamp, schDigest, archive, _1));
        }

        void InfluxDeviceData::onCheckSchemaInDb(const karabo::data::Timestamp& stamp, const std::string& schDigest,
                                                 const std::shared_ptr<std::vector<char>>& schemaArchive,
                                                 const HttpResponse& o) {
            // Not running in Strand anymore - take care not to access any potentially changing data members!

            bool schemaInDb = false;
            if (o.code < 300) {
                // HTTP request with query to retrieve schema by digest succeeded.
                try {
                    nl::json j = nl::json::parse(o.payload);
                    auto count = j["results"][0]["series"][0]["values"][0][1];
                    if (!count.is_null()) {
                        // at least one schema with the digest has been found.
                        // When the digest isn't found, the response json is '{"results":[{"statement_id":0}]}'.
                        schemaInDb = true;

                        KARABO_LOG_FRAMEWORK_DEBUG << "Schema with digest '" << schDigest << "' for device '"
                                                   << m_deviceToBeLogged << "' already exists in Influx.";
                    }
                } catch (const nl::json::exception& je) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Error checking if schema with digest '" << schDigest
                                               << "' is already saved for device '" << m_deviceToBeLogged << "': '"
                                               << je.what() << "'.";
                }
            } else {
                KARABO_LOG_FRAMEWORK_ERROR << "Error checking if schema with digest '" << schDigest
                                           << "' is already saved for device '" << m_deviceToBeLogged << "': '" << o
                                           << "'.";
            }

            if (!schemaInDb) {
                // Schema not in db, or query request failed or query results could not be parsed.
                // In any of those cases, try to log the schema in the database.

                // Note: if the schema was already in the database, but the query request failed or returned an
                // unparseable response, requesting to save it again won't cause any harm appart from taking some
                // extra space. Not saving when in doubt would be the really harmful outcome, as that would lead to
                // failures in retrieving past device configurations.
                try {
                    schemaInDb = logNewSchema(schDigest, *schemaArchive);
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Error writing schema with digest '" << schDigest << "' for device '"
                                               << m_deviceToBeLogged << "': '" << e.what() << "'";
                }
            }

            if (schemaInDb) {
                const unsigned long long ts = stamp.toTimestamp() * INFLUX_PRECISION_FACTOR;
                std::stringstream ss;
                ss << m_deviceToBeLogged << "__EVENTS,type=\"SCHEMA\" schema_digest=\"" << schDigest << "\" " << ts
                   << "\n";
                KARABO_LOG_FRAMEWORK_DEBUG << "checkSchemaInDb ...\n" << o.payload;
                m_dbClientWrite->enqueueQuery(ss.str());
                m_dbClientWrite->flushBatch();
            }
        }


        bool InfluxDeviceData::logNewSchema(const std::string& schemaDigest, const std::vector<char>& schemaArchive) {
            using std::string_view;
            // Encode serialized schema into Base64
            const unsigned char* uarchive = reinterpret_cast<const unsigned char*>(schemaArchive.data());
            const std::string base64Schema = base64Encode(uarchive, schemaArchive.size());
            const std::size_t schemaSize = base64Schema.size();
            unsigned int newLogRate = newSchemaLogRate(schemaSize);
            if (newLogRate <= m_maxSchemaLogRateBytesSec) {
                // Log the new schema in chunks of up to "m_maxValueStringSize" bytes.
                // The first chunk is named "schema" for full backwards compatibility. The remaining
                // chunks are numbered starting from 1: "schema_1" is the second chunk, "schema_2" is
                // the third chunk and so on.
                const int nFullChunks = schemaSize / m_maxValueStringSize;
                const int lastChunkSize = schemaSize % m_maxValueStringSize;
                std::stringstream ss;
                ss << m_deviceToBeLogged << "__SCHEMAS," << "digest=\"" << schemaDigest << "\" digest_start=\""
                   << schemaDigest.substr(0, 8) << "\",schema_size=" << schemaSize
                   << "i,n_schema_chunks=" << nFullChunks + (lastChunkSize > 0 ? 1 : 0) << "i";
                for (int i = 0; i < nFullChunks; i++) {
                    ss << ",schema" << (i > 0 ? "_" + karabo::data::toString(i) : "") << "=\""
                       << string_view(base64Schema.data() + (i * m_maxValueStringSize), m_maxValueStringSize) << "\"";
                }
                if (lastChunkSize > 0) {
                    // Write the last chunk of the schema.
                    ss << ",schema" << (nFullChunks > 0 ? "_" + karabo::data::toString(nFullChunks) : "") << "=\""
                       << string_view(base64Schema.data() + (nFullChunks * m_maxValueStringSize), lastChunkSize)
                       << "\"";
                }
                ss << "\n";

                // Flush what was accumulated before ...
                m_dbClientWrite->flushBatch();
                m_dbClientWrite->enqueueQuery(ss.str());

                KARABO_LOG_FRAMEWORK_DEBUG << "Schema with digest '" << schemaDigest << "' for device '"
                                           << m_deviceToBeLogged << "' submitted to Influx. The schema has "
                                           << toString(schemaSize) << " bytes and has been saved in "
                                           << toString(nFullChunks + (lastChunkSize > 0 ? 1 : 0)) << " chunk(s).";

                return true;
            } else {
                // New schema cannot be logged - would violate max schema logging rate threshold.
                logRejectedDatum(RejectedData{RejectionType::SCHEMA_WRITE_RATE, m_deviceToBeLogged + "::schema",
                                              "Update of schema with size '" + toString(base64Schema.size() / 1024) +
                                                    " Kb' would reach a schema logging rate '" +
                                                    toString(newLogRate / 1024) + " Kb/sec'."});

                return false;
            }
        }


        void InfluxDataLogger::expectedParameters(karabo::data::Schema& expected) {
            OVERWRITE_ELEMENT(expected)
                  .key("state")
                  .setNewOptions(State::INIT, State::ON, State::ERROR)
                  .setNewDefaultValue(State::INIT)
                  .commit();

            STRING_ELEMENT(expected)
                  .key("urlWrite")
                  .displayedName("Influxdb URL (write)")
                  .description("URL should be given in form: tcp://host:port. 'Write' interface")
                  .assignmentOptional()
                  .defaultValue("tcp://localhost:8086")
                  .init()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("urlRead")
                  .displayedName("Influxdb URL (read)")
                  .description("URL should be given in form: tcp://host:port. 'Query' interface")
                  .assignmentOptional()
                  .defaultValue("tcp://localhost:8086")
                  .init()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("dbname")
                  .displayedName("Database name")
                  .description("Name of the database in which the data should be inserted")
                  .assignmentMandatory()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("maxBatchPoints")
                  .displayedName("Max. batch points")
                  .description("Max number of InfluxDB points in batch")
                  .assignmentOptional()
                  .defaultValue(200)
                  .init()
                  .commit();

            INT32_ELEMENT(expected)
                  .key("maxTimeAdvance")
                  .displayedName("Max Time Advance")
                  .description(
                        "Maximum time advance allowed for data. "
                        "Data too far ahead in the future will be dropped. "
                        "Negative values or 0 means no limit.")
                  .assignmentOptional()
                  .defaultValue(7200)
                  .unit(Unit::SECOND)
                  .init()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("maxVectorSize")
                  .displayedName("Max Vector Size")
                  .description(
                        "Vector properties longer than this are skipped and not written to the database. "
                        "(For tables, i.e. vector<Hash>, the limit is maxVectorSize / 10.)")
                  .assignmentOptional()
                  .defaultValue(4 * 2700) // four times number of bunches per EuXFEL train
                  .init()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("maxValueStringSize")
                  .displayedName("Max String Size")
                  .description(
                        "Maximum size, in characters, for a property value to be inserted into Influx and for a "
                        "schema chunk. "
                        "(All values are feed to Influx as strings in a text format called Line Protocol)")
                  .assignmentOptional()
                  .defaultValue(MAX_INFLUX_VALUE_LENGTH)
                  .maxInc(MAX_INFLUX_VALUE_LENGTH)
                  .init()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("maxPerDevicePropLogRate")
                  .displayedName("Max per Device Property Logging Rate (Kb/sec)")
                  .description(
                        "Entries for a device property that would move its logging rate above this threshold are "
                        "skipped.")
                  .assignmentOptional()
                  .defaultValue(5 * 1024) // 5 Mb/s
                  .minInc(1)              // 1 Kb/s
                  .init()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("propLogRatePeriod")
                  .displayedName("Interval for logging rate calc")
                  .description("Interval for calculating per device property logging rate")
                  .assignmentOptional()
                  .defaultValue(5)
                  .minInc(1)
                  .maxInc(60)
                  .unit(Unit::SECOND)
                  .init()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("maxSchemaLogRate")
                  .displayedName("Max Schema Logging Rate (Kb/sec)")
                  .description(
                        "Schema updates for a device that would move its schema logging rate above this threshold "
                        "are "
                        "skipped. Sizes are for the base64 encoded form of the binary serialized schema.")
                  .assignmentOptional()
                  .defaultValue(5 * 1024) // 5 Mb/s
                  .minInc(1)              // 1 Kb/s
                  .init()
                  .commit();

            UINT32_ELEMENT(expected)
                  .key("schemaLogRatePeriod")
                  .displayedName("Interval for schema logging rate calc")
                  .description("Interval for calculating per device schema logging rate")
                  .assignmentOptional()
                  .defaultValue(5)
                  .minInc(1)
                  .maxInc(60)
                  .unit(Unit::SECOND)
                  .init()
                  .commit();

            DOUBLE_ELEMENT(expected)
                  .key("safeSchemaRetentionPeriod")
                  .displayedName("Period for safe schema retention")
                  .description(
                        "For how long can a stored schema be safely assumed to be kept? Must be an "
                        "interval smaller than the database retention policy")
                  .assignmentOptional()
                  .defaultValue(2.0)
                  .minExc(0.0)
                  .unit(Unit::YEAR)
                  .init()
                  .commit();
        }


        InfluxDataLogger::InfluxDataLogger(const karabo::data::Hash& input)
            : DataLogger(input), m_dbName(input.get<std::string>("dbname")) {
            // We have to work in cluster environment where we have 2 nodes and proxy
            // that runs 'telegraf' working as a proxy and load balancer
            // All write requests should go to the load balancer
            // All queries should go to the one of 'influxdb' nodes directly
            //
            // We should be able to work in CI and local installation environments as well
            //
            // We can run CI with InfluxDB docker or even InfluxDB cluster by setting
            // the database name registered already in cluster DB

            m_urlWrite = input.get<std::string>("urlWrite");
            m_urlQuery = input.get<std::string>("urlRead");

            std::string dbUserWrite;
            if (getenv("KARABO_INFLUXDB_WRITE_USER")) {
                dbUserWrite = getenv("KARABO_INFLUXDB_WRITE_USER");
            } else {
                dbUserWrite = "infadm";
            }

            std::string dbPasswordWrite;
            if (getenv("KARABO_INFLUXDB_WRITE_PASSWORD")) {
                dbPasswordWrite = getenv("KARABO_INFLUXDB_WRITE_PASSWORD");
            } else {
                dbPasswordWrite = "admpwd";
            }

            std::string dbUserQuery;
            if (getenv("KARABO_INFLUXDB_QUERY_USER")) {
                dbUserQuery = getenv("KARABO_INFLUXDB_QUERY_USER");
            } else {
                dbUserQuery = dbUserWrite;
            }

            std::string dbPasswordQuery;
            if (getenv("KARABO_INFLUXDB_QUERY_PASSWORD")) {
                dbPasswordQuery = getenv("KARABO_INFLUXDB_QUERY_PASSWORD");
            } else {
                dbPasswordQuery = dbPasswordWrite;
            }

            Hash configWrite("dbname", m_dbName, "url", m_urlWrite, "durationUnit", INFLUX_DURATION_UNIT,
                             "maxPointsInBuffer", input.get<unsigned int>("maxBatchPoints"));

            configWrite.set("dbUser", dbUserWrite);
            configWrite.set("dbPassword", dbPasswordWrite);

            m_clientWrite = Configurator<InfluxDbClient>::create("InfluxDbClient", configWrite);

            Hash configRead("dbname", m_dbName, "url", m_urlQuery, "durationUnit", INFLUX_DURATION_UNIT,
                            "maxPointsInBuffer", input.get<unsigned int>("maxBatchPoints"));

            configRead.set("dbUser", dbUserQuery);
            configRead.set("dbPassword", dbPasswordQuery);
            configRead.set("disconnectOnIdle", true);

            m_clientRead = Configurator<InfluxDbClient>::create("InfluxDbClient", configRead);
        }


        InfluxDataLogger::~InfluxDataLogger() {}


        void InfluxDataLogger::preDestruction() {
            DataLogger::preDestruction();

            if (m_clientWrite->isConnected()) {
                auto prom = std::make_shared<std::promise<void>>();
                std::future<void> fut = prom->get_future();
                m_clientWrite->flushBatch([prom](const HttpResponse& resp) { prom->set_value(); });

                auto status = fut.wait_for(std::chrono::milliseconds(1500));

                if (status != std::future_status::ready) {
                    KARABO_LOG_FRAMEWORK_WARN << "Timeout in flushBatch while waiting for response from InfluxDB.";
                    return;
                }
                fut.get();
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "Skip final flush to influx since not connected";
            }
        }


        DeviceData::Pointer InfluxDataLogger::createDeviceData(const karabo::data::Hash& cfg) {
            Hash config = cfg;
            config.set("dbClientReadPointer", m_clientRead);
            config.set("dbClientWritePointer", m_clientWrite);
            config.set("maxTimeAdvance", get<int>("maxTimeAdvance"));
            config.set("maxVectorSize", get<unsigned int>("maxVectorSize"));
            config.set("maxValueStringSize", get<unsigned int>("maxValueStringSize"));
            config.set("maxPropLogRateBytesSec", get<unsigned int>("maxPerDevicePropLogRate") * 1024);
            config.set("propLogRatePeriod", get<unsigned int>("propLogRatePeriod"));
            config.set("maxSchemaLogRateBytesSec", get<unsigned int>("maxSchemaLogRate") * 1024);
            config.set("schemaLogRatePeriod", get<unsigned int>("schemaLogRatePeriod"));
            config.set("safeSchemaRetentionPeriod", get<double>("safeSchemaRetentionPeriod"));
            DeviceData::Pointer deviceData =
                  Factory<DeviceData>::create<karabo::data::Hash>("InfluxDataLoggerDeviceData", config);
            return deviceData;
        }


        void InfluxDataLogger::initializeLoggerSpecific() {
            m_clientWrite->startDbConnectIfDisconnected(bind_weak(&InfluxDataLogger::checkDb, this, _1));
        }


        void InfluxDataLogger::asyncCreateDbIfNeededAndStart() {
            std::string statement = "SHOW DATABASES";
            m_clientRead->queryDb(statement, bind_weak(&InfluxDataLogger::onShowDatabases, this, _1));
        }


        void InfluxDataLogger::onShowDatabases(const HttpResponse& o) {
            if (o.code >= 300) {
                KARABO_LOG_FRAMEWORK_ERROR << "Failed to view list of databases available: " << o.toString();
                updateState(State::ERROR,
                            Hash("status", "Failed to list databases. Response from Influx: " + o.toString()));
                return;
            }

            try {
                nl::json j = nl::json::parse(o.payload);
                auto values = j["results"][0]["series"][0]["values"];
                if (values.is_array()) {
                    // There's at least one database that is acessible to the user.
                    // See if the database to be used is available and then proceed with its use
                    for (nl::json::iterator it = values.begin(); it != values.end(); ++it) {
                        if ((*it)[0].get<std::string>() == m_dbName) {
                            KARABO_LOG_FRAMEWORK_INFO << "Database \"" << m_dbName << "\" already exists";
                            startConnection();
                            return;
                        }
                    }
                }
            } catch (const nl::json::parse_error& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Failed to parse list of databases from '" << o.payload << "' ("
                                           << e.what() << ").";
                updateState(State::ERROR, Hash("status", "Failed to unpack list of databases."));
                return;
            }

            KARABO_LOG_FRAMEWORK_INFO << "Database '" << m_dbName << "' not available. Will try to create it.";
            createDatabase(bind_weak(&karabo::devices::InfluxDataLogger::onCreateDatabase, this, _1));
        }


        void InfluxDataLogger::createDatabase(const InfluxResponseHandler& action) {
            const std::string statement = "CREATE DATABASE " + m_dbName;
            m_clientWrite->postQueryDb(statement, action);
            KARABO_LOG_FRAMEWORK_INFO << statement << "\n";
        }


        void InfluxDataLogger::onCreateDatabase(const HttpResponse& o) {
            if (o.code >= 300 || (o.code == 200 && o.payload.find("statement-id") == std::string::npos)) {
                // Database not available and could not be created. A response for an unsuccessful database creation
                // can also have a 200 status code but will have the fixed payload '{"result":[]}'. A successful
                // database creation will have a 200 status code, will have 'chunked' as transfer encoding and will
                // have the payload '{"results:[{"statement-id":0}]}'.
                KARABO_LOG_FRAMEWORK_ERROR << "Database '" << m_dbName << "' not available. "
                                           << "Tried to create it but got error with http status code '" << o.code
                                           << "' and message '" << o.message
                                           << "'. InfluxDataLogger going to ERROR state.";
                updateState(State::ERROR,
                            Hash("status",
                                 std::string("Database '") + m_dbName +
                                       "' not available. Influx response to create database request:" + o.toString()));
            } else {
                KARABO_LOG_FRAMEWORK_INFO << "Database " << m_dbName << " created";
                startConnection();
            }
        }


        void InfluxDataLogger::onPingDb(const HttpResponse& o) {
            if (o.code >= 300) {
                KARABO_LOG_FRAMEWORK_ERROR << "Failed to ping Influx DB: " << o.toString();
                updateState(State::ERROR, Hash("status", "Failed to ping InfluxDB."));
                return;
            }

            KARABO_LOG_FRAMEWORK_INFO << "X-Influxdb-Build: " << o.build << ", X-Influxdb-Version: " << o.version;
            asyncCreateDbIfNeededAndStart();
        }


        void InfluxDataLogger::checkDb(bool connected) {
            if (connected) {
                // A connection to the InfluxDb server host and port combination could be established.
                // Go ahead with the Ping -> Show Databases ... sequence.
                KARABO_LOG_FRAMEWORK_INFO << "PING InfluxDB server ...";
                m_clientWrite->getPingDb(bind_weak(&karabo::devices::InfluxDataLogger::onPingDb, this, _1));
            } else {
                // Either the InfluxDb server is not available or the connection params are invalid.
                const std::string errMsg("Failed to connect to Influx DB server at '" + m_urlWrite + "'");
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                updateState(State::ERROR, Hash("status", errMsg));
            }
        }


        void InfluxDataLogger::flushImpl(const std::shared_ptr<SignalSlotable::AsyncReply>& aReplyPtr) {
            karabo::net::InfluxResponseHandler handler;
            if (aReplyPtr) {
                handler = [weakThis{weak_from_this()}, aReplyPtr](const HttpResponse& resp) {
                    auto guard(weakThis.lock());
                    if (!guard) return; // Do not use AsyncReply anymore if device gone or being destructed
                    if (resp.code >= 300) {
                        std::ostringstream errMsg;
                        errMsg << "Flush request failed - InfluxDb response code/message: " << resp.code << " '"
                               << resp.message << "'";
                        aReplyPtr->error(errMsg.str());
                    } else {
                        (*aReplyPtr)();
                    }
                };
            }
            m_clientWrite->flushBatch(handler);
        }
    } // namespace devices
} // namespace karabo
