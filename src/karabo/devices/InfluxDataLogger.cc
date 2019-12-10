#include <iostream>
#include <chrono>
#include <future>
#include <boost/io/detail/quoted_manip.hpp>
#include <openssl/sha.h>
#include "InfluxDataLogger.hh"
#include <nlohmann/json.hpp>
#include <karabo/net/InfluxDbClient.hh>

// Precision is microseconds
// for nanoseconds:  DUR is "ns",  PRECISION_FACTOR is 1000000000
#define DUR "u"
#define PRECISION_FACTOR 1000000

namespace karabo {
    namespace devices {

        namespace nl = nlohmann;
        using namespace karabo::io;
        using namespace karabo::net;
        using namespace karabo::util;


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, DataLogger, InfluxDataLogger)
        KARABO_REGISTER_IN_FACTORY_1(DeviceData, InfluxDeviceData, karabo::util::Hash)

	const unsigned int InfluxDeviceData::k_httpResponseTimeoutMs = 1500u;


        InfluxDeviceData::InfluxDeviceData(const karabo::util::Hash& input)
            : DeviceData(input)
            , m_this(input.get<InfluxDataLogger*>("this"))
            , m_query()
            , m_serializer(karabo::io::BinarySerializer<karabo::util::Hash>::create("Bin"))
            , m_digest("")
            , m_archive() {
        }


        InfluxDeviceData::~InfluxDeviceData() {
            if (m_initLevel != InitLevel::COMPLETE) {
                // We have not yet started logging this device, so nothing to mark about being done.
                return;
            }

            const std::string& deviceId = m_deviceToBeLogged;
            std::stringstream ss;
            ss << deviceId + "__EVENTS,type=\"-LOG\" karabo_user=\"" << m_user << "\"\n";

            std::promise<void> prom;
            std::future<void> fut = prom.get_future();

            std::string message(ss.str());
            m_this->m_client->postWriteDb(message,[&prom](const HttpResponse& o) {
                prom.set_value();
            });

            auto status = fut.wait_for(std::chrono::milliseconds(k_httpResponseTimeoutMs));
            if (status != std::future_status::ready) {
                KARABO_LOG_FRAMEWORK_WARN << "Timeout in ~InfluxDeviceData for message: \n" << message;
                return;
            }
            fut.get();
            KARABO_LOG_FRAMEWORK_INFO << "Proxy for \"" << deviceId << "\" is destroyed ...";
        }


        void InfluxDeviceData::handleChanged(const karabo::util::Hash& configuration, const std::string& user) {

            if (user.empty()) {
                m_user = ".";
            } else {
                m_user = user; // set under m_strand protection
            }
            const std::string& deviceId = m_deviceToBeLogged;

            // To write log I need schema - but that has arrived before connecting signal[State]Changed to slotChanged
            // and thus before any data can arrive here in handleChanged.
            std::vector<std::string> paths;
            getPathsForConfiguration(configuration, m_currentSchema, paths);

            for (size_t i = 0; i < paths.size(); ++i) {
                const std::string& path = paths[i];

                // Skip those elements which should not be archived
                const bool noArchive = (!m_currentSchema.has(path)
                                        || (m_currentSchema.hasArchivePolicy(path)
                                            && (m_currentSchema.getArchivePolicy(path) == Schema::NO_ARCHIVING)));

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
                bool terminateLine = false;
                {
                    // Update time stamp for DeviceData destructor and property "lastUpdatesUtc".
                    // Since the latter is accessing it when not posted on m_strand, need mutex protection:
                    boost::mutex::scoped_lock lock(m_lastTimestampMutex);
                    if (t.getEpochstamp() != m_lastDataTimestamp.getEpochstamp()) {
                        // If mixed timestamps in single message (or arrival in wrong order), always take most recent one.
                        m_updatedLastTimestamp = true;
                        m_lastDataTimestamp = t;
                        terminateLine = true;
                    }
                }

                if (noArchive) continue; // Bail out after updating time stamp!
                std::string value; // "value" should be a string, so convert depending on type ...
                if (leafNode.getType() == Types::VECTOR_HASH) {
                    // Represent any vector<Hash> as Base64 string
                    std::vector<char> archive;
                    m_serializer->save(leafNode.getValue<std::vector < Hash >> (), archive);
                    const unsigned char* uarchive = reinterpret_cast<const unsigned char*> (archive.data());
                    value = base64Encode(uarchive, archive.size());
                } else if (Types::isVector(leafNode.getType())) {
                    // ... and any other vector as a comma separated text string of vector elements
                    value = toString(leafNode.getValueAs<std::string, std::vector>());
                    if (leafNode.getType() == Types::VECTOR_STRING) {
                        // Line breaks in content confuse indexing and reading back - so better mangle strings... :-(.
                        boost::algorithm::replace_all(value, "\n", karabo::util::DATALOG_NEWLINE_MANGLE);
                    }
                } else {
                    value = leafNode.getValueAs<std::string>();
                    if (leafNode.getType() == Types::STRING) {
                        // Line breaks in content confuse indexing and reading back - so better mangle strings... :-(.
                        boost::algorithm::replace_all(value, "\n", karabo::util::DATALOG_NEWLINE_MANGLE);
                    }
                }

                if (m_pendingLogin) {
                    std::stringstream ss;
                    ss << deviceId << "__EVENTS,type=\"+LOG\" karabo_user=\"" << m_user << "\"\n";
                    m_pendingLogin = false;
                    m_this->enqueueQuery(ss.str());
                }

                logValue(deviceId, path, value, leafNode.getType());

                boost::mutex::scoped_lock lock(m_lastTimestampMutex);
                if (terminateLine) terminateQuery();
            }
        }


