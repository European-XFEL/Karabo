#include <iostream>
#include "FileDataLogger.hh"


namespace karabo {
    namespace devices {
        
        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace karabo::xms;

        KARABO_REGISTER_IN_FACTORY_1(karabo::devices::DeviceData, karabo::devices::FileDeviceData, karabo::util::Hash)

        FileDeviceData::FileDeviceData(const karabo::util::Hash& input)
            : DeviceData(input)
            , m_currentSchema()
            , m_configStream()
            , m_lastIndex(0u)
            , m_user(".") //TODO:  Define proper user running a device. The dot is unknown user?
            , m_lastTimestampMutex()
            , m_lastDataTimestamp(Epochstamp(0ull, 0ull), Trainstamp())
            , m_updatedLastTimestamp(false)
            , m_pendingLogin(true)
            , m_idxMap()
            , m_idxprops()
            , m_propsize(0u)
            , m_lasttime(0)
            , m_serializer(TextSerializer<Hash>::create(Hash("Xml.indentation", -1))) {
        }


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
                    boost::mutex::scoped_lock lock(m_lastTimestampMutex);
                    karabo::util::Timestamp& lastTs = m_lastDataTimestamp;
                    m_configStream << lastTs.toIso8601Ext() << "|" << fixed << lastTs.toTimestamp()
                            << "|" << lastTs.getTrainId() << "|.|||" << m_user << "|LOGOUT\n";
                    m_configStream.flush();
                    std::ostream::pos_type position = m_configStream.tellp();
                    m_configStream.close();
                    if (position >= 0) {
                        string contentPath = m_directory + "/" + deviceId + "/raw/archive_index.txt";
                        ofstream contentStream(contentPath.c_str(), ios::app);
                        contentStream << "-LOG " << lastTs.toIso8601Ext() << " " << fixed << lastTs.toTimestamp()
                                << " " << lastTs.getTrainId() << " " << position << " "
                                << (m_user.empty() ? "." : m_user) << " " << m_lastIndex << "\n";
                        contentStream.close();
                    } else {
                        KARABO_LOG_FRAMEWORK_ERROR << "Error retrieving position of LOGOUT entry in archive with index '"
                                << m_lastIndex << "': skipped writing index entry for " << deviceId;
                    }

