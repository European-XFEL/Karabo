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
#include "FileDataLogger.hh"

#include <chrono>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>


KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device, karabo::devices::DataLogger,
                                  karabo::devices::FileDataLogger)
KARABO_REGISTER_IN_FACTORY_1(karabo::devices::DeviceData, karabo::devices::FileDeviceData, karabo::util::Hash)

namespace karabo {
    namespace devices {

        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace karabo::xms;
        using json = nlohmann::json;
        using namespace std::chrono_literals;


        FileDeviceData::FileDeviceData(const karabo::util::Hash& input)
            : DeviceData(input),
              m_directory(input.get<std::string>("directory")),
              m_maxFileSize(input.get<int>("maximumFileSize")),
              m_configStream(),
              m_lastIndex(0u),
              m_idxMap(),
              m_idxprops(),
              m_propsize(0u),
              m_lasttime(0h),
              m_serializer(TextSerializer<Hash>::create(Hash("Xml.indentation", -1))) {}


        FileDeviceData::~FileDeviceData() {
            if (m_initLevel != InitLevel::COMPLETE) {
                // We have not yet started logging this device, so nothing to mark about being done.
                return;
            }

            const std::string& deviceId = m_deviceToBeLogged;
            // Mark as logger stopped.
            // Although this destructor is not running on the strand, accessing all members is safe:
            // All other actions touching the members are posted on the strand and have a shared  pointer
            // to the DeviceData - so this destructor can only run when all these actions are done.
            try {
                if (m_configStream.is_open()) {
                    // Timestamp shall be the one of the most recent update - this ensures that all stamps come from
                    // the device and cannot be screwed up if clocks of logger and device are off from each other.
                    // Since the time when logging stops might be of interest as well (for silent devices), we add it to
                    // value field
                    std::lock_guard<std::mutex> lock(m_lastTimestampMutex);
                    karabo::util::Timestamp& lastTs = m_lastDataTimestamp;
                    m_configStream << lastTs.toIso8601Ext() << "|" << fixed << lastTs.toTimestamp() << "|"
                                   << lastTs.getTrainId() << "|.||"
                                   << karabo::util::Timestamp().toIso8601Ext() // i.e. 'now' from clock of logger
                                   << "|" << m_user << "|LOGOUT\n";
                    m_configStream.flush();
                    std::ostream::pos_type position = m_configStream.tellp();
                    m_configStream.close();
                    if (position >= 0) {
                        string contentPath = m_directory + "/" + deviceId + "/raw/archive_index.txt";
                        ofstream contentStream(contentPath.c_str(), ios::app);
                        // Again use timestamp from device to ensure consistency for searching in archive_index.txt
                        contentStream << "-LOG " << lastTs.toIso8601Ext() << " " << fixed << lastTs.toTimestamp() << " "
                                      << lastTs.getTrainId() << " " << position << " "
                                      << (m_user.empty() ? "." : m_user) << " " << m_lastIndex << "\n";
                        contentStream.close();
                    } else {
                        KARABO_LOG_FRAMEWORK_ERROR
                              << "Error retrieving position of LOGOUT entry in archive with index '" << m_lastIndex
                              << "': skipped writing index entry for " << deviceId;
                    }

                    for (map<string, MetaData::Pointer>::iterator it = m_idxMap.begin(); it != m_idxMap.end(); ++it) {
                        MetaData::Pointer mdp = it->second;
                        if (mdp && mdp->idxStream.is_open()) mdp->idxStream.close();
                    }
                    m_idxMap.clear();
                }
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_LOGIC_EXCEPTION("Problems tagging " + deviceId + " to be discontinued"));
            }
        }


        void FileDataLogger::expectedParameters(karabo::util::Schema& expected) {
            STRING_ELEMENT(expected)
                  .key("directory")
                  .displayedName("Directory")
                  .description("The directory where the log files should be placed")
                  .assignmentOptional()
                  .defaultValue("karaboHistory")
                  .commit();

            INT32_ELEMENT(expected)
                  .key("maximumFileSize")
                  .displayedName("Maximum file size")
                  .description(
                        "After any archived file has reached this size it will be time-stamped and not appended "
                        "anymore")
                  .unit(Unit::BYTE)
                  .metricPrefix(MetricPrefix::MEGA)
                  .assignmentOptional()
                  .defaultValue(100)
                  .commit();
        }


