/*
 * File:   InfluxLogReader.cc
 *
 * Created on November 4, 2019, 9:09 AM
 *
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

#include "InfluxLogReader.hh"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <complex>
#include <nlohmann/json.hpp>
#include <sstream>
#include <vector>

#include "karabo/net/EventLoop.hh"
#include "karabo/util/FromLiteral.hh"

// The size of the batch of properties queried at once during slotGetConfigurationFromPast
constexpr int PROPS_BATCH_SIZE = 20;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, karabo::devices::DataLogReader,
                                  karabo::devices::InfluxLogReader)

namespace karabo {

    namespace devices {

        using namespace karabo::core;
        using namespace karabo::io;
        using namespace karabo::util;
        using namespace karabo::net;
        using namespace karabo::xms;
        using namespace std::placeholders;

        namespace nl = nlohmann;


        PropertyHistoryContext::PropertyHistoryContext(const std::string& deviceId, const std::string& property,
                                                       const karabo::util::Epochstamp& from,
                                                       const karabo::util::Epochstamp& to, int maxDataPoints,
                                                       const karabo::xms::SignalSlotable::AsyncReply& aReply,
                                                       const karabo::net::InfluxDbClient::Pointer& influxClient)
            : deviceId(deviceId),
              property(property),
              from(from),
              to(to),
              maxDataPoints(maxDataPoints),
              aReply(aReply),
              influxClient(influxClient){};

        double PropertyHistoryContext::getInterval() const {
            const TimeDuration d = to - from;
            return std::round(static_cast<double>(d) / maxDataPoints * 1'000'000);
        }


        PropFromPastInfo::PropFromPastInfo(const std::string& name, const karabo::util::Types::ReferenceType type,
                                           bool infiniteOrNan)
            : name(name), type(type), infiniteOrNan(infiniteOrNan) {}


        ConfigFromPastContext::ConfigFromPastContext(const std::string& deviceId,
                                                     const karabo::util::Epochstamp& atTime,
                                                     const karabo::xms::SignalSlotable::AsyncReply& aReply,
                                                     const karabo::net::InfluxDbClient::Pointer& influxClient)
            : deviceId(deviceId),
              atTime(atTime),
              configTimePoint(Epochstamp(0, 0)),
              lastLoginBeforeTime(0ull),
              lastLogoutBeforeTime(0ull),
              logFormatVersion(0),
              aReply(aReply),
              influxClient(influxClient){};

        const unsigned long InfluxLogReader::kFracConversionFactor = 1'000'000'000'000;
        const int InfluxLogReader::kMaxHistorySize = 10'000;
        const TimeValue InfluxLogReader::kMaxInfluxDataDelaySecs = 300ull;

        void InfluxLogReader::expectedParameters(karabo::util::Schema& expected) {
            OVERWRITE_ELEMENT(expected)
                  .key("state")
                  .setNewOptions(State::ON, State::ERROR)
                  .setNewDefaultValue(State::ON)
                  .commit();

            STRING_ELEMENT(expected)
                  .key("urlPropHistory")
                  .displayedName("URL for Property History")
                  .description(
                        "URL of InfluxDB used for slotGetPropertyHistory (typically shorter retention time).\n"
                        "If empty (default), use value of 'URL for Config. and Schema'")
                  .assignmentOptional()
                  .defaultValue(std::string())
                  .commit();

            STRING_ELEMENT(expected)
                  .key("urlConfigSchema")
                  .displayedName("URL for Config. and Schema")
                  .description(
                        "URL of InfluxDB used for slotGetConfigurationFromPast (typically longer retention time).\n")
                  .assignmentOptional()
                  .defaultValue("tcp://localhost:8086")
                  .commit();

            STRING_ELEMENT(expected)
                  .key("dbname")
                  .displayedName("Database name")
                  .description("Name of the database in which the data resides")
                  .assignmentMandatory()
                  .commit();

            INT32_ELEMENT(expected)
                  .key("maxHistorySize")
                  .displayedName("Max. Property History Size")
                  .description(
                        "Maximum value allowed for the 'maxNumData' parameter in a call to slot 'getPropertyHistory'.")
                  .assignmentOptional()
                  .defaultValue(kMaxHistorySize)
                  .init()
                  .commit();
        }


        InfluxLogReader::InfluxLogReader(const karabo::util::Hash& cfg)
            : karabo::devices::DataLogReader(cfg),
              m_dbName(cfg.get<std::string>("dbname")),
              m_urlConfigSchema(cfg.get<std::string>("urlConfigSchema")),
              m_urlPropHistory(cfg.get<std::string>("urlPropHistory")),
              m_hashSerializer(BinarySerializer<Hash>::create("Bin")),
              m_schemaSerializer(BinarySerializer<Schema>::create("Bin")),
              m_maxHistorySize(cfg.get<int>("maxHistorySize")),
              kNumberTypes({Types::to<ToLiteral>(Types::INT8), Types::to<ToLiteral>(Types::UINT8),
                            Types::to<ToLiteral>(Types::INT16), Types::to<ToLiteral>(Types::UINT16),
                            Types::to<ToLiteral>(Types::INT32), Types::to<ToLiteral>(Types::UINT32),
                            Types::to<ToLiteral>(Types::INT64),
                            // Warning! this is dangerous, arithmetic operators will be performed
                            // server side on the INT64 cast of the UINT64 value.
                            Types::to<ToLiteral>(Types::UINT64), Types::to<ToLiteral>(Types::FLOAT),
                            Types::to<ToLiteral>(Types::DOUBLE),
                            // _INF columns, despite storing string values, have to be among the numerical columns so
                            // the reader can use MEAN instead of SAMPLE when reducing the data points. As it can be
                            // seen from the MEAN function documentation at
                            // https://docs.influxdata.com/influxdb/v1.8/query_language/functions/#mean,
                            // non-numerical values are skipped during the averaging.
                            Types::to<ToLiteral>(Types::FLOAT) + "_INF",
                            Types::to<ToLiteral>(Types::DOUBLE) + "_INF"}) {
            KARABO_SLOT(slotGetBadData, std::string /*from*/, std::string /*to*/);
            if (getenv("KARABO_INFLUXDB_QUERY_USER")) {
                m_dbUser = getenv("KARABO_INFLUXDB_QUERY_USER");
            } else {
                m_dbUser = "infadm";
            }
            std::string dbPassword;
            if (getenv("KARABO_INFLUXDB_QUERY_PASSWORD")) {
                m_dbPassword = getenv("KARABO_INFLUXDB_QUERY_PASSWORD");
            } else {
                m_dbPassword = "admpwd";
            }
            m_durationUnit = toInfluxDurationUnit(TIME_UNITS::MICROSEC);
        }


        InfluxLogReader::~InfluxLogReader() {
            KARABO_LOG_FRAMEWORK_DEBUG << this->getInstanceId() << " being destroyed.";
        }


        Hash InfluxLogReader::buildInfluxClientConfig(const std::string& dbUrlForSlot) const {
            Hash dbClientCfg("dbname", m_dbName, "durationUnit", INFLUX_DURATION_UNIT, "dbUser", m_dbUser, "dbPassword",
                             m_dbPassword);
            dbClientCfg.set("url", dbUrlForSlot);

            return dbClientCfg;
        }


        void InfluxLogReader::slotGetPropertyHistoryImpl(const std::string& deviceId, const std::string& property,
                                                         const Hash& params) {
            Epochstamp from;
            if (params.has("from")) from = Epochstamp(params.get<std::string>("from"));
            Epochstamp to;
            if (params.has("to")) to = Epochstamp(params.get<std::string>("to"));
            int maxNumData = m_maxHistorySize;
            if (params.has("maxNumData")) maxNumData = params.get<int>("maxNumData");
            if (maxNumData == 0) {
                // 0 is interpreted as unlimited, but for the Influx case a limit is
                // always enforced.
                maxNumData = m_maxHistorySize;
            }

            if (maxNumData < 0 || maxNumData > m_maxHistorySize) {
                throw KARABO_PARAMETER_EXCEPTION(
                      "Requested maximum number of data points ('maxNumData') is " + util::toString(maxNumData) +=
                      " which surpasses the limit of " + util::toString(m_maxHistorySize) +=
                      ". Property History polling is not designed for Scientific Data Analysis.");
            }

            // This prevents the slot from sending an automatic empty response at the end of
            // the slot method execution. Either a success reply or an error reply must be
            // sent exactly once from one of the other methods involved in the processing of
            // the slot call. A successful reply can be sent through the AsyncReply operator
            // () - AsyncReply is a functor. An error reply can be sent through AsyncReply
            // error method.
            SignalSlotable::AsyncReply aReply(this);

            std::string propHistUrl = m_urlPropHistory;
            if (propHistUrl.empty()) {
                propHistUrl = m_urlConfigSchema;
            }
            Hash config = buildInfluxClientConfig(propHistUrl);
            karabo::net::InfluxDbClient::Pointer influxClient =
                  Configurator<InfluxDbClient>::create("InfluxDbClient", config);

            auto ctxtPtr(std::make_shared<PropertyHistoryContext>(deviceId, property, from, to, maxNumData, aReply,
                                                                  influxClient));

            asyncDataCountForProperty(ctxtPtr);
        }


        void InfluxLogReader::asyncDataCountForProperty(const std::shared_ptr<PropertyHistoryContext>& ctxt) {
            std::ostringstream iqlQuery;

            /* The query for data count, differently from the query for the property values (or samples) that will
               be executed later,  doesn't select the '_tid' field. The goal of this query is to count how many
               entries will exist in the property history and '_tid' field entries only make into the resulting
               property history as attributes of entries. */
            iqlQuery << "SELECT COUNT(/^" << ctxt->property << "-.[A-Z0-9_]+/) FROM \"" << ctxt->deviceId
                     << "\" WHERE time >= " << epochAsMicrosecString(ctxt->from) << m_durationUnit
                     << " AND time <= " << epochAsMicrosecString(ctxt->to) << m_durationUnit;

            const std::string queryStr = iqlQuery.str();

            try {
                ctxt->influxClient->queryDb(queryStr,
                                            bind_weak(&InfluxLogReader::onDataCountForProperty, this, _1, ctxt));
            } catch (const std::exception& e) {
                const std::string errMsg = onException("Error querying data count for property");
                // As this is in the same thread at which the slot call started, if we send the async reply directly,
                // the reply will be sent, and then unregistered from the SignalSlotable. When this method execution
                // finishes soon after and the control returns to the SignalSlotable, it won't find any asynchronous
                // reply registered and will send the default empty reply, ignoring that a reply has already been sent.
                // That's the reason for posting the reply to the event loop instead of sending it directly. The
                // remaining calls to ctxt->aReply.error in the processing of the slot can be sent directly.
                std::weak_ptr<karabo::xms::SignalSlotable> weakThis(weak_from_this());
                EventLoop::getIOService().post([weakThis, ctxt, errMsg]() {
                    // Only sends a reply if the InfluxLogReader instance is still alive - lock() call is successful.
                    std::shared_ptr<karabo::xms::SignalSlotable> ptr = weakThis.lock();
                    if (ptr) {
                        ctxt->aReply.error(errMsg);
                    }
                });
            }
        }


        void InfluxLogReader::onDataCountForProperty(const karabo::net::HttpResponse& dataCountResp,
                                                     const std::shared_ptr<PropertyHistoryContext>& ctxt) {
            bool fullyHandled = preHandleHttpResponse(dataCountResp, ctxt->aReply);

            if (fullyHandled) {
                // An error happened and has been reported to the slot caller.
                // Nothing left for the execution of this slot.
                return;
            }

            unsigned long long dataCount = 0;
            nl::json respObj;
            try {
                // The format of the data received is available here
                //  https://docs.influxdata.com/influxdb/v1.7/guides/querying_data/
                respObj = nl::json::parse(dataCountResp.payload);
                const auto& values = respObj["results"][0]["series"][0]["values"];
                for (const auto& value : values) {
                    auto countValue = value[1].get<unsigned long long>();
                    dataCount += countValue;
                }
            } catch (const std::exception& e) {
                const std::string errMsg = onException("Error summing up amount of values");
                ctxt->aReply.error(errMsg);
                return;
            }

            bool allNumbers = true; // check if all fields support statistical aggregators
            try {
                const auto& columns = respObj["results"][0]["series"][0]["columns"];
                for (const auto& column : columns) {
                    const std::string& columnStr = column.get<std::string>();
                    if (columnStr == "time") {
                        // "time" column in Influx response should not be type
                        // checked - it is always a numeric data type, won't be
                        // averaged by and does not follow the data type suffix
                        // convention.
                        continue;
                    }
                    const std::string::size_type typeSeparatorPos = columnStr.rfind('-');
                    if (typeSeparatorPos != std::string::npos) {
                        const std::string typeName = columnStr.substr(typeSeparatorPos + 1);
                        if (kNumberTypes.find(typeName) != kNumberTypes.end()) {
                            continue;
                        } else {
                            allNumbers = false;
                            break;
                        }
                    } else {
                        allNumbers = false;
                        KARABO_LOG_FRAMEWORK_ERROR << "Query for property '" << ctxt->deviceId << "." << ctxt->property
                                                   << "' returned column without type separator '" << column << "'";
                    }
                }
            } catch (const std::exception& e) {
                const std::string errMsg = onException("Error checking if fields support statistics aggregators");
                ctxt->aReply.error(errMsg);
                return;
            }

            if (dataCount < 1) {
                // No data point for the given period.
                ctxt->aReply(ctxt->deviceId, ctxt->property, std::vector<Hash>());
                onOk();
            } else if (dataCount <= ctxt->maxDataPoints) {
                asyncGetPropertyValues(ctxt);
            } else if (allNumbers) { // group by mean
                asyncGetPropertyValuesMean(ctxt);
            } else { // sample down
                asyncGetPropertyValuesSamples(ctxt);
            }
        }


        void InfluxLogReader::asyncGetPropertyValues(const std::shared_ptr<PropertyHistoryContext>& ctxt) {
            std::ostringstream iqlQuery;

            iqlQuery << "SELECT /^" << ctxt->property << "-[A-Z0-9_]+$/ FROM \"" << ctxt->deviceId
                     << "\" WHERE time >= " << epochAsMicrosecString(ctxt->from) << m_durationUnit
                     << " AND time <= " << epochAsMicrosecString(ctxt->to) << m_durationUnit;

            const std::string queryStr = iqlQuery.str();

            try {
                ctxt->influxClient->queryDb(queryStr,
                                            bind_weak(&InfluxLogReader::onPropertyValues, this, _1, "", ctxt));
            } catch (const std::exception& e) {
                const std::string errMsg = onException("Error querying property values");
                ctxt->aReply.error(errMsg);
            }
        }

        void InfluxLogReader::asyncGetPropertyValuesSamples(const std::shared_ptr<PropertyHistoryContext>& ctxt) {
            std::ostringstream iqlQuery;

            iqlQuery << "SELECT SAMPLE(/^" << ctxt->property << "-[A-Z0-9_]+$/, " << ctxt->maxDataPoints << ") FROM \""
                     << ctxt->deviceId << "\" WHERE time >= " << epochAsMicrosecString(ctxt->from) << m_durationUnit
                     << " AND time <= " << epochAsMicrosecString(ctxt->to) << m_durationUnit;

            const std::string queryStr = iqlQuery.str();

            try {
                ctxt->influxClient->queryDb(queryStr,
                                            bind_weak(&InfluxLogReader::onPropertyValues, this, _1, "sample_", ctxt));
            } catch (const std::exception& e) {
                const std::string errMsg = onException("Error querying property values samples");
                ctxt->aReply.error(errMsg);
            }
        }


        void InfluxLogReader::onPropertyValues(const karabo::net::HttpResponse& valuesResp,
                                               const std::string& columnPrefixToRemove,
                                               const std::shared_ptr<PropertyHistoryContext>& ctxt) {
            bool fullyHandled = preHandleHttpResponse(valuesResp, ctxt->aReply);

            if (fullyHandled) {
                // An error happened and has been reported to the slot caller.
                // Nothing left for the execution of this slot.
                return;
            }

            try {
                InfluxResultSet influxResult;
                jsonResultsToInfluxResultSet(valuesResp.payload, influxResult, columnPrefixToRemove);

                std::vector<Hash> propValues;
                influxResultSetToVectorHash(influxResult, propValues);

                ctxt->aReply(ctxt->deviceId, ctxt->property, propValues);
                onOk();

            } catch (const std::exception& e) {
                std::ostringstream oss;
                oss << "Error retrieving values of property '" << ctxt->property << "' of device '" << ctxt->deviceId
                    << "' between '" << ctxt->from.toIso8601Ext() << "' and '" << ctxt->to.toIso8601Ext() << "'";
                const std::string errMsg = onException(oss.str());
                ctxt->aReply.error(errMsg);
            }
        }

        void InfluxLogReader::asyncGetPropertyValuesMean(const std::shared_ptr<PropertyHistoryContext>& ctxt) {
            std::ostringstream iqlQuery;
            iqlQuery << "SELECT MEAN(/^" << ctxt->property << "-[A-Z0-9_]+$/) FROM \"" << ctxt->deviceId
                     << "\" WHERE time >= " << epochAsMicrosecString(ctxt->from) << m_durationUnit
                     << " AND time <= " << epochAsMicrosecString(ctxt->to) << m_durationUnit << " GROUP BY time("
                     << toString(ctxt->getInterval()) << m_durationUnit << ") fill(none)";

            const std::string queryStr = iqlQuery.str();

            try {
                ctxt->influxClient->queryDb(queryStr,
                                            bind_weak(&InfluxLogReader::onMeanPropertyValues, this, _1, ctxt));
            } catch (const std::exception& e) {
                const std::string errMsg = onException("Error querying property values samples");
                ctxt->aReply.error(errMsg);
            }
        }


        void InfluxLogReader::onMeanPropertyValues(const karabo::net::HttpResponse& valuesResp,
                                                   const std::shared_ptr<PropertyHistoryContext>& ctxt) {
            bool fullyHandled = preHandleHttpResponse(valuesResp, ctxt->aReply);

            if (fullyHandled) {
                // An error happened and has been reported to the slot caller.
                // Nothing left for the execution of this slot.
                return;
            }

            try {
                InfluxResultSet influxResult;
                jsonResultsToInfluxResultSet(valuesResp.payload, influxResult, "");

                std::vector<Hash> propValues;
                propValues.reserve(influxResult.second.size());
                // Converts each row of values into a Hash.
                // Gets the data type names of each column.
                std::vector<std::string> colTypeNames(influxResult.first.size());
                std::vector<std::string> colKeyNames(influxResult.first.size());
                for (size_t col = 0; col < influxResult.first.size(); col++) {
                    const std::string::size_type typeSeparatorPos = influxResult.first[col].rfind('-');
                    if (typeSeparatorPos != std::string::npos) {
                        const std::string typeName = influxResult.first[col].substr(typeSeparatorPos + 1);
                        colTypeNames[col] = typeName;
                    }
                    colKeyNames[col] = "v"; // the mean value will be passed on in the key "v" to match the protocol
                }
                for (const std::vector<boost::optional<std::string>>& valuesRow : influxResult.second) {
                    unsigned long long time =
                          std::stoull(*(valuesRow[0])); // This is safe as Influx always returns time.
                    Epochstamp epoch(time / INFLUX_PRECISION_FACTOR,
                                     (time % INFLUX_PRECISION_FACTOR) * kFracConversionFactor);
                    Hash hash;
                    for (size_t col = 1; col < influxResult.first.size(); col++) {
                        if (!valuesRow[col]) {
                            // Skips any null value in the result set - any row returned by Influx will have at least
                            // one non-null value (maybe an empty string).
                            continue;
                        }
                        // For nan/inf floating points we added "_INF" we skip.
                        const std::string& typeNameInflux = colTypeNames[col];
                        const Types::ReferenceType type = Types::from<FromLiteral>(typeNameInflux);
                        addNodeToHash(hash, colKeyNames[col], type, 0ull, epoch, *(valuesRow[col]));
                        // skip further columns. In the rare case of schema evolution in the same interval
                        // we take the first one reported. Multiple entries on the same timestamp will be an issue.
                        break;
                    }
                    if (hash.has("v")) {
                        // TODO: the timestamp is the beginning of the interval group.
                        //       we should add half the time interval to center the time interval
                        //       the last interval should be half of the beginning of the interval and the
                        //       end of the query (ctxt->to).
                        // https://docs.influxdata.com/influxdb/v1.8/query_language/explore-data/#the-group-by-clause
                        propValues.push_back(std::move(hash));
                    }
                }

                ctxt->aReply(ctxt->deviceId, ctxt->property, propValues);
                onOk();

            } catch (const std::exception& e) {
                std::ostringstream oss;
                oss << "Error retrieving values of property '" << ctxt->property << "' of device '" << ctxt->deviceId
                    << "' between '" << ctxt->from.toIso8601Ext() << "' and '" << ctxt->to.toIso8601Ext() << "'";
                const std::string errMsg = onException(oss.str());
                ctxt->aReply.error(errMsg);
            }
        }

        void InfluxLogReader::slotGetConfigurationFromPastImpl(const std::string& deviceId,
                                                               const std::string& timepoint) {
            Epochstamp atTime(timepoint);
            SignalSlotable::AsyncReply aReply(this);

            Hash config = buildInfluxClientConfig(m_urlConfigSchema);
            karabo::net::InfluxDbClient::Pointer influxClient =
                  Configurator<InfluxDbClient>::create("InfluxDbClient", config);

            ConfigFromPastContext ctxt(deviceId, atTime, aReply, influxClient);

            asyncLastLoginFormatBeforeTime(std::make_shared<ConfigFromPastContext>(ctxt));
        }


        void InfluxLogReader::asyncLastLoginFormatBeforeTime(const std::shared_ptr<ConfigFromPastContext>& ctxt) {
            std::ostringstream iqlQuery;

            iqlQuery << "SELECT karabo_user, format FROM \"" << ctxt->deviceId
                     << R"(__EVENTS" WHERE "type" = '"+LOG"' AND time <= )" << epochAsMicrosecString(ctxt->atTime)
                     << m_durationUnit << " ORDER BY DESC LIMIT 1";

            const std::string queryStr = iqlQuery.str();

            try {
                ctxt->influxClient->queryDb(queryStr,
                                            bind_weak(&InfluxLogReader::onLastLoginFormatBeforeTime, this, _1, ctxt));
            } catch (const std::exception& e) {
                const std::string errMsg = onException("Error querying last login before time");
                std::weak_ptr<karabo::xms::SignalSlotable> weakThis(weak_from_this());
                EventLoop::getIOService().post([weakThis, ctxt, errMsg]() {
                    // Only sends a reply if the InfluxLogReader instance is still alive - lock() call is successful.
                    std::shared_ptr<karabo::xms::SignalSlotable> ptr = weakThis.lock();
                    if (ptr) {
                        ctxt->aReply.error(errMsg);
                    }
                });
            }
        }


        void InfluxLogReader::onLastLoginFormatBeforeTime(const karabo::net::HttpResponse& valueResp,
                                                          const std::shared_ptr<ConfigFromPastContext>& ctxt) {
            bool fullyHandled = preHandleHttpResponse(valueResp, ctxt->aReply);

            if (fullyHandled) {
                // An error happened and has been reported to the slot caller.
                // Nothing left for the execution of this slot.
                return;
            }

            try {
                unsigned long long loginBeforeTime = 0ull;
                int logFormatVersion = 0;

                nl::json respObj = nl::json::parse(valueResp.payload);
                // values will have just one record, values[0] (due to LIMIT 1 in the query)
                // values[0][0] - timestamp
                // values[0][1] - karabo_user
                // values[0][2] - format (can be null)
                auto loginVal = respObj["results"][0]["series"][0]["values"][0][0];
                if (!loginVal.is_null()) {
                    // Db has a Login event before time.
                    loginBeforeTime = loginVal.get<unsigned long long>();
                }
                ctxt->lastLoginBeforeTime = loginBeforeTime;
                auto formatVal = respObj["results"][0]["series"][0]["values"][0][2];
                if (!formatVal.is_null()) {
                    logFormatVersion = formatVal.get<int>();
                }
                ctxt->logFormatVersion = logFormatVersion;
            } catch (const std::exception& e) {
                std::ostringstream oss;
                oss << "Error retrieving timestamp and log format for last instantiation of device '" << ctxt->deviceId
                    << "' before '" << ctxt->atTime.toIso8601Ext() << "' as part of operation getConfigurationFromPast";
                const std::string errMsg = onException(oss.str());
                ctxt->aReply.error(errMsg);
                return;
            }

            asyncLastLogoutBeforeTime(ctxt);
        }


        void InfluxLogReader::asyncLastLogoutBeforeTime(const std::shared_ptr<ConfigFromPastContext>& ctxt) {
            std::ostringstream iqlQuery;

            iqlQuery << "SELECT LAST(karabo_user) FROM \"" << ctxt->deviceId << "__EVENTS\""
                     << R"( WHERE "type" = '"-LOG"' AND time <= )" << epochAsMicrosecString(ctxt->atTime)
                     << m_durationUnit;

            const std::string queryStr = iqlQuery.str();

            try {
                ctxt->influxClient->queryDb(queryStr,
                                            bind_weak(&InfluxLogReader::onLastLogoutBeforeTime, this, _1, ctxt));
            } catch (const std::exception& e) {
                const std::string errMsg = onException("Error querying last logout before time");
                ctxt->aReply.error(errMsg);
            }
        }


        void InfluxLogReader::onLastLogoutBeforeTime(const karabo::net::HttpResponse& valueResp,
                                                     const std::shared_ptr<ConfigFromPastContext>& ctxt) {
            bool fullyHandled = preHandleHttpResponse(valueResp, ctxt->aReply);

            if (fullyHandled) {
                // An error happened and has been reported to the slot caller.
                // Nothing left for the execution of this slot.
                return;
            }

            try {
                unsigned long long lastLogoutBeforeTime = 0ull;

                // Note: all the accesses to keys or indexes of the deserialized
                //       JSON object sent by Influx do not throw any exception
                //       if the key or index doesn't exist. This is an intended
                //       feature of the nlohmann JSON library. Further details
                //       at https://json.nlohmann.me/features/element_access/unchecked_access/.
                //       The "unchecked" accesses made to the paths inside the
                //       deserialized JSON object are therefore safe, as long as
                //       the deserialized object is not declared as const.
                nl::json respObj = nl::json::parse(valueResp.payload);
                auto values = respObj["results"][0]["series"][0]["values"];
                auto value = values[0][0];
                if (!value.is_null()) {
                    // Db has a last Logout event before time.
                    lastLogoutBeforeTime = value.get<unsigned long long>();
                }
                ctxt->lastLogoutBeforeTime = lastLogoutBeforeTime;
            } catch (const std::exception& e) {
                std::ostringstream oss;
                oss << "Error retrieving timestamp of last end of logging for device '" << ctxt->deviceId
                    << "' before '" << ctxt->atTime.toIso8601Ext() << "' as part of operation getConfigurationFromPast";
                const std::string errMsg = onException(oss.str());
                ctxt->aReply.error(errMsg);
                return;
            }

            asyncLastSchemaDigestBeforeTime(ctxt);
        }


        void InfluxLogReader::asyncLastSchemaDigestBeforeTime(const std::shared_ptr<ConfigFromPastContext>& ctxt) {
            std::ostringstream iqlQuery;

            iqlQuery << "SELECT LAST(schema_digest) FROM \"" << ctxt->deviceId
                     << R"(__EVENTS" WHERE "type" = '"SCHEMA"' AND time <= )" << epochAsMicrosecString(ctxt->atTime)
                     << m_durationUnit;

            const std::string queryStr = iqlQuery.str();

            try {
                ctxt->influxClient->queryDb(queryStr,
                                            bind_weak(&InfluxLogReader::onLastSchemaDigestBeforeTime, this, _1, ctxt));
            } catch (const std::exception& e) {
                const std::string errMsg = onException("Error querying last schema digest before time");
                ctxt->aReply.error(errMsg);
            }
        }


        void InfluxLogReader::onLastSchemaDigestBeforeTime(const karabo::net::HttpResponse& valueResp,
                                                           const std::shared_ptr<ConfigFromPastContext>& ctxt) {
            bool fullyHandled = preHandleHttpResponse(valueResp, ctxt->aReply);

            if (fullyHandled) {
                // An error happened and has been reported to the slot caller.
                // Nothing left for the execution of this slot.
                return;
            }

            std::string digest;
            try {
                nl::json respObj = nl::json::parse(valueResp.payload);
                auto value = respObj["results"][0]["series"][0]["values"][0][1];
                if (value.is_null()) {
                    // No digest has been found - it's not possible to go ahead.
                    // Note that following text is expected in BaseLogging_Test::testCfgFromPastRestart
                    std::ostringstream oss;
                    oss << "No active schema could be found for device at (or before) timepoint.";
                    Epochstamp currTime;
                    TimeDuration elapsed = currTime - ctxt->atTime;
                    const TimeValue atTimeSecsAgo = elapsed.getTotalSeconds();
                    if (atTimeSecsAgo <= kMaxInfluxDataDelaySecs && currTime > ctxt->atTime) {
                        // The requested timepoint is not "old" enough - there's a chance that the schema will be
                        // available soon in InfluxDb.
                        oss << " As the requested time point is " << atTimeSecsAgo
                            << " secs. ago, the schema for device may soon be available.";
                    }
                    const std::string errMsg = oss.str();
                    KARABO_LOG_FRAMEWORK_ERROR << "For device '" << ctxt->deviceId << "': " << errMsg;
                    ctxt->aReply.error(errMsg);
                    return;
                } else {
                    digest = value.get<std::string>();
                }
            } catch (const std::exception& e) {
                std::ostringstream oss;
                oss << "Error retrieving schema that was active for device '" << ctxt->deviceId << "' at '"
                    << ctxt->atTime.toIso8601Ext() << "' as part of operation getConfigurationFromPast";
                const std::string errMsg = onException(oss.str());
                ctxt->aReply.error(errMsg);
                return;
            }

            asyncSchemaForDigest(digest, ctxt);
        }


        void InfluxLogReader::asyncSchemaForDigest(const std::string& digest,
                                                   const std::shared_ptr<ConfigFromPastContext>& ctxt) {
            std::ostringstream iqlQuery;

            iqlQuery << R"(SELECT * FROM ")" << ctxt->deviceId << R"(__SCHEMAS" WHERE "digest"='")" << digest
                     << R"("' ORDER BY time DESC LIMIT 1)";
            const std::string queryStr = iqlQuery.str();

            try {
                ctxt->influxClient->queryDb(queryStr,
                                            bind_weak(&InfluxLogReader::onSchemaForDigest, this, _1, ctxt, digest));
            } catch (const std::exception& e) {
                const std::string errMsg = onException("Error querying schema for digest");
                ctxt->aReply.error(errMsg);
            }
        }


        void InfluxLogReader::onSchemaForDigest(const karabo::net::HttpResponse& schemaResp,
                                                const std::shared_ptr<ConfigFromPastContext>& ctxt,
                                                const std::string& digest) {
            bool fullyHandled = preHandleHttpResponse(schemaResp, ctxt->aReply);

            if (fullyHandled) {
                // An error happened and has been reported to the slot caller.
                // Nothing left for the execution of this slot.
                return;
            }

            std::string encodedSch;
            try {
                nl::json respObj = nl::json::parse(schemaResp.payload);
                // Creates a map with the columns in the response - the columns names
                // are the keys and the columns indexes are the values. The first column,
                // index 0, is the time and is skipped since it won't be used.
                std::map<std::string, int> colMap;
                const auto& series = respObj["results"][0]["series"];
                if (series.is_null()) {
                    // The returned json is completely empty - an empty result for an InfluxQL
                    // query is '{"results:":[{}]}'. Any non-empty result will contain at least a
                    // "series" key.
                    throw KARABO_PARAMETER_EXCEPTION("No schema found for digest. Influx's response: " +
                                                     schemaResp.payload);
                }
                const auto& respColumns = series[0]["columns"];
                for (size_t i = 1; i < respColumns.size(); i++) {
                    colMap[respColumns[i].get<std::string>()] = i;
                }
                // Initializes a reference to the values of the single "record" retrieved from Influx.
                // It can be assumed that there is a single "record" in the response because the query
                // that generated the response uses "ORDER BY time DESC" followed by "LIMIT 1".
                const auto& respValues = series[0]["values"][0];

                int schemaChunks = 1;
                const auto nChunksIt = colMap.find("n_schema_chunks");
                if (nChunksIt != colMap.end()) {
                    // Schemas saved before schema chunking will have null for the
                    // n_schema_chunks metrics
                    if (respValues[nChunksIt->second].is_number()) {
                        schemaChunks = respValues[nChunksIt->second].get<int>();
                    }
                }

                std::stringstream base64Sch;
                base64Sch << respValues[colMap["schema"]].get<std::string>();
                for (int i = 1; i < schemaChunks; i++) {
                    base64Sch << respValues[colMap["schema_" + karabo::util::toString(i)]].get<std::string>();
                }

                encodedSch = base64Sch.str();

            } catch (const std::exception& e) {
                std::ostringstream oss;
                oss << "Error retrieving schema with digest '" << digest << "' for device '" << ctxt->deviceId
                    << "' at '" << ctxt->atTime.toIso8601Ext() << "'";
                const std::string errMsg = onException(oss.str());
                ctxt->aReply.error(errMsg);
                return;
            }

            // A schema has been found - processing it means base64 decoding, deserializing and
            // then iterating over it to capture all the properties keys and their types for further processing.
            try {
                // The use of the specialized method loadLastFromSequence is needed because schemas saved in Influx
                // prior to the fix in https://git.xfel.eu/Karabo/Framework/-/merge_requests/6470 can have multiple
                // (and different) versions of a device's schema. When that happens, the version that must be
                // retrieved is the last one (the most recent at the time the schema was saved in Influx).
                std::vector<unsigned char> decodedSch;
                base64Decode(encodedSch, decodedSch);
                const char* decoded = reinterpret_cast<const char*>(decodedSch.data());
                m_schemaSerializer->loadLastFromSequence(ctxt->configSchema, decoded, decodedSch.size());
                const Schema& schema = ctxt->configSchema;

                // Stores the properties keys and types in the context.
                ctxt->propsInfo.clear();
                std::vector<std::string> schPaths = schema.getDeepPaths();
                for (const std::string& path : schPaths) {
                    if (schema.isLeaf(path) &&
                        !(schema.hasArchivePolicy(path) && schema.getArchivePolicy(path) == Schema::NO_ARCHIVING)) {
                        // Current path is for a leaf node that set is to archive (more precisely, not set to not
                        // archive).
                        const Types::ReferenceType valType = schema.getValueType(path);
                        ctxt->propsInfo.emplace_back(path, valType, false);
                        if (valType == Types::FLOAT || valType == Types::DOUBLE) {
                            // For floating point properties we also "schedule"
                            // their infinite or Nan potential values for retrieval
                            ctxt->propsInfo.emplace_back(path, valType, true);
                        }
                    }
                }
            } catch (const std::exception& e) {
                std::ostringstream oss;
                oss << "Error processing schema retrieved for device '" << ctxt->deviceId << "' at '"
                    << ctxt->atTime.toIso8601Ext() << "'";
                const std::string errMsg = onException(oss.str());
                ctxt->aReply.error(errMsg);
                return;
            }

            // Triggers the sequence of configuration value retrievals. The configuration
            // values retrievals are an interplay between asyncPropValueBeforeTime and
            // onAsyncPropValueBeforeTime - they will both consume the propsInfo
            // vector, sending a response back to the slotGetConfigurationFromPast caller
            // when the last property value is retrieved.
            asyncPropValueBeforeTime(ctxt);
        }


        void InfluxLogReader::asyncPropValueBeforeTime(const std::shared_ptr<ConfigFromPastContext>& ctxt) {
            int nProps = 0;
            std::ostringstream iqlQuery;
            std::vector<PropFromPastInfo> propInfos;
            do {
                const PropFromPastInfo propInfo = ctxt->propsInfo.front();
                ctxt->propsInfo.pop_front();

                std::string fieldKey{propInfo.name};

                fieldKey += "-" + Types::to<ToLiteral>(propInfo.type);
                if (propInfo.infiniteOrNan) {
                    // We are supposed to retrieve a potential NAN or INF value
                    // for the property.
                    fieldKey += "_INF";
                }

                iqlQuery << (nProps > 0 ? "; " : "") << "SELECT LAST(\"" << fieldKey << "\") AS \"" << fieldKey
                         << "\" FROM \"" << ctxt->deviceId << "\" WHERE time <= " << epochAsMicrosecString(ctxt->atTime)
                         << m_durationUnit;

                if (ctxt->lastLoginBeforeTime != 0ull && ctxt->logFormatVersion > 0) {
                    // This is possible since in the new format timestamps older than the start of logging are
                    // replaced by start of logging. The restricted search in the past ensures that for unset
                    // properties with `noDefaultValue`, our query here does not return old values from previous
                    // incarnations of the device. For old data we need to keep the old behaviour since otherwise
                    // properties would be lost that had timestamps shortly before start of logging.
                    iqlQuery << " AND time >= " << ctxt->lastLoginBeforeTime << m_durationUnit;
                }

                propInfos.push_back(propInfo);
                nProps++;

            } while (nProps < PROPS_BATCH_SIZE && !ctxt->propsInfo.empty());

            const std::string queryStr = iqlQuery.str();

            try {
                ctxt->influxClient->queryDb(
                      queryStr, bind_weak(&InfluxLogReader::onPropValueBeforeTime, this, propInfos, _1, ctxt));
            } catch (const std::exception& e) {
                const auto errMsg = onException("Error querying property value before time");
                ctxt->aReply.error(errMsg);
            }
        }


        void InfluxLogReader::onPropValueBeforeTime(const std::vector<PropFromPastInfo>& propInfos,
                                                    const karabo::net::HttpResponse& propValueResp,
                                                    const std::shared_ptr<ConfigFromPastContext>& ctxt) {
            bool fullyHandled = preHandleHttpResponse(propValueResp, ctxt->aReply);

            if (fullyHandled) {
                // An error happened and has been reported to the slot caller.
                // Nothing left for the execution of this slot.
                return;
            }

            try {
                nl::json respObj = nl::json::parse(propValueResp.payload);
                const std::size_t nProps = respObj["results"].size();

                for (std::size_t propIdx = 0; propIdx < nProps; propIdx++) {
                    const std::string& propName = propInfos[propIdx].name;
                    const karabo::util::Types::ReferenceType& propType = propInfos[propIdx].type;

                    try {
                        const auto& value = respObj["results"][propIdx]["series"][0]["values"][0][1];
                        if (!value.is_null()) {
                            auto timeObj = respObj["results"][propIdx]["series"][0]["values"][0][0];
                            const Epochstamp timeEpoch(toEpoch(timeObj.get<unsigned long long>()));
                            if (timeEpoch > ctxt->configTimePoint) {
                                ctxt->configTimePoint = timeEpoch;
                            }
                            const boost::optional<std::string> valueAsString = jsonValueAsString(value);
                            if (valueAsString) {
                                if (!ctxt->configHash.has(propName)) {
                                    // The normal case - the result is not yet there
                                    addNodeToHash(ctxt->configHash, propName, propType, 0, timeEpoch, *valueAsString);
                                } else {
                                    // Second query for field corresponding to property that
                                    // has already been queried.
                                    if (propInfos[propIdx].infiniteOrNan) {
                                        const Epochstamp stampQuery1(
                                              Epochstamp::fromHashAttributes(ctxt->configHash.getAttributes(propName)));
                                        if (stampQuery1 < timeEpoch) {
                                            // This (i.e. the 2nd query) has more recent result
                                            addNodeToHash(ctxt->configHash, propName, propType, 0, timeEpoch,
                                                          *valueAsString);
                                        }
                                    } else {
                                        throw KARABO_LOGIC_EXCEPTION(
                                              "Unexpected case of multiple metric retrieval for a property.");
                                    }
                                }
                            }
                        }
                    } catch (const std::exception& e) {
                        std::ostringstream oss;
                        oss << "Error retrieving value of property '" << propName << "' of type '"
                            << Types::to<ToLiteral>(propType) << "' for device '" << ctxt->deviceId << "': " << e.what()
                            << "\nRemaining property value(s) to retrieve: "
                            << ctxt->propsInfo.size() + nProps - propIdx - 1u << ".";
                        const std::string& errMsg = oss.str();
                        KARABO_LOG_FRAMEWORK_ERROR << errMsg;
                        // Go on with the remaining properties of this batch of properties.
                    }

                } // for (std::size_t propIdx; propIdx < nProps; propIdx++)
            } catch (std::exception& e) {
                std::ostringstream oss;
                oss << "Error retrieving results of queries for property batch with '" << propInfos.size()
                    << "' properties for device ' " << ctxt->deviceId << "':\n";
                for (const auto& propInfo : propInfos) {
                    oss << "\t'" << propInfo.name << "' of type '" << Types::to<ToLiteral>(propInfo.type) << "'\n";
                }
                oss << e.what() << "\n";
                const std::string& errMsg = oss.str();
                KARABO_LOG_FRAMEWORK_ERROR << errMsg;
            }

            if (ctxt->propsInfo.size() > 0) {
                // There is at least one more property whose value should be retrieved.
                asyncPropValueBeforeTime(ctxt);
            } else {
                // All properties have been retrieved. Reply to the slot caller.
                bool configAtTimePoint = ctxt->lastLogoutBeforeTime < ctxt->lastLoginBeforeTime;
                ctxt->aReply(ctxt->configHash, ctxt->configSchema, configAtTimePoint,
                             ctxt->configTimePoint.toIso8601Ext());
                onOk();
            }
        }


        std::string InfluxLogReader::unescapeLoggedString(const std::string& loggedStr) {
            std::string unescaped = boost::replace_all_copy(loggedStr, "\\\"", "\"");
            boost::replace_all(unescaped, "\\\\", "\\");
            boost::replace_all(unescaped, DATALOG_NEWLINE_MANGLE, "\n");
            return unescaped;
        }

        void InfluxLogReader::influxResultSetToVectorHash(const InfluxResultSet& influxResult,
                                                          std::vector<Hash>& vectHash) {
            // Finds the position of the trainId column, if it is in the result set.
            int tidCol = -1;
            auto iter = std::find(influxResult.first.begin(), influxResult.first.end(), "_tid");
            if (iter != influxResult.first.end()) {
                tidCol = std::distance(influxResult.first.begin(), iter);
            }

            // Gets the data type names of each column.
            std::vector<std::string> colTypeNames(influxResult.first.size());
            for (size_t col = 0; col < influxResult.first.size(); col++) {
                const std::string::size_type typeSeparatorPos = influxResult.first[col].rfind('-');
                if (typeSeparatorPos != std::string::npos) {
                    const std::string typeName = influxResult.first[col].substr(typeSeparatorPos + 1);
                    colTypeNames[col] = typeName;
                }
            }

            vectHash.reserve(influxResult.second.size());
            // Converts each row of values into a Hash.
            for (const std::vector<boost::optional<std::string>>& valuesRow : influxResult.second) {
                unsigned long long time = std::stoull(*(valuesRow[0])); // This is safe as Influx always returns time.
                Epochstamp epoch(time / INFLUX_PRECISION_FACTOR,
                                 (time % INFLUX_PRECISION_FACTOR) * kFracConversionFactor);
                unsigned long long tid = 0;
                if (tidCol >= 0) {
                    if (valuesRow[tidCol]) {
                        tid = std::stoull(*(valuesRow[tidCol]));
                    } else {
                        KARABO_LOG_FRAMEWORK_WARN << "Missing train_id (_tid) for property at timestamp '" << time
                                                  << "'. '0' will be used for the train_id value.";
                    }
                }
                Hash hash;
                for (size_t col = 1; col < influxResult.first.size(); col++) {
                    if (static_cast<int>(col) == tidCol) continue; // Skips the trainId column
                    try {
                        if (!valuesRow[col]) {
                            // Skips any null value in the result set - any row returned by Influx will have at
                            // least one non-null value (maybe an empty string).
                            continue;
                        }
                        // Figure out the real Karabo type:
                        // For nan/inf floating points we added "_INF" when writing to influxDB (and stored as
                        // strings).
                        const std::string& typeNameInflux = colTypeNames[col];
                        const size_t posInf = typeNameInflux.rfind("_INF");
                        const std::string& typeName =
                              (posInf != std::string::npos && posInf == typeNameInflux.size() - 4ul
                                     ? typeNameInflux.substr(0, posInf)
                                     : typeNameInflux);

                        const Types::ReferenceType type = Types::from<FromLiteral>(typeName);
                        addNodeToHash(hash, "v", type, tid, epoch, *(valuesRow[col]));
                    } catch (const std::exception& e) {
                        KARABO_LOG_FRAMEWORK_ERROR << "Error adding node to hash:\nValue type: " << colTypeNames[col]
                                                   << "\nValue (as string): " << *(valuesRow[col])
                                                   << "\nTimestamp: " << epoch.toIso8601Ext()
                                                   << "\nError: " << e.what();
                    }
                }
                if (hash.has("v")) {
                    vectHash.push_back(std::move(hash));
                }
            }
        }

        void InfluxLogReader::addNodeToHash(karabo::util::Hash& hash, const std::string& path,
                                            const karabo::util::Types::ReferenceType& type, unsigned long long trainId,
                                            const karabo::util::Epochstamp& epoch, const std::string& valueAsString) {
            Hash::Node* node = nullptr;

            switch (type) {
                case Types::VECTOR_HASH: {
                    // Vectors of Hashes are binary serialized and then base64 encoded by the Influx Logger.
                    std::vector<unsigned char> base64Decoded;
                    base64Decode(valueAsString, base64Decoded);
                    const char* decoded = reinterpret_cast<const char*>(base64Decoded.data());
                    node = &hash.set(path, std::vector<Hash>());
                    std::vector<Hash>& value = node->getValue<std::vector<Hash>>();
                    m_hashSerializer->load(value, decoded, base64Decoded.size());
                    break;
                }
                case Types::VECTOR_STRING: {
                    // Convert value from base64 -> JSON -> vector<string> ...
                    node = &hash.set(path, std::vector<std::string>());
                    std::vector<std::string>& value = node->getValue<std::vector<std::string>>();
                    std::vector<unsigned char> decoded;
                    base64Decode(valueAsString, decoded);
                    nl::json j = nl::json::parse(decoded.begin(), decoded.end());
                    for (nl::json::iterator ii = j.begin(); ii != j.end(); ++ii) {
                        value.push_back(*ii);
                    }
                    break;
                }
                case Types::VECTOR_CHAR: {
                    node = &hash.set(path, std::vector<char>());
                    std::vector<char>& value = node->getValue<std::vector<char>>();
                    base64Decode(valueAsString, *reinterpret_cast<std::vector<unsigned char>*>(&value));
                    break;
                }
                case Types::CHAR: {
                    std::vector<unsigned char> decoded;
                    base64Decode(valueAsString, decoded);
                    if (decoded.size() != 1ul) {
                        throw KARABO_PARAMETER_EXCEPTION("Base64 Encoded char of wrong size: " +
                                                         karabo::util::toString(decoded.size()));
                    }
                    node = &hash.set(path, static_cast<char>(decoded[0]));
                    break;
                }
                case Types::VECTOR_UINT8: {
                    // The fromString specialisation for vector<unsigned char> as used in the HANDLE_VECTOR_TYPE
                    // below erroneously does base64 decoding. We do not dare to fix that now, but workaround it
                    // here:
                    node = &hash.set(path, std::vector<unsigned char>());
                    node->getValue<std::vector<unsigned char>>() =
                          fromStringForSchemaOptions<unsigned char>(valueAsString, ",");
                    break;
                }
#define HANDLE_VECTOR_TYPE(VectorType, ElementType)                                      \
    case Types::VectorType: {                                                            \
        node = &hash.set(path, std::vector<ElementType>());                              \
        std::vector<ElementType>& value = node->getValue<std::vector<ElementType>>();    \
        if (!valueAsString.empty()) {                                                    \
            value = std::move(fromString<ElementType, std::vector>(valueAsString, ",")); \
        }                                                                                \
        break;                                                                           \
    }

                    HANDLE_VECTOR_TYPE(VECTOR_BOOL, bool);
                    HANDLE_VECTOR_TYPE(VECTOR_INT8, signed char);
                    HANDLE_VECTOR_TYPE(VECTOR_INT16, short);
                    HANDLE_VECTOR_TYPE(VECTOR_UINT16, unsigned short);
                    HANDLE_VECTOR_TYPE(VECTOR_INT32, int);
                    HANDLE_VECTOR_TYPE(VECTOR_UINT32, unsigned int);
                    HANDLE_VECTOR_TYPE(VECTOR_INT64, long long);
                    HANDLE_VECTOR_TYPE(VECTOR_UINT64, unsigned long long);
                    HANDLE_VECTOR_TYPE(VECTOR_FLOAT, float);
                    HANDLE_VECTOR_TYPE(VECTOR_DOUBLE, double);
                    HANDLE_VECTOR_TYPE(VECTOR_COMPLEX_FLOAT, std::complex<float>);
                    HANDLE_VECTOR_TYPE(VECTOR_COMPLEX_DOUBLE, std::complex<double>);
#undef HANDLE_VECTOR_TYPE
                case Types::STRING: {
                    std::string unescaped = unescapeLoggedString(valueAsString);
                    node = &hash.set<std::string>(path, unescaped);
                    break;
                }
                case Types::UINT64: {
                    // behavior on simple casting is implementation defined. We memcpy instead to be sure of the
                    // results
                    unsigned long long uv;
                    long long sv = std::move(fromString<long long>(valueAsString));
                    memcpy(&uv, &sv, sizeof(unsigned long long));
                    node = &hash.set<unsigned long long>(path, uv);
                    node->setType(type);
                }
                default: {
                    node = &hash.set<std::string>(path, valueAsString);
                    node->setType(type);
                }
            }
            Hash::Attributes& attrs = node->getAttributes();
            Timestamp(epoch, trainId).toHashAttributes(attrs);
        }


        void InfluxLogReader::slotGetBadData(const std::string& fromStr, const std::string& toStr) {
            const Epochstamp from(fromStr);
            const Epochstamp to(toStr);

            SignalSlotable::AsyncReply aReply(this);

            Hash config = buildInfluxClientConfig(m_urlConfigSchema);
            karabo::net::InfluxDbClient::Pointer influxClient =
                  Configurator<InfluxDbClient>::create("InfluxDbClient", config);

            std::ostringstream query;
            query << "SELECT * FROM \"__BAD__DATA__\" WHERE time >= " << epochAsMicrosecString(from) << m_durationUnit
                  << " AND time <= " << epochAsMicrosecString(to) << m_durationUnit;
            const std::string queryStr = query.str();
            try {
                // Not a priory clear which client to use. Since this slot is called so rarely, dare to use the one
                // querying the database with the typically longer retention policy.
                // Shared pointer to InfluxDbClient is not used by handler, but needs to be passed to guarantee that the
                // InfluxDbClient will live long enough to fulfill the query.
                influxClient->queryDb(queryStr,
                                      bind_weak(&InfluxLogReader::onGetBadData, this, _1, aReply, influxClient));
            } catch (const std::exception& e) {
                std::string details(e.what());
                std::string errMsg("Error querying for bad data");
                KARABO_LOG_FRAMEWORK_ERROR << errMsg << ": " << details;
                // In the thread where AsyncReply was created we must not use it, so post:
                std::weak_ptr<karabo::xms::SignalSlotable> weakThis(weak_from_this());
                EventLoop::getIOService().post([weakThis, errMsg, details, aReply]() {
                    std::shared_ptr<karabo::xms::SignalSlotable> ptr(weakThis.lock());
                    if (ptr) {                         // Cannot use AsyncReply if its SignalSlotable is dying/dead
                        aReply.error(errMsg, details); // 2.13.X: aReply.error(errMsg + ": " + details);
                    }
                });
            }
        }


        void InfluxLogReader::onGetBadData(const HttpResponse& response, SignalSlotable::AsyncReply aReply,
                                           const karabo::net::InfluxDbClient::Pointer& /* influxClient */) {
            if (preHandleHttpResponse(response, aReply)) {
                // Nothing left to do, failure is already replied.
                return;
            }

            try {
                InfluxResultSet influxResult;
                jsonResultsToInfluxResultSet(response.payload, influxResult, "");
                Hash result;

                const std::vector<std::string>& deviceIds = influxResult.first;
                for (size_t i = 1; i < deviceIds.size(); ++i) { // but index 0 is "time"
                    result.set(deviceIds[i], std::vector<Hash>());
                }

                // Converts each row of values into a Hash.
                for (const std::vector<boost::optional<std::string>>& valuesRow : influxResult.second) {
                    const unsigned long long time =
                          std::stoull(*(valuesRow[0])); // This is safe as Influx always returns time.
                    const Epochstamp epoch(toEpoch(time));
                    const std::string epochStr(epoch.toIso8601Ext());
                    for (size_t i = 1; i < valuesRow.size(); ++i) {
                        if (valuesRow[i]) {
                            Hash h("info", std::move(*(valuesRow[i])));
                            Hash::Node& node = h.set("time", epochStr);
                            epoch.toHashAttributes(node.getAttributes());
                            result.get<std::vector<Hash>>(deviceIds[i]).push_back(std::move(h));
                        }
                    }
                }
                // Filters out all devices that had no bad data in the requested interval.
                // Skips first element because it is the "time" column from the InfluxSet, not
                // a deviceId.
                for (size_t i = 1; i < deviceIds.size(); i++) {
                    if (result.get<std::vector<Hash>>(deviceIds[i]).empty()) {
                        result.erase(deviceIds[i]);
                    }
                }
                aReply(result);
                onOk();

            } catch (const std::exception& e) {
                std::string details(e.what());
                std::string errMsg("Error unpacking retrieved bad data info");
                KARABO_LOG_FRAMEWORK_ERROR << errMsg << ": " << details;
                aReply.error(errMsg, details); // 2.13.X: aReply.error(errMsg + ": " + details);
            }
        }

        bool InfluxLogReader::preHandleHttpResponse(const karabo::net::HttpResponse& httpResponse,
                                                    const karabo::xms::SignalSlotable::AsyncReply& asyncReply) {
            bool fullyHandled = false;
            const State currentState(get<State>("state"));

            if (httpResponse.code >= 300) {
                // Some error happened while processing the request.
                std::ostringstream errMsg;
                errMsg << "InfluxDb response status code: " << httpResponse.code << ". ";
                if (httpResponse.payload.empty() && httpResponse.message.empty()) {
                    errMsg << "Description: Could not process request.";
                } else {
                    errMsg << "Response payload: " << httpResponse.payload
                           << "\nResponse message: " << httpResponse.message;
                }
                asyncReply.error(errMsg.str());

                if (httpResponse.code == 503) {
                    // A 503 reply from the InfluxDbClient (or even one originated from Influx
                    // and propagated by the InfluxDbClient) can be interpreted as server not
                    // available and should put the log reader in ERROR state (if not already).
                    if (currentState != State::ERROR) {
                        updateState(State::ERROR, Hash("status", "Influx server not available"));
                    }
                } else {
                    // Any other status code means the server was responsive. The log reader should
                    // go to ON state (if not already).
                    if (currentState != State::ON) {
                        updateState(State::ON, Hash("status", std::string()));
                    }
                }

                fullyHandled = true;
            } else {
                // For status codes that don't indicate errors while processing the request,
                // the state of the log reader should be ON.
                if (currentState != State::ON) {
                    updateState(State::ON, Hash("status", std::string()));
                }
            }

            return fullyHandled;
        }

        karabo::util::Epochstamp InfluxLogReader::toEpoch(unsigned long long timeFromInflux) const {
            const unsigned long long timeSec = timeFromInflux / INFLUX_PRECISION_FACTOR;
            const unsigned long long timeFrac = (timeFromInflux % INFLUX_PRECISION_FACTOR) * kFracConversionFactor;
            return Epochstamp(timeSec, timeFrac);
        }

    } // namespace devices

} // namespace karabo