                    for (map<string, MetaData::Pointer>::iterator it = m_idxMap.begin(); it != m_idxMap.end(); it++) {
                        MetaData::Pointer mdp = it->second;
                        if (mdp && mdp->idxStream.is_open()) mdp->idxStream.close();
                    }
                    m_idxMap.clear();
                }
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_LOGIC_EXCEPTION("Problems tagging " + deviceId + " to be discontinued"));
            }
        }


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, DataLogger, FileDataLogger)


        DeviceData::Pointer FileDataLogger::createDeviceData(const karabo::util::Hash& config) {
            return Factory<karabo::devices::DeviceData>::create<karabo::util::Hash>("FileDataLoggerDeviceData", config);
        }


        FileDataLogger::FileDataLogger(const karabo::util::Hash& input) : DataLogger(input) {
        }


        FileDataLogger::~FileDataLogger() {}

        void FileDataLogger::setupDirectory(const DeviceData::Pointer& devicedata) {
            FileDeviceData::Pointer data = boost::static_pointer_cast<FileDeviceData>(devicedata);
            boost::system::error_code ec;
            if (!boost::filesystem::exists(get<string>("directory") + "/" + data->m_deviceToBeLogged)) {
                boost::filesystem::create_directories(get<string>("directory") + "/" + data->m_deviceToBeLogged, ec);
                if (ec) {
                    const std::string msg("Failed to create directories : " + data->m_deviceToBeLogged + ". code = "
                                          + toString(ec.value()) += " -- " + ec.message());
                    KARABO_LOG_FRAMEWORK_ERROR << msg;
                    throw KARABO_INIT_EXCEPTION(msg);
                }
            }
            if (!boost::filesystem::exists(get<string>("directory") + "/" + data->m_deviceToBeLogged + "/raw")) {
                boost::filesystem::create_directory(get<string>("directory") + "/" + data->m_deviceToBeLogged + "/raw");
            }
            if (!boost::filesystem::exists(get<string>("directory") + "/" + data->m_deviceToBeLogged + "/idx")) {
                boost::filesystem::create_directory(get<string>("directory") + "/" + data->m_deviceToBeLogged + "/idx");
            }

            data->m_lastIndex = determineLastIndex(data->m_deviceToBeLogged);
        }


        void FileDataLogger::handleChanged(const karabo::util::Hash& configuration, const std::string& user,
                                           const DeviceData::Pointer& devicedata) {

            FileDeviceData::Pointer data = boost::static_pointer_cast<FileDeviceData>(devicedata);
            data->m_user = user; // set under m_strand protection
            const std::string& deviceId = data->m_deviceToBeLogged;

            const bool newPropToIndex = this->updatePropsToIndex(*data);

            // TODO: Define these variables: each of them uses 24 bits
            int expNum = 0x0F0A1A2A;
            int runNum = 0x0F0B1B2B;

            // To write log I need schema - but that has arrived before connecting signal[State]Changed to slotChanged
            // and thus before any data can arrive here in handleChanged.
            vector<string> paths;
            getPathsForConfiguration(configuration, data->m_currentSchema, paths);

            if (newPropToIndex) {
                // DataLogReader got request for history of a property not indexed
                // so far, that means it triggered the creation of an index file
                // for that property. Since we cannot be sure that the index creation
                // has finished when we want to add a new index entry, we close the
                // file and thus won't touch this new index file (but start a new one).
                this->ensureFileClosed(*data);
            }

            for (size_t i = 0; i < paths.size(); ++i) {
                const string& path = paths[i];
                
                // Skip those elements which should not be archived
                const bool noArchive = (!data->m_currentSchema.has(path)
                                        || (data->m_currentSchema.hasArchivePolicy(path)
                                            && (data->m_currentSchema.getArchivePolicy(path) == Schema::NO_ARCHIVING)));

                const Hash::Node& leafNode = configuration.getNode(path);

                // Check for timestamp ...
                if (!Timestamp::hashAttributesContainTimeInformation(leafNode.getAttributes())) {
                    if (!noArchive) { // Lack of timestamp for non-archived properties does not harm logging
                        KARABO_LOG_WARN << "Skip '" << path << "' of '" << deviceId
                                << "' - it lacks time information attributes.";
                    }
                    continue;
                }

                Timestamp t = Timestamp::fromHashAttributes(leafNode.getAttributes());
                {
                    // Update time stamp for DeviceData destructor and property "lastUpdatesUtc".
                    // Since the latter is accessing it when not posted on data->m_strand, need mutex protection:
                    boost::mutex::scoped_lock lock(data->m_lastTimestampMutex);
                    if (t.getEpochstamp() > data->m_lastDataTimestamp.getEpochstamp()) {
                        // If mixed timestamps in single message (or arrival in wrong order), always take most recent one.
                        data->m_updatedLastTimestamp = true;
                        data->m_lastDataTimestamp = t;
                    }
                }

                if (noArchive) continue; // Bail out after updating time stamp!
                string value; // "value" should be a string, so convert depending on type ...
                if (leafNode.getType() == Types::VECTOR_HASH) {
                    // Represent any vector<Hash> as XML string ...
                    data->m_serializer->save(leafNode.getValue<vector < Hash >> (), value);
                    boost::algorithm::replace_all(value, "\n", karabo::util::DATALOG_NEWLINE_MANGLE);
                } else if (Types::isVector(leafNode.getType())) {
                    // ... and any other vector as a comma separated text string of vector elements
                    value = toString(leafNode.getValueAs<string, vector>());
                    if (leafNode.getType() == Types::VECTOR_STRING) {
                        // Line breaks in content confuse indexing and reading back - so better mangle strings... :-(.
                        boost::algorithm::replace_all(value, "\n", karabo::util::DATALOG_NEWLINE_MANGLE);
                    }
                } else {
                    value = leafNode.getValueAs<string>();
                    if (leafNode.getType() == Types::STRING) {
                        // Line breaks in content confuse indexing and reading back - so better mangle strings... :-(.
                        boost::algorithm::replace_all(value, "\n", karabo::util::DATALOG_NEWLINE_MANGLE);
                    }
                }

                bool newFile = false;
                if (!data->m_configStream.is_open()) {
                    string configName = get<string>("directory") + "/" + deviceId + "/raw/archive_" + toString(data->m_lastIndex) + ".txt";
                    data->m_configStream.open(configName.c_str(), ios::out | ios::app);
                    if (!data->m_configStream.is_open()) {
                        KARABO_LOG_ERROR << "Failed to open \"" << configName << "\". Check permissions.";
                        return;
                    }
                    if (data->m_configStream.tellp() > 0) {
                        // Make sure that the file contains '\n' (newline) at the end of previous round
                        data->m_configStream << '\n';
                    } else
                        newFile = true;
                }

                size_t position = data->m_configStream.tellp(); // get current file size
                const string type = Types::to<ToLiteral>(leafNode.getType());
                data->m_configStream << t.toIso8601Ext() << "|" << fixed << t.toTimestamp() << "|"
                        << t.getTrainId() << "|" << path << "|" << type << "|" << scientific
                        << value << "|" << data->m_user;
                if (data->m_pendingLogin) data->m_configStream << "|LOGIN\n";
                else data->m_configStream << "|VALID\n";

                if (data->m_pendingLogin || newFile) {
                    string contentPath = get<string>("directory") + "/" + deviceId + "/raw/archive_index.txt";
                    ofstream contentStream(contentPath.c_str(), ios::app);
                    if (data->m_pendingLogin) {
                        contentStream << "+LOG ";
                    } else {
                        contentStream << "=NEW ";
                    }

                    contentStream << t.toIso8601Ext() << " " << fixed << t.toTimestamp() << " "
                            << t.getTrainId() << " " << position << " " << (data->m_user.empty() ? "." : data->m_user) << " " << data->m_lastIndex << "\n";
                    contentStream.close();

                    data->m_pendingLogin = false;
                }

                // check if we have property registered
                if (find(data->m_idxprops.begin(), data->m_idxprops.end(), path) == data->m_idxprops.end()) continue;

                MetaData::Pointer& mdp = data->m_idxMap[path]; //Pointer by reference!
                bool first = false;
                if (!mdp) {
                    // a property not yet indexed - create meta data and set file
                    mdp = MetaData::Pointer(new MetaData);
                    mdp->idxFile = get<string>("directory") + "/" + deviceId + "/idx/archive_" + toString(data->m_lastIndex)
                            + "-" + path + "-index.bin";
                    first = true;
                }
                if (!mdp->idxStream.is_open()) {
                    mdp->idxStream.open(mdp->idxFile.c_str(), ios::out | ios::app | ios::binary);
                }
                mdp->record.epochstamp = t.toTimestamp();
                mdp->record.trainId = t.getTrainId();
                mdp->record.positionInRaw = position;
                mdp->record.extent1 = (expNum & 0xFFFFFF);
                mdp->record.extent2 = (runNum & 0xFFFFFF);
                if (first) {
                    mdp->record.extent2 |= (1 << 30);
                }
                mdp->idxStream.write((char*) &mdp->record, sizeof (MetaData::Record));
            }

            long maxFilesize = get<int>("maximumFileSize") * 1000000; // times to 1000000 because maximumFilesSize in MBytes
            long position = data->m_configStream.tellp();
            if (maxFilesize <= position) {
                this->ensureFileClosed(*data);
            }
        }


        bool FileDataLogger::updatePropsToIndex(FileDeviceData& data) {
            const std::string& deviceId = data.m_deviceToBeLogged;
            boost::filesystem::path propPath(get<string>("directory") + "/" + deviceId + "/raw/properties_with_index.txt");
            if (boost::filesystem::exists(propPath)) {
                const size_t propsize = boost::filesystem::file_size(propPath);
                const time_t lasttime = boost::filesystem::last_write_time(propPath);
                // read prop file only if it was changed
                if (data.m_propsize != propsize || data.m_lasttime != lasttime) {
                    data.m_propsize = propsize;
                    data.m_lasttime = lasttime;
                    // re-read prop file
                    ifstream in(propPath.c_str());
                    string content(propsize, ' ');
                    content.assign((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
                    in.close();
                    data.m_idxprops.clear();
                    boost::split(data.m_idxprops, content, boost::is_any_of("\n"));
                    // Could do more clever gymnastics to check whether content of
                    // data.m_idxprops now really differs from old content...
                    return true;
                }
            }
            return false;
        }


        void FileDataLogger::ensureFileClosed(FileDeviceData& data) {
            const std::string& deviceId = data.m_deviceToBeLogged;
            if (data.m_configStream.is_open()) {
                // increment index number for configuration file
                data.m_lastIndex = this->incrementLastIndex(deviceId);
                data.m_configStream.close();
            }

            for (std::map<std::string, MetaData::Pointer>::iterator it = data.m_idxMap.begin(), endIt = data.m_idxMap.end();
                 it != endIt; ++it) {
                MetaData::Pointer mdp = it->second;
                if (mdp && mdp->idxStream.is_open()) mdp->idxStream.close();
            }
            data.m_idxMap.clear();
        }


        void FileDataLogger::doFlush() {

            // Flush all files, but also hijack this actor to update lastUpdatesUtc
            std::vector<Hash> lastStamps;
            bool updatedAnyStamp = false;
            {
                boost::mutex::scoped_lock lock(m_perDeviceDataMutex);
                lastStamps.reserve(m_perDeviceData.size());
                for (auto& idData : m_perDeviceData) {
                    FileDeviceData::Pointer data = boost::static_pointer_cast<FileDeviceData>(idData.second);
                    {
                        // To avoid this mutex lock, access to m_lastTimestampMutex would have to be posted on m_strand.
                        // Keep as is since this file based DataLogger is supposed to phase out soon...
                        boost::mutex::scoped_lock lock(data->m_lastTimestampMutex);
                        updatedAnyStamp |= data->m_updatedLastTimestamp;
                        data->m_updatedLastTimestamp = false;
                        const karabo::util::Timestamp& ts = data->m_lastDataTimestamp;
                        Hash h("deviceId", idData.first);
                        // Human readable Epochstamp (except if no updates yet), attributes for machines
                        Hash::Node& node = h.set("lastUpdateUtc", "");
                        if (ts.getSeconds() != 0ull) {
                            node.setValue(ts.toFormattedString()); //"%Y%m%dT%H%M%S"));
                        }
                        ts.getEpochstamp().toHashAttributes(node.getAttributes());
                        lastStamps.push_back(std::move(h));
                    }

                    // We post on strand to exclude parallel access to data->m_configStream and data->m_idxMap
                    data->m_strand->post(karabo::util::bind_weak(&FileDataLogger::flushOne, this, data));
                }
            }

            if (updatedAnyStamp
                || (lastStamps.size() != get<std::vector < Hash >> ("lastUpdatesUtc").size())) {
                // If sizes are equal, but devices have changed, then at least one time stamp must have changed as well.
                set("lastUpdatesUtc", lastStamps);
            }
        }


        void FileDataLogger::flushOne(const FileDeviceData::Pointer& data) {
            if (data->m_configStream.is_open()) {
                data->m_configStream.flush();
            }
            for (std::map<string, MetaData::Pointer>::iterator it = data->m_idxMap.begin(); it != data->m_idxMap.end(); ++it) {
                MetaData::Pointer mdp = it->second;
                if (mdp && mdp->idxStream.is_open()) mdp->idxStream.flush();
            }
        }


        int FileDataLogger::determineLastIndex(const std::string& deviceId) const {
            string lastIndexFilename = get<string>("directory") + "/" + deviceId + "/raw/archive.last";
            int idx;
            fstream fs;
            if (!boost::filesystem::exists(lastIndexFilename)) {
                for (size_t i = 0;; i++) {
                    string filename = get<string>("directory") + "/" + deviceId + "/raw/archive_" + toString(i) + ".txt";
                    if (!boost::filesystem::exists(filename)) {
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


        int FileDataLogger::incrementLastIndex(const std::string& deviceId) {
            string lastIndexFilename = get<string>("directory") + "/" + deviceId + "/raw/archive.last";
            int idx;
            if (!boost::filesystem::exists(lastIndexFilename)) {
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


        void FileDataLogger::handleSchemaUpdated(const karabo::util::Schema& schema, const DeviceData::Pointer& devicedata) {
            const FileDeviceData::Pointer& data = boost::static_pointer_cast<FileDeviceData>(devicedata);

            const std::string& deviceId = data->m_deviceToBeLogged;

            data->m_currentSchema = schema;

            string filename = get<string>("directory") + "/" + deviceId + "/raw/archive_schema.txt";
            fstream fileout(filename.c_str(), ios::out | ios::app);
            if (fileout.is_open()) {
                Timestamp t;
                // Since schema updates are rare, do not store this serialiser as the one for Hash (data->m_serializer):
                TextSerializer<Schema>::Pointer serializer = TextSerializer<Schema>::create(Hash("Xml.indentation", -1));
                string archive;
                serializer->save(schema, archive);
                fileout << t.getSeconds() << " " << t.getFractionalSeconds() << " " << t.getTrainId() << " " << archive << "\n";
                fileout.close();
            } else
                throw KARABO_IO_EXCEPTION("Failed to open \"" + filename + "\". Check permissions.");
        }
    }
}
