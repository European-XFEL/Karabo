#include <iostream>
#include "InfluxDataLogger.hh"


namespace karabo {
    namespace devices {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace karabo::xms;


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, DataLogger, InfluxDataLogger)
        KARABO_REGISTER_IN_FACTORY_1(DeviceData, InfluxDeviceData, karabo::util::Hash)

        InfluxDeviceData::InfluxDeviceData(const karabo::util::Hash& input)
            : DeviceData(input)
            , m_this(input.get<InfluxDataLogger*>("this"))
            , m_query()
            , m_serializer(TextSerializer<Hash>::create(Hash("Xml.indentation", -1))) {
        }


        InfluxDeviceData::~InfluxDeviceData() {
            if (m_initLevel != InitLevel::COMPLETE) {
                // We have not yet started logging this device, so nothing to mark about being done.
                return;
            }

            const std::string& deviceId = m_deviceToBeLogged;
            // Mark as logger stopped.
            // Although this destructor is not running on the strand, accessing all members is safe:
            // All other actions touching the members are posted on the strand and have a shared  pointer
            // to the DeviceData - so this destructor can only run when all these actions are done.
            logValue(deviceId, "log", m_lastDataTimestamp, "LOGOUT", Types::STRING);
        }


        void InfluxDataLogger::expectedParameters(karabo::util::Schema& expected) {

            STRING_ELEMENT(expected).key("url")
                    .displayedName("Influxdb URL")
                    .description("URL should be given in form: tcp://localhost:8086")
                    .assignmentMandatory()
                    .init()
                    .commit();

            UINT32_ELEMENT(expected).key("maxQuerySize")
                    .displayedName("MaxQuerySize")
                    .description("Max size of sent buffer")
                    .unit(Unit::BYTE)
                    .assignmentOptional().defaultValue(1024)
                    .init()
                    .commit();
        }


        DeviceData::Pointer InfluxDataLogger::createDeviceData(const karabo::util::Hash& cfg) {
            Hash config = cfg;
            config.set("this", this);
            return Factory<karabo::devices::DeviceData>::create<karabo::util::Hash>("InfluxDataLoggerDeviceData", config);
        }


        InfluxDataLogger::InfluxDataLogger(const karabo::util::Hash& input)
            : DataLogger(input)
            , m_url(input.get<std::string>("url"))
            , m_dbConnection()
            , m_dbChannel()
            , m_isDbConnected(false)
            , m_bufferMutex()
            , m_buffer()
            , m_bufferLen(0)
            , m_topic(getTopic()) {
        }


        InfluxDataLogger::~InfluxDataLogger() {
        }


        void InfluxDataLogger::initializeBackend(const DeviceData::Pointer& devicedata) {
            InfluxDeviceData::Pointer data = boost::static_pointer_cast<InfluxDeviceData>(devicedata);
            if (!m_isDbConnected) {
                if (!m_dbConnection) {
                    Hash config("url", m_url, "sizeofLength", 0, "type", "client");
                    m_dbConnection = karabo::net::Connection::create("Tcp", config);
                    m_dbConnection->startAsync(bind_weak(&InfluxDataLogger::onDbConnect, this, _1, _2));
                }
            }
        }


        void InfluxDataLogger::onDbConnect(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel) {
            if (ec) {
                m_isDbConnected = false;
                m_dbConnection.reset();
                KARABO_LOG_FRAMEWORK_WARN << "Influxdb connection failed: code #" << ec.value() << " -- " << ec.message();
                return;
            }

            m_isDbConnected = true;
            m_dbChannel = channel;

            try {

                m_dbChannel->readAsyncStringUntil("\r\n", bind_weak(&InfluxDataLogger::onDbRead, this, _1, _2));

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_WARN << "Influxdb read request failed: " << e;
                m_isDbConnected = false;
                m_dbChannel->close();
                m_dbConnection->stop();
                m_dbChannel.reset();
                m_dbConnection.reset();
            }
        }