        void InfluxDeviceData::logValue(const std::string& deviceId, const std::string& path,
                                        const std::string& value, const karabo::util::Types::ReferenceType& type) {
            std::string field_value;
            switch (type) {
                case Types::BOOL:
                {
                    field_value = path + "-BOOL=" + ((value == "1") ? "t":"f");
                    break;
                }
                case Types::INT8:
                case Types::UINT8:
                case Types::INT16:
                case Types::UINT16:
                case Types::INT32:
                case Types::UINT32:
                case Types::INT64:
                {
                    field_value = path + "-" + Types::to<ToLiteral>(type) + "=" + value + "i";
                    break;
                }
                case Types::FLOAT:
                case Types::DOUBLE:
                {
                    field_value = path + "-" + Types::to<ToLiteral>(type) + "=" + value;
                    break;
                }
                case Types::COMPLEX_FLOAT:
                case Types::COMPLEX_DOUBLE:
                case Types::UINT64:
                case Types::VECTOR_HASH:
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
                case Types::VECTOR_COMPLEX_DOUBLE:
                {
                    field_value = path + "-" + Types::to<ToLiteral>(type) + "=\"" + value + "\"";
                    break;
                }
                case Types::STRING:
                case Types::VECTOR_STRING:
                {
                    std::string v = boost::replace_all_copy(value, "\\", "\\\\");
                    boost::replace_all(v, "\"", "\\\"");
                    field_value = path + "-" + Types::to<ToLiteral>(type) + "=\"" + v + "\"";
                    break;
                }
                default:
                    return;
            }

            if (m_query.str().empty()) {
                m_query << deviceId << ",karabo_user=\"" << m_user << "\" " << field_value;
            } else {
                m_query << "," << field_value;
            }
        }


        void InfluxDeviceData::terminateQuery() {
            const unsigned long long tid = m_lastDataTimestamp.getTrainId();
            if (tid > 0) {
                m_query << ",tid-UINT64=" << tid << "i";
            }
            const unsigned long long ts = m_lastDataTimestamp.toTimestamp() * PRECISION_FACTOR;
            if (ts > 0) {
                m_query << " " << ts;
            }
            m_query << "\n";
            m_this->enqueueQuery(m_query.str());
            m_query.str(""); // Clear content of stringstream
        }


        void InfluxDeviceData::handleSchemaUpdated(const karabo::util::Schema& schema, const DeviceData::Pointer& devicedata) {
            m_currentSchema = schema;

            // Use Binary serializer as soon as we encode into Base64:
            karabo::io::BinarySerializer<karabo::util::Schema>::Pointer serializer
                    = karabo::io::BinarySerializer<karabo::util::Schema>::create("Bin");
            serializer->save(schema, m_archive);
            const unsigned char* uarchive = reinterpret_cast<const unsigned char*> (m_archive.data());

            // Calculate 'digest' on serialized schema
            {
                const std::size_t DIGEST_LENGTH = 20;
                unsigned char obuf[DIGEST_LENGTH];
                SHA1(uarchive, m_archive.size(), obuf);
                std::ostringstream oss;
                for (size_t i = 0; i < DIGEST_LENGTH; ++i) oss << std::hex << int(obuf[i]);
                m_digest.assign(oss.str());
            }

            std::ostringstream oss;
            oss << "SELECT COUNT(*) FROM \"" << m_deviceToBeLogged << "__SCHEMAS\" WHERE digest='\"" << m_digest << "\"'";
            m_this->m_client->getQueryDb(oss.str(), bind_weak(&InfluxDataLogger::checkSchemaInDb, m_this, devicedata, _1));
        }