        DeviceData::Pointer FileDataLogger::createDeviceData(const karabo::util::Hash& cfg) {
            Hash config = cfg;
            config.set("directory", get<std::string>("directory"));
            config.set("maximumFileSize", get<int>("maximumFileSize"));
            DeviceData::Pointer devicedata =
                  Factory<karabo::devices::DeviceData>::create<karabo::util::Hash>("FileDataLoggerDeviceData", config);
            FileDeviceData::Pointer data = std::static_pointer_cast<FileDeviceData>(devicedata);
            data->setupDirectory();
            return devicedata;
        }


        FileDataLogger::FileDataLogger(const karabo::util::Hash& input) : DataLogger(input) {}


        FileDataLogger::~FileDataLogger() {}


        void FileDataLogger::flushImpl(const std::shared_ptr<SignalSlotable::AsyncReply>& aReplyPtr) {
            // We loop on all m_perDeviceData - their flushOne() method needs to run on the strand.
            // If a reply is needed, we have to instruct the handler to use it if all are ready.

            // Setup all variables needed for sending reply
            std::shared_ptr<std::pair<std::mutex, std::vector<bool>>> fencePtr;
            std::function<void(const FileDeviceData::Pointer&, size_t)> callback;
            std::lock_guard<std::mutex> lock(m_perDeviceDataMutex);
            if (aReplyPtr) {
                fencePtr = std::make_shared<std::pair<std::mutex, std::vector<bool>>>();
                fencePtr->second.resize(m_perDeviceData.size(), false);
                callback = [aReplyPtr, fencePtr](const FileDeviceData::Pointer& data, size_t num) -> void {
                    data->flushOne();

                    std::lock_guard<std::mutex> lock(fencePtr->first);
                    fencePtr->second[num] = true;
                    for (bool ok : fencePtr->second) {
                        if (!ok) return; // not all done yet, nothing to do
                    }
                    // Also last flushOne is done, report that flush has finished
                    (*aReplyPtr)();
                };
            }
            // Actually loop on deviceData
            size_t counter = 0;
            for (auto& idData : m_perDeviceData) {
                FileDeviceData::Pointer data = std::static_pointer_cast<FileDeviceData>(idData.second);
                // We post on strand to exclude parallel access to data->m_configStream and data->m_idxMap.
                if (aReplyPtr) {
                    // Bind the shared_ptr data to ensure that reply is given, even if logging is stopped
                    data->m_strand->post(std::bind(callback, data, counter));
                    ++counter;
                } else {
                    // Here we could use bind_weak - but FileDeviceData does not inherit from enable_shared_from_this...
                    data->m_strand->post(std::bind(&FileDeviceData::flushOne, data));
                }
            }
        }


        void FileDeviceData::setupDirectory() {
            boost::system::error_code ec;
            const std::string fullDir(m_directory + "/" + m_deviceToBeLogged);
            if (!std::filesystem::exists(fullDir)) {
                std::filesystem::create_directories(fullDir, ec);
                if (ec) {
                    const std::string msg("Failed to create directories : " + fullDir +
                                                ". code = " + toString(ec.value()) += " -- " + ec.message());
                    KARABO_LOG_FRAMEWORK_ERROR << msg;
                    throw KARABO_INIT_EXCEPTION(msg);
                }
            }
            if (!std::filesystem::exists(fullDir + "/raw")) {
                std::filesystem::create_directory(fullDir + "/raw");
            }
            if (!std::filesystem::exists(fullDir + "/idx")) {
                std::filesystem::create_directory(fullDir + "/idx");
            }

            m_lastIndex = determineLastIndex(m_deviceToBeLogged);
        }