        void InfluxDataLogger::onDbRead(const karabo::net::ErrorCode& ec, const std::string& line) {
            if (ec) {
                m_isDbConnected = false;
                m_dbChannel->close();
                m_dbConnection->stop();
                m_dbConnection.reset();
                KARABO_LOG_FRAMEWORK_WARN << "Influxdb read failed: code #" << ec.value() << " -- " << ec.message();
                return;
            }

            KARABO_LOG_FRAMEWORK_INFO << "DBREAD Ack: " << line;

            try {

                m_dbChannel->readAsyncStringUntil("\r\n", bind_weak(&InfluxDataLogger::onDbRead, this, _1, _2));

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_WARN << "Influxdb read request failed: " << e;
                m_isDbConnected = false;
                m_dbChannel->close();
                m_dbConnection->stop();
                m_dbChannel.reset();
                m_dbConnection.reset();
            }
        }


        void InfluxDataLogger::onDbWrite(const karabo::net::ErrorCode& ec) {
            if (ec) {
                m_isDbConnected = false;
                m_dbChannel->close();
                m_dbConnection->stop();
                m_dbConnection.reset();
                KARABO_LOG_FRAMEWORK_WARN << "Influxdb write failed: code #" << ec.value() << " -- " << ec.message();
                return;
            }
        }