        void InfluxDataLogger::expectedParameters(karabo::util::Schema& expected) {

            STRING_ELEMENT(expected).key("url")
                    .displayedName("Influxdb URL")
                    .description("URL should be given in form: tcp://host:port")
                    .assignmentOptional().defaultValue("tcp://localhost:8086")
                    .init()
                    .commit();

            UINT32_ELEMENT(expected).key("maxBatchPoints")
                    .displayedName("Max. batch points")
                    .description("Max number of InfluxDB points in batch")
                    .assignmentOptional().defaultValue(200)
                    .init()
                    .commit();
        }


        InfluxDataLogger::InfluxDataLogger(const karabo::util::Hash& input)
            : DataLogger(input)
            , m_bufferMutex()
            , m_buffer()
            , m_bufferLen(0)
            , m_topic(getTopic())
            , m_nPoints(0)
            , m_startTimePoint() {
            Hash config("url", input.get<std::string>("url"), "dbname", m_topic, "durationUnit", DUR);
            m_client = boost::make_shared<InfluxDbClient>(config);
        }


        InfluxDataLogger::~InfluxDataLogger() {
        }


	void InfluxDataLogger::preDestruction() {
		DataLogger::preDestruction();
        }


        DeviceData::Pointer InfluxDataLogger::createDeviceData(const karabo::util::Hash& cfg) {
            Hash config = cfg;
            config.set("this", this);
            DeviceData::Pointer devicedata = Factory<DeviceData>::create<karabo::util::Hash>("InfluxDataLoggerDeviceData", config);
            return devicedata;
        }


        void InfluxDataLogger::initializeLoggerSpecific() {
            m_client->connectDbIfDisconnected(bind_weak(&InfluxDataLogger::checkDb, this));
        }


        void InfluxDataLogger::showDatabases(const InfluxResponseHandler& action) {
            std::string statement = "SHOW DATABASES";
            m_client->postQueryDb(statement, action);
            KARABO_LOG_FRAMEWORK_INFO << statement << "\n";
        }


        void InfluxDataLogger::createDatabase(const std::string& dbname, const InfluxResponseHandler& action) {
            std::string statement = "CREATE DATABASE " + dbname;
            m_client->postQueryDb(statement, action);
            KARABO_LOG_FRAMEWORK_INFO << statement << "\n";
        }


        void InfluxDataLogger::onCreateDatabase(const HttpResponse& o) {
            KARABO_LOG_FRAMEWORK_INFO << "Database " << m_topic << " created";
            startConnection();
        }


        void InfluxDataLogger::onShowDatabases(const HttpResponse& o) {
            //TODO: Do error handling here...
            // ...
            KARABO_LOG_FRAMEWORK_DEBUG << "onShowDatabases ...\n" << o.toString();

            nl::json j = nl::json::parse(o.payload);
            auto values = j["results"][0]["series"][0]["values"];
            assert(values.is_array());
            // Should we create database?
            for(nl::json::iterator it = values.begin(); it != values.end(); ++it) {
                if ((*it)[0].get<std::string>() == m_topic) {
                    KARABO_LOG_FRAMEWORK_INFO << "Database \"" << m_topic << "\" already exists";
                    startConnection();
                    return;
                }
            }
            // CREATE DATABASE m_topic
            createDatabase(m_topic, bind_weak(&karabo::devices::InfluxDataLogger::onCreateDatabase, this, _1));
        }


        void InfluxDataLogger::onPingDb(const HttpResponse& o) {
                //TODO: Do error handling here...
                // ...
                KARABO_LOG_FRAMEWORK_INFO << "X-Influxdb-Build: " << o.build << ", X-Influxdb-Version: " << o.version;
                showDatabases(bind_weak(&karabo::devices::InfluxDataLogger::onShowDatabases, this, _1));
        }


        void InfluxDataLogger::checkDb() {
            KARABO_LOG_FRAMEWORK_INFO << "PING InfluxDB server ...";
            m_client->getPingDb(bind_weak(&karabo::devices::InfluxDataLogger::onPingDb, this, _1));
        }