        void FileDeviceData::handleChanged(const karabo::util::Hash& configuration, const std::string& user) {
            m_user = user; // set under m_strand protection
            const std::string& deviceId = m_deviceToBeLogged;

            const bool newPropToIndex = this->updatePropsToIndex();

            // To write log I need schema - but that has arrived before connecting signal[State]Changed to slotChanged
            // and thus before any data can arrive here in handleChanged.
            vector<string> paths;
            getPathsForConfiguration(configuration, m_currentSchema, paths);

            if (newPropToIndex) {
                // DataLogReader got request for history of a property not indexed
                // so far, that means it triggered the creation of an index file
                // for that property. Since we cannot be sure that the index creation
                // has finished when we want to add a new index entry, we close the
                // file and thus won't touch this new index file (but start a new one).
                this->ensureFileClosed();
            }

            for (size_t i = 0; i < paths.size(); ++i) {
                const string& path = paths[i];

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

                Timestamp t = Timestamp::fromHashAttributes(leafNode.getAttributes());
                {
                    // Update time stamp for updates of property "lastUpdatesUtc" and for LOGOUT timestamp.
                    // Since for "lastUpdatesUtc" it is accessed when not posted on m_strand, need mutex protection:
                    std::lock_guard<std::mutex> lock(m_lastTimestampMutex);
                    if (t.getEpochstamp() > m_lastDataTimestamp.getEpochstamp()) {
                        // If mixed timestamps in single message (or arrival in wrong order), always take most recent
                        // one.
                        m_updatedLastTimestamp = true;
                        m_lastDataTimestamp = t;
                    }
                }

                if (noArchive) continue; // Bail out after updating time stamp!
                string value;            // "value" should be a string, so convert depending on type ...
                string typeString = Types::to<ToLiteral>(leafNode.getType());

                if (leafNode.getType() == Types::VECTOR_HASH) {
                    // Represent any vector<Hash> as XML string ...
                    m_serializer->save(leafNode.getValue<vector<Hash>>(), value);
                    boost::algorithm::replace_all(value, "\n", karabo::util::DATALOG_NEWLINE_MANGLE);
                } else if (Types::isVector(leafNode.getType())) {
                    // ... and any other vector as a comma separated text string of vector elements
                    value = toString(leafNode.getValueAs<string, vector>());
                    if (leafNode.getType() == Types::VECTOR_STRING) {
                        // New format: convert to JSON and then base64 ...
                        typeString = "VECTOR_STRING_BASE64"; // set artificial marker
                        const std::vector<std::string>& vecstr = leafNode.getValue<std::vector<std::string>>();
                        json j(vecstr);                   // convert to JSON
                        const std::string str = j.dump(); // JSON as a string
                        const unsigned char* encoded = reinterpret_cast<const unsigned char*>(str.c_str());
                        const size_t length = str.length();
                        value = base64Encode(encoded, length); // encode to base64
                    }
                } else {
                    value = leafNode.getValueAs<string>();
                    if (leafNode.getType() == Types::STRING) {
                        // Line breaks in content confuse indexing and reading back - so better mangle strings... :-(.
                        boost::algorithm::replace_all(value, "\n", karabo::util::DATALOG_NEWLINE_MANGLE);
                    }
                }

                const std::pair<bool, size_t> newFilePlusPosition = ensureFileOpen();
                if (newFilePlusPosition.second == -1ul) continue; // problem with file permissions, skip and go on
                logValue(deviceId, path, t, value, typeString, newFilePlusPosition.second);

                // Possibly add new line to index file:
                if (m_pendingLogin || newFilePlusPosition.first) {
                    string contentPath = m_directory + "/" + deviceId + "/raw/archive_index.txt";
                    ofstream contentStream(contentPath.c_str(), ios::app);
                    if (m_pendingLogin) {
                        contentStream << "+LOG ";
                        // TRICK: 'configuration' is the one requested at the beginning. For devices which have
                        // properties with older timestamps than the time of their instantiation (as e.g. read from
                        // hardware), we keep stamps in the archive_index.txt file sequential by overwriting here these
                        // old stamps with the most recent one ('paths' are sorted above!) which should be one of the
                        // 'Karabo only' properties like _deviceId_ etc.
                        const auto& attrsOfPathWithMostRecentStamp = configuration.getAttributes(paths.back());
                        t = Timestamp::fromHashAttributes(attrsOfPathWithMostRecentStamp);

                        m_pendingLogin = false;
                    } else {
                        contentStream << "=NEW ";
                    }
                    contentStream << t.toIso8601Ext() << " " << fixed << t.toTimestamp() << " " << t.getTrainId() << " "
                                  << newFilePlusPosition.second << " " << (m_user.empty() ? "." : m_user) << " "
                                  << m_lastIndex << "\n";
                    contentStream.close();
                }
            }

            long maxFilesize = m_maxFileSize * 1000000; // times to 1000000 because maximumFilesSize in MBytes
            long position = m_configStream.tellp();
            if (maxFilesize <= position) {
                ensureFileClosed();
            }
        }