        void InfluxDataLogger::handleChanged(const karabo::util::Hash& configuration, const std::string& user,
                                             const DeviceData::Pointer& devicedata) {

            InfluxDeviceData::Pointer data = boost::static_pointer_cast<InfluxDeviceData>(devicedata);
            data->m_user = user; // set under m_strand protection
            const std::string& deviceId = data->m_deviceToBeLogged;

            // To write log I need schema - but that has arrived before connecting signal[State]Changed to slotChanged
            // and thus before any data can arrive here in handleChanged.
            vector<string> paths;
            getPathsForConfiguration(configuration, data->m_currentSchema, paths);

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
                //{
                //    // Update time stamp for DeviceData destructor and property "lastUpdatesUtc".
                //    // Since the latter is accessing it when not posted on data->m_strand, need mutex protection:
                //    boost::mutex::scoped_lock lock(data->m_lastTimestampMutex);
                //    if (t.getEpochstamp() > data->m_lastDataTimestamp.getEpochstamp()) {
                //        // If mixed timestamps in single message (or arrival in wrong order), always take most recent one.
                //        data->m_updatedLastTimestamp = true;
                //        data->m_lastDataTimestamp = t;
                //    }
                //}

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

                if (data->m_pendingLogin) {
                    data->logValue(deviceId, "log", t, "LOGIN", Types::STRING);
                    data->m_pendingLogin = false;
                }
                data->logValue(deviceId, path, t, value, leafNode.getType());
            }
            if (m_bufferLen >= get<std::uint32_t>("maxQuerySize")) flushOne(devicedata);
        }


        void InfluxDeviceData::logValue(const std::string& deviceId, const std::string& path,
                                        const karabo::util::Timestamp& ts, const std::string& value,
                                        const karabo::util::Types::ReferenceType& type) {
            std::string field_value;
            switch (type) {
                case Types::BOOL:
                {
                    if (value == "1") {
                        field_value = str(boost::format("%1%_b=t") % path);
                    } else if (value == "0") {
                        field_value = str(boost::format("%1%_b=f") % path);
                    }
                    break;
                }
                case Types::INT8:
                case Types::UINT8:
                case Types::INT16:
                case Types::UINT16:
                case Types::INT32:
                case Types::UINT32:
                case Types::INT64:
                case Types::UINT64:
                {
                    field_value = str(boost::format("%1%_i=%2%i") % path % value);
                    break;
                }
                case Types::FLOAT:
                case Types::DOUBLE:
                case Types::COMPLEX_FLOAT:
                case Types::COMPLEX_DOUBLE:
                {
                    field_value = str(boost::format("%1%_f=%2%") % path % value);
                    break;
                }
                case Types::STRING:
                case Types::VECTOR_STRING:
                case Types::VECTOR_HASH:
                {
                    std::string v = boost::replace_all_copy(value, "\\", "\\\\");
                    boost::replace_all(v, "\"", "\\\"");
                    field_value = str(boost::format("%1%_s=\"%2%\"") % path % v);
                    break;
                }
                default:
                    if (Types::isVector(type)) {
                        field_value = str(boost::format("%1%_s=\"%2%\"") % path % value);
                        break;
                    }
                    return;
            }

            if (m_query.str().empty()) {
                m_query << deviceId
                        << " " << str(boost::format("%1%_s=%2%s") % "user" % m_user);
            }
            m_query << "," << field_value;

            if (ts != m_lastDataTimestamp) terminateQuery();
            m_updatedLastTimestamp = true;
            m_lastDataTimestamp = ts;
        }


        void InfluxDeviceData::terminateQuery() {
            const unsigned long long tid = m_lastDataTimestamp.getTrainId();
            if (tid > 0) {
                m_query << "," << str(boost::format("%1%_i=%2%i") % "tid" % tid);
            }
            const unsigned long long ts = m_lastDataTimestamp.toTimestamp() * 1000000000; // in ns
            if (ts > 0) {
                m_query << " " << ts;
            }
            m_query << "\n";
            m_this->enqueueQuery(m_query);
            m_query.str("");  // Clear content of stringstream
        }


        void InfluxDataLogger::enqueueQuery(const std::stringstream& ss) {
            boost::mutex::scoped_lock lock(m_bufferMutex);
            m_buffer << ss.str();
            std::streampos readpos = m_buffer.tellg();
            m_buffer.seekg(0, ios::end);
            m_bufferLen = m_buffer.tellg();
            m_buffer.seekg(readpos);
        }


        void InfluxDataLogger::flushOne(const DeviceData::Pointer& devicedata) {
            if (!m_isDbConnected) {
                initializeBackend(devicedata);
                return;
            }

            if (m_bufferLen == 0) return;
            
            std::ostringstream oss;
            std::string url = m_url; // copy
            boost::replace_first(url, "tcp", "http"); // replace url
            oss << str(boost::format("POST %1%/write?db=%2% HTTP/1.1\r\n"
                                     "Host: %1%\r\n"
                                     "Content-Length: %3%\r\n\r\n") % url % m_topic % m_bufferLen);
            {
                // Append buffer stream under mutex and clean
                boost::mutex::scoped_lock lock(m_bufferMutex);
                oss << m_buffer.str();
                m_buffer.str(""); // clear buffer stream
                m_bufferLen = 0;
            }
            auto data = boost::make_shared<std::vector<char> >(std::vector<char>());
            data->assign(oss.str().begin(), oss.str().end());
            m_dbChannel->writeAsyncVectorPointer(data, bind_weak(&InfluxDataLogger::onDbWrite, this, _1));
        }


        void InfluxDataLogger::handleSchemaUpdated(const karabo::util::Schema& schema, const DeviceData::Pointer& devicedata) {
            const InfluxDeviceData::Pointer& data = boost::static_pointer_cast<InfluxDeviceData>(devicedata);

            const std::string& deviceId = data->m_deviceToBeLogged;

            data->m_currentSchema = schema;

            // Since schema updates are rare, do not store this serialiser as the one for Hash (data->m_serializer):
            TextSerializer<Schema>::Pointer serializer = TextSerializer<Schema>::create(Hash("Xml.indentation", -1));
            string archive;
            serializer->save(schema, archive);

            std::ostringstream oss;
            std::string url = m_url; // copy
            boost::replace_first(url, "tcp", "http"); // replace url
            oss << str(boost::format("POST %1%/write?db=%2% HTTP/1.1\r\n"
                                     "Host: %1%\r\n"
                                     "Content-Length: %3%\r\n\r\n") % url % m_topic % m_bufferLen);

            oss << deviceId << "," << str(boost::format("%1%_s=%2%s") % "user" % data->m_user)
                    << " " << str(boost::format("%1%_s=%2%s") % "schema" % archive);
            // No timestamp here ... influxdb will generate one...
            oss << "\n";

            auto dataPointer = boost::make_shared<std::vector<char> >(std::vector<char>());
            dataPointer->assign(oss.str().begin(), oss.str().end());
            m_dbChannel->writeAsyncVectorPointer(dataPointer, bind_weak(&InfluxDataLogger::onDbWrite, this, _1));
        }
    }
}