        void InfluxDataLogger::handleChanged(const karabo::util::Hash& config, const std::string& user,
                           const DeviceData::Pointer& devicedata) {
            if (!m_client->isConnected()) {
                m_client->connectDbIfDisconnected();
            }
            InfluxDeviceData::Pointer data = boost::static_pointer_cast<InfluxDeviceData>(devicedata);
            data->handleChanged(config, user);
            flushOne(devicedata);
        }


        void InfluxDataLogger::enqueueQuery(const std::string& line) {
            boost::mutex::scoped_lock lock(m_bufferMutex);
            m_buffer << line;
            ++m_nPoints;
            std::streampos readpos = m_buffer.tellg();
            m_buffer.seekg(0, std::ios::end);
            m_bufferLen = m_buffer.tellg();
            m_buffer.seekg(readpos);
        }


        void InfluxDataLogger::flushOne(const DeviceData::Pointer& devicedata) {
            boost::mutex::scoped_lock lock(m_bufferMutex);
            // Check when we need to flush ...
            // ... by size ( number of points) ...
            if (m_nPoints >= get<std::uint32_t>("maxBatchPoints")) flushBatch(devicedata);
            if (m_nPoints == 0) return;
            // ... by time ...
            auto endTimePoint = std::chrono::high_resolution_clock::now();
            auto dur = endTimePoint - m_startTimePoint;
            auto elapsedTimeInSeconds = std::chrono::duration_cast<std::chrono::seconds>(dur).count();
            if (elapsedTimeInSeconds >= get<std::uint32_t>("flushInterval")) flushBatch(devicedata);
        }


        void InfluxDataLogger::flushBatch(const DeviceData::Pointer& devicedata) {
            if (m_bufferLen > 0 && m_nPoints > 0) {
                // Post accumulated batch ...
                m_client->postWriteDb(m_buffer.str(), [](const HttpResponse& o) {
                    if (o.code != 204) {
                        KARABO_LOG_FRAMEWORK_ERROR << "Code " << o.code << " " << o.message;
                    }
                });
            }
            m_buffer.str(""); // clear buffer stream
            m_bufferLen = 0;
            m_nPoints = 0;
            m_startTimePoint = std::chrono::high_resolution_clock::now();
        }


        void InfluxDataLogger::checkSchemaInDb(const DeviceData::Pointer& devicedata, const HttpResponse& o) {
            //TODO: Do error handling ...
            //...
            InfluxDeviceData::Pointer data = boost::static_pointer_cast<InfluxDeviceData>(devicedata);
            const std::string& deviceId = data->m_deviceToBeLogged;
            const std::string& digest   = data->m_digest;
            const std::vector<char>& archive = data->m_archive;

            std::stringstream ss;
            if (data->m_pendingLogin) {
                ss << deviceId << "__EVENTS,type=\"+LOG\" karabo_user=\"" << data->m_user << "\"\n";
                data->m_pendingLogin = false;
            }

            ss << deviceId << "__EVENTS,type=\"SCHEMA\" schema_digest=\"" << digest << "\"\n";

            nl::json j = nl::json::parse(o.payload);
            auto count = j["results"][0]["series"][0]["values"][0][1];
            if (count.is_null()) {
                // digest is not found:  json is '{"results":[{"statement_id":0}]}'
                // Encode serialized schema into Base64
                const unsigned char* uarchive = reinterpret_cast<const unsigned char*> (archive.data());
                std::string base64Schema = base64Encode(uarchive, archive.size());
                // and write to SCHEMAS table ...
                ss << deviceId << "__SCHEMAS," << "digest=\"" << digest << "\" schema=\"" << base64Schema << "\"\n";
                // Flush what was accumulated before ...
                boost::mutex::scoped_lock lock(m_bufferMutex);
                flushBatch(devicedata);
            }
            // digest already exists!
            KARABO_LOG_FRAMEWORK_DEBUG << "checkSchemaInDb ...\n" << o.payload;
            enqueueQuery(ss.str());
            boost::mutex::scoped_lock lock(m_bufferMutex);
            flushBatch(devicedata);
        }


        void InfluxDataLogger::handleSchemaUpdated(const karabo::util::Schema& schema, const DeviceData::Pointer& devicedata) {
            if (!m_client->isConnected()) {
                m_client->connectDbIfDisconnected();
                KARABO_LOG_FRAMEWORK_WARN << "Skip signalSchemaUpdated: DB connection is requested...";
                return;
            }
            InfluxDeviceData::Pointer data = boost::static_pointer_cast<InfluxDeviceData>(devicedata);
            data->handleSchemaUpdated(schema, devicedata);
        }
    }
}