        std::pair<bool, size_t> FileDeviceData::ensureFileOpen() {
            bool newFile = false;
            if (!m_configStream.is_open()) {
                string configName =
                      m_directory + "/" + m_deviceToBeLogged + "/raw/archive_" + toString(m_lastIndex) + ".txt";
                m_configStream.open(configName.c_str(), ios::out | ios::app);
                if (!m_configStream.is_open()) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Failed to open \"" << configName << "\". Check permissions.";
                    return std::make_pair(newFile, -1ul); // maximum unsigned long value to indicate trouble
                }
                if (m_configStream.tellp() > 0) {
                    // Make sure that the file contains '\n' (newline) at the end of previous round
                    m_configStream << '\n';
                } else {
                    newFile = true;
                }
            }
            return std::make_pair(newFile, m_configStream.tellp());
        }


        void FileDeviceData::logValue(const std::string& deviceId, const std::string& path,
                                      const karabo::util::Timestamp& ts, const std::string& value,
                                      const std::string& type, size_t filePosition) {
            m_configStream << ts.toIso8601Ext() << "|" << fixed << ts.toTimestamp() << "|" << ts.getTrainId() << "|"
                           << path << "|" << type << "|" << scientific << value << "|" << m_user;
            if (m_pendingLogin) m_configStream << "|LOGIN\n";
            else m_configStream << "|VALID\n";

            // check if we have property registered
            if (find(m_idxprops.begin(), m_idxprops.end(), path) == m_idxprops.end()) return;

            MetaData::Pointer& mdp = m_idxMap[path]; // Pointer by reference!
            bool first = false;
            if (!mdp) {
                // a property not yet indexed - create meta data and set file
                mdp = MetaData::Pointer(new MetaData);
                mdp->idxFile = m_directory + "/" + deviceId + "/idx/archive_" + toString(m_lastIndex) + "-" + path +
                               "-index.bin";
                first = true;
            }
            if (!mdp->idxStream.is_open()) {
                mdp->idxStream.open(mdp->idxFile.c_str(), ios::out | ios::app | ios::binary);
            }
            // TODO: Define these variables: each of them uses 24 bits
            int expNum = 0x0F0A1A2A;
            int runNum = 0x0F0B1B2B;

            mdp->record.epochstamp = ts.toTimestamp();
            mdp->record.trainId = ts.getTrainId();
            mdp->record.positionInRaw = filePosition;
            mdp->record.extent1 = (expNum & 0xFFFFFF);
            mdp->record.extent2 = (runNum & 0xFFFFFF);
            if (first) {
                mdp->record.extent2 |= (1 << 30);
            }
            mdp->idxStream.write((char*)&mdp->record, sizeof(MetaData::Record));
        }


        bool FileDeviceData::updatePropsToIndex() {
            const std::string& deviceId = m_deviceToBeLogged;
            std::filesystem::path propPath(m_directory + "/" + deviceId + "/raw/properties_with_index.txt");
            if (std::filesystem::exists(propPath)) {
                const size_t propsize = std::filesystem::file_size(propPath);
                const std::filesystem::file_time_type lasttime = std::filesystem::last_write_time(propPath);
                // read prop file only if it was changed
                if (m_propsize != propsize || m_lasttime != lasttime) {
                    m_propsize = propsize;
                    m_lasttime = lasttime;
                    // re-read prop file
                    ifstream in(propPath.c_str());
                    string content(propsize, ' ');
                    content.assign((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
                    in.close();
                    m_idxprops.clear();
                    boost::split(m_idxprops, content, boost::is_any_of("\n"));
                    // Could do more clever gymnastics to check whether content of
                    // data.m_idxprops now really differs from old content...
                    return true;
                }
            }
            return false;
        }


        void FileDeviceData::ensureFileClosed() {
            const std::string& deviceId = m_deviceToBeLogged;
            if (m_configStream.is_open()) {
                // increment index number for configuration file
                m_lastIndex = this->incrementLastIndex(deviceId);
                m_configStream.close();
            }

            for (std::map<std::string, MetaData::Pointer>::iterator it = m_idxMap.begin(), endIt = m_idxMap.end();
                 it != endIt; ++it) {
                MetaData::Pointer mdp = it->second;
                if (mdp && mdp->idxStream.is_open()) mdp->idxStream.close();
            }
            m_idxMap.clear();
        }


        void FileDeviceData::flushOne() {
            if (m_configStream.is_open()) {
                m_configStream.flush();
            }
            for (std::map<string, MetaData::Pointer>::iterator it = m_idxMap.begin(); it != m_idxMap.end(); ++it) {
                MetaData::Pointer mdp = it->second;
                if (mdp && mdp->idxStream.is_open()) mdp->idxStream.flush();
            }
        }


        int FileDeviceData::determineLastIndex(const std::string& deviceId) const {
            string lastIndexFilename = m_directory + "/" + deviceId + "/raw/archive.last";
            int idx;
            fstream fs;
            if (!std::filesystem::exists(lastIndexFilename)) {
                for (size_t i = 0;; i++) {
                    string filename = m_directory + "/" + deviceId + "/raw/archive_" + toString(i) + ".txt";
                    if (!std::filesystem::exists(filename)) {
                        idx = i;
                        break;
                    }
                }
                fs.open(lastIndexFilename.c_str(), ios::out | ios::app);
                fs << idx << "\n";
            } else {
                fs.open(lastIndexFilename.c_str(), ios::in);
                fs >> idx;
            }
            fs.close();
            return idx;
        }


        int FileDeviceData::incrementLastIndex(const std::string& deviceId) {
            string lastIndexFilename = m_directory + "/" + deviceId + "/raw/archive.last";
            int idx;
            if (!std::filesystem::exists(lastIndexFilename)) {
                idx = determineLastIndex(deviceId);
            }
            fstream file(lastIndexFilename.c_str(), ios::in | ios::out);
            file >> idx;
            if (file.fail()) file.clear();
            ++idx;
            file.seekg(0);
            file << idx << "\n";
            file.close();
            return idx;
        }


        void FileDeviceData::handleSchemaUpdated(const karabo::util::Schema& schema,
                                                 const karabo::util::Timestamp& stamp) {
            const std::string& deviceId = m_deviceToBeLogged;

            m_currentSchema = schema;

            string filename = m_directory + "/" + deviceId + "/raw/archive_schema.txt";
            fstream fileout(filename.c_str(), ios::out | ios::app);
            if (fileout.is_open()) {
                // Since schema updates are rare, do not store this serialiser as the one for Hash (data->m_serializer):
                TextSerializer<Schema>::Pointer serializer =
                      TextSerializer<Schema>::create(Hash("Xml.indentation", -1));
                string archive;
                try {
                    serializer->save(schema, archive);
                } catch (const std::exception& e) {
                    // Currently (2.7.0rc2), a karabo::util::NotSupportedException is thrown when the first option
                    // of a string element contains a comma. But whatever exception is thrown, we should go on,
                    // otherwise DataLogger::handleSchemaReceived2 will not connect to signal[State]Changed
                    // and thus configurations are not stored, either.
                    // Note: Do not dare to print Schema as part of log message, either...
                    KARABO_LOG_FRAMEWORK_ERROR << "Failed to serialise Schema of " << deviceId
                                               << ", store incomplete XML: " << e.what();
                }
                fileout << stamp.getSeconds() << " " << stamp.getFractionalSeconds() << " " << stamp.getTrainId() << " "
                        << archive << "\n";
                fileout.close();
            } else {
                // Should not throw, either (see above).
                KARABO_LOG_FRAMEWORK_ERROR << "Failed to open '" << filename << "'. Check permissions.";
            }
        }

    } // namespace devices
} // namespace karabo
