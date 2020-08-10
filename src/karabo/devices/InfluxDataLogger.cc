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

	const unsigned int InfluxDataLogger::k_httpResponseTimeoutMs = 1500u;


        InfluxDeviceData::InfluxDeviceData(const karabo::util::Hash& input)
            : DeviceData(input)
            , m_dbClientRead(input.get<karabo::net::InfluxDbClient::Pointer>("dbClientReadPointer"))
            , m_dbClientWrite(input.get<karabo::net::InfluxDbClient::Pointer>("dbClientWritePointer"))
            , m_serializer(karabo::io::BinarySerializer<karabo::util::Hash>::create("Bin"))
            , m_archive() {
        }


        InfluxDeviceData::~InfluxDeviceData() {
        }


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
                // TODO: Since the time when logging stops might be of interest as well (for silent devices), it would
                //       be nice to store it somehow as well.
                boost::mutex::scoped_lock lock(m_lastTimestampMutex);
                const unsigned long long ts = m_lastDataTimestamp.toTimestamp() * PRECISION_FACTOR;
                ss << deviceId << "__EVENTS,type=\"-LOG\" karabo_user=\"" << m_user << "\" " << ts << "\n";
            }
            m_dbClientWrite->enqueueQuery(ss.str());
            m_dbClientWrite->flushBatch();

            KARABO_LOG_FRAMEWORK_INFO << "Proxy for \"" << deviceId << "\" is destroyed ...";
        }


        void InfluxDeviceData::handleChanged(const karabo::util::Hash& configuration, const std::string& user) {

            m_dbClientWrite->connectDbIfDisconnected();

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
            std::stringstream query;
            Timestamp lineTimestamp(Epochstamp(0ull, 0ull), Trainstamp(0ull));
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
                {
                    // Update time stamp for updates of property "lastUpdatesUtc"
                    // Since the former is accessing it when not posted on m_strand, need mutex protection:
                    boost::mutex::scoped_lock lock(m_lastTimestampMutex);
                    if (t.getEpochstamp() != m_lastDataTimestamp.getEpochstamp()) {
                        // If mixed timestamps in single message (or arrival in wrong order), always take last received.
                        m_updatedLastTimestamp = true;
                        m_lastDataTimestamp = t;
                    }
                }

                if (noArchive) continue; // Bail out after updating time stamp!
                std::string value; // "value" should be a string, so convert depending on type ...
                bool isFinite = true; // false for nan and inf DOUBLE/FLOAT
                Types::ReferenceType type = leafNode.getType();
                if (type == Types::VECTOR_HASH) {
                    // Represent any vector<Hash> as Base64 string
                    std::vector<char> archive;
                    m_serializer->save(leafNode.getValue<std::vector < Hash >> (), archive);
                    const unsigned char* uarchive = reinterpret_cast<const unsigned char*> (archive.data());
                    value = base64Encode(uarchive, archive.size());
                } else if (leafNode.getType() == Types::CHAR) {
                    const unsigned char* uarchive = reinterpret_cast<const unsigned char*> (&leafNode.getValue<char>());
                    value = base64Encode(uarchive, 1ul);
                } else if (leafNode.getType() == Types::VECTOR_CHAR) {
                    const std::vector<char> & v = leafNode.getValue<std::vector<char>>();
                    const unsigned char* uarchive = reinterpret_cast<const unsigned char*> (v.data());
                    value = base64Encode(uarchive, v.size());
                } else if (type == Types::VECTOR_UINT8) {
                    // The generic vector code below uses toString(vector<unsigned char>) which
                    // erroneously uses base64 encoding.
                    // We do not dare to fix that now, but workaround it here to have a human readable string
                    // in the DB to ease the use of the data outside Karabo:
                    const std::vector<unsigned char>& vec = leafNode.getValue<std::vector<unsigned char>>();
                    if (!vec.empty()) {
                        std::ostringstream s;
                        s << static_cast<unsigned int> (vec[0]);
                        for (size_t i = 1ul; i < vec.size(); ++i) {
                            s << "," << static_cast<unsigned int> (vec[i]);
                        }
                        value = s.str();
                    }
                } else if (type == Types::VECTOR_STRING) {
                    // Special case: convert to JSON and then base64 ...
                    const std::vector<std::string>& vecstr = leafNode.getValue<std::vector<std::string> >();
                    nl::json j(vecstr);                     // convert to JSON
                    const std::string str = j.dump();       // JSON as a string
                    const unsigned char* encoded = reinterpret_cast<const unsigned char*>(str.c_str());
                    const size_t length = str.length();
                    value = base64Encode(encoded, length);  // encode to base64
                } else if (Types::isVector(type)) {
                    // ... and treat  any other vectors as a comma separated text string of vector elements
                    value = toString(leafNode.getValueAs<std::string, std::vector>());
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
                    // behavior on simple casting is implementation defined. We memcpy instead to be sure of the results
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
                    terminateQuery(query, lineTimestamp);
                    lineTimestamp = t;
                }
                if (m_pendingLogin) {
                    // TRICK: 'configuration' is the one requested at the beginning. For devices which have
                    // properties with older timestamps than the time of their instantiation (as e.g. read from
                    // hardware), we can claim that logging is active only from the most recent update we receive here.
                    const auto& attrsOfPathWithMostRecentStamp = configuration.getAttributes(paths.back());
                    const Epochstamp t = Epochstamp::fromHashAttributes(attrsOfPathWithMostRecentStamp);
                    const unsigned long long ts = t.toTimestamp() * PRECISION_FACTOR;
                    std::stringstream ss;
                    ss << deviceId << "__EVENTS,type=\"+LOG\" karabo_user=\"" << m_user << "\" " << ts << "\n";

                    m_pendingLogin = false;
                    m_dbClientWrite->enqueueQuery(ss.str());
                }

                // isFinite matters only for FLOAT/DOUBLE
                logValue(query, deviceId, path, value, leafNode.getType(), isFinite);
            }
            if (!query.str().empty()) {
                // flush the query if something is in it.
                terminateQuery(query, lineTimestamp);
            }
        }


        void InfluxDeviceData::logValue(std::stringstream& query, const std::string& deviceId, const std::string& path,
                                        const std::string& value, karabo::util::Types::ReferenceType type,
                                        bool isFinite) {
            std::string field_value;
            switch (type) {
                case Types::BOOL:
                {
                    if (value.empty()) {
                        // Should never happen! We try to save the line protocol by skipping
                        KARABO_LOG_FRAMEWORK_ERROR << "Empty value for property '" << path << "' on device '" << deviceId << "'";
                        return;
                    }
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
                case Types::UINT64:
                {
                    if (value.empty()) {
                        // Should never happen! We try to save the line protocol by skipping
                        KARABO_LOG_FRAMEWORK_ERROR << "Empty value for property '" << path << "' on device '" << deviceId << "'";
                        return;
                    }
                    field_value = path + "-" + Types::to<ToLiteral>(type) + "=" + value + "i";
                    break;
                }
                case Types::FLOAT:
                case Types::DOUBLE:
                {
                    if (value.empty()) {
                        // Should never happen! We try to save the line protocol by skipping
                        KARABO_LOG_FRAMEWORK_ERROR << "Empty value for property '" << path << "' on device '" << deviceId << "'";
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
                case Types::VECTOR_COMPLEX_DOUBLE:
                {
                    // empty strings shall be saved. They do not spoil the line protocol since they are between quotes
                    field_value = path + "-" + Types::to<ToLiteral>(type) + "=\"" + value + "\"";
                    break;
                }
                case Types::VECTOR_CHAR:
                case Types::VECTOR_HASH:
                case Types::CHAR:
                {
                    if (value.empty()) {
                        // Should never happen! These types are base64 encoded
                        KARABO_LOG_FRAMEWORK_ERROR << "Empty value for property '" << path << "' on device '" << deviceId << "'";
                        return;
                    }
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

            if (query.str().empty()) {
                query << deviceId << ",karabo_user=\"" << m_user << "\" " << field_value;
            } else {
                query << "," << field_value;
            }
        }


        void InfluxDeviceData::terminateQuery(std::stringstream& query,
                                              const karabo::util::Timestamp& stamp) {
            const unsigned long long tid = stamp.getTrainId();
            // influxDB integers are signed 64 bits. here we check that the we are within such limits
            // Assuming a trainId rate of 10 Hz this limit will be surpassed in about 29 billion years
            if (0 < tid && tid <= static_cast<unsigned long long>(std::numeric_limits<long long>::max())) {
                query << ",_tid=" << tid << "i";
            }
            const unsigned long long ts = stamp.toTimestamp() * PRECISION_FACTOR;
            if (ts > 0) {
                query << " " << ts;
            }
            query << "\n";
            m_dbClientWrite->enqueueQuery(query.str());
            query.str("");
        }


        void InfluxDeviceData::handleSchemaUpdated(const karabo::util::Schema& schema,
                                                   const karabo::util::Timestamp& stamp) {
            // Before checking client status: enables buffering of property updates in handleChanged:
            m_currentSchema = schema;

            if (!m_dbClientRead->isConnected()) {
                m_dbClientRead->connectDbIfDisconnected();
                KARABO_LOG_FRAMEWORK_WARN << "Skip schema updated: DB connection is requested...";
                return;
            }

            // Use Binary serializer as soon as we encode into Base64:
            karabo::io::BinarySerializer<karabo::util::Schema>::Pointer serializer
                    = karabo::io::BinarySerializer<karabo::util::Schema>::create("Bin");
            serializer->save(schema, m_archive);
            const unsigned char* uarchive = reinterpret_cast<const unsigned char*> (m_archive.data());

            // Calculate 'digest' on serialized schema
            const std::size_t DIGEST_LENGTH = 20;
            unsigned char obuf[DIGEST_LENGTH];
            SHA1(uarchive, m_archive.size(), obuf);
            std::ostringstream dss;
            for (size_t i = 0; i < DIGEST_LENGTH; ++i) dss << std::hex << int(obuf[i]);
            std::string schDigest(dss.str());

            std::ostringstream oss;
            oss << "SELECT COUNT(*) FROM \"" << m_deviceToBeLogged
                    << "__SCHEMAS\" WHERE digest='\"" << schDigest << "\"'";
            m_dbClientRead->queryDb(oss.str(), bind_weak(&InfluxDeviceData::checkSchemaInDb, this,
                                                         stamp, schDigest, _1));
        }


        void InfluxDeviceData::checkSchemaInDb(const karabo::util::Timestamp& stamp,
                                               const std::string& schDigest,
                                               const HttpResponse& o) {
            //TODO: Do error handling ...
            //...
            const unsigned long long ts = stamp.toTimestamp() * PRECISION_FACTOR;
            std::stringstream ss;
            ss << m_deviceToBeLogged << "__EVENTS,type=\"SCHEMA\" schema_digest=\"" << schDigest << "\" " << ts << "\n";

            nl::json j = nl::json::parse(o.payload);
            auto count = j["results"][0]["series"][0]["values"][0][1];
            if (count.is_null()) {
                // digest is not found:  json is '{"results":[{"statement_id":0}]}'
                // Encode serialized schema into Base64
                const unsigned char* uarchive = reinterpret_cast<const unsigned char*> (m_archive.data());
                std::string base64Schema = base64Encode(uarchive, m_archive.size());
                // and write to SCHEMAS table ...
                ss << m_deviceToBeLogged << "__SCHEMAS," << "digest=\""
                        << schDigest << "\" schema=\"" << base64Schema << "\"\n";
                // Flush what was accumulated before ...
                m_dbClientWrite->flushBatch();
            }
            // digest already exists!
            KARABO_LOG_FRAMEWORK_DEBUG << "checkSchemaInDb ...\n" << o.payload;
            m_dbClientWrite->enqueueQuery(ss.str());
            m_dbClientWrite->flushBatch();
        }


        void InfluxDataLogger::expectedParameters(karabo::util::Schema& expected) {

            OVERWRITE_ELEMENT(expected).key("state")
                    .setNewOptions(State::INIT, State::NORMAL, State::ERROR)
                    .setNewDefaultValue(State::INIT)
                    .commit();

            STRING_ELEMENT(expected).key("urlWrite")
                    .displayedName("Influxdb URL (write)")
                    .description("URL should be given in form: tcp://host:port. 'Write' interface")
                    .assignmentOptional().defaultValue("tcp://localhost:8086")
                    .init()
                    .commit();

            STRING_ELEMENT(expected).key("urlQuery")
                    .displayedName("Influxdb URL (query)")
                    .description("URL should be given in form: tcp://host:port. 'Query' interface")
                    .assignmentOptional().defaultValue("tcp://localhost:8086")
                    .init()
                    .commit();

            STRING_ELEMENT(expected).key("dbname")
                    .displayedName("Database name")
                    .description("Name of the database in which the data should be inserted")
                    .assignmentMandatory()
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
            , m_dbName(input.get<std::string>("dbname")) {

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
            m_urlQuery = input.get<std::string>("urlQuery");

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

            Hash configWrite("dbname", m_dbName,
                             "url", m_urlWrite,
                             "durationUnit", DUR,
                             "maxPointsInBuffer", input.get<unsigned int>("maxBatchPoints"));

            configWrite.set<std::string>("dbUser", dbUserWrite);
            configWrite.set<std::string>("dbPassword", dbPasswordWrite);

            m_clientWrite = Configurator<InfluxDbClient>::create("InfluxDbClient", configWrite);

            Hash configRead("dbname", m_dbName,
                            "url", m_urlQuery,
                            "durationUnit", DUR,
                            "maxPointsInBuffer", input.get<unsigned int>("maxBatchPoints"));

            configRead.set<std::string>("dbUser", dbUserQuery);
            configRead.set<std::string>("dbPassword", dbPasswordQuery);

            m_clientRead = Configurator<InfluxDbClient>::create("InfluxDbClient", configRead);
        }


        InfluxDataLogger::~InfluxDataLogger() {
        }


        void InfluxDataLogger::preDestruction() {

            DataLogger::preDestruction();

            auto prom = boost::make_shared<std::promise<void>>();
            std::future<void> fut = prom->get_future();
            m_clientWrite->flushBatch([prom](const HttpResponse & resp) {
                prom->set_value();
            });

            auto status = fut.wait_for(std::chrono::milliseconds(1500));

            if (status != std::future_status::ready) {
                KARABO_LOG_FRAMEWORK_WARN << "Timeout in flushBatch while waiting for response from InfluxDB.";
                return;
            }
            fut.get();
        }


        DeviceData::Pointer InfluxDataLogger::createDeviceData(const karabo::util::Hash& cfg) {
            Hash config = cfg;
            config.set("dbClientReadPointer", m_clientRead);
            config.set("dbClientWritePointer", m_clientWrite);
            DeviceData::Pointer deviceData =
                    Factory<DeviceData>::create<karabo::util::Hash>("InfluxDataLoggerDeviceData", config);
            return deviceData;
        }


        void InfluxDataLogger::initializeLoggerSpecific() {
            m_clientWrite->connectDbIfDisconnected(bind_weak(&InfluxDataLogger::checkDb, this));
        }


        void InfluxDataLogger::showDatabases(const InfluxResponseHandler& action) {
            std::string statement = "SHOW DATABASES";
            m_clientRead->postQueryDb(statement, action);
            KARABO_LOG_FRAMEWORK_INFO << statement << "\n";
        }


        void InfluxDataLogger::createDatabase(const InfluxResponseHandler& action) {
            const std::string statement = "CREATE DATABASE " + m_dbName;
            m_clientWrite->postQueryDb(statement, action);
            KARABO_LOG_FRAMEWORK_INFO << statement << "\n";
        }


        void InfluxDataLogger::onCreateDatabase(const HttpResponse& o) {
            if (o.code >= 300 ||
                (o.code == 200 && o.payload.find("statement-id") == std::string::npos)) {
                // Database not available and could not be created. A response for an unsuccessful database creation
                // can also have a 200 status code but will have the fixed payload '{"result":[]}'. A successful
                // database creation will have a 200 status code, will have 'chunked' as transfer encoding and will
                // have the payload '{"results:[{"stattement-id":0}]}'.
                KARABO_LOG_FRAMEWORK_ERROR << "Database '" << m_dbName << "' not available. "
                        << "Tried to create it but got error with http status code '"
                        << o.code << "' and message '" << o.message << "'. InfluxDataLogger going to ERROR state.";
                updateState(State::ERROR,
                            Hash("status",
                                 std::string("Database '") + m_dbName + "' not available. Influx response to create database request:" +
                                 o.toString()));
            } else {
                KARABO_LOG_FRAMEWORK_INFO << "Database " << m_dbName << " created";
                startConnection();
            }
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
                updateState(State::ERROR, Hash("status", "Failed unpack list of databases."));
                return;
            }

            KARABO_LOG_FRAMEWORK_INFO << "Database '" << m_dbName << "' not available. Will try to create it.";
            createDatabase(bind_weak(&karabo::devices::InfluxDataLogger::onCreateDatabase, this, _1));
        }


        void InfluxDataLogger::onPingDb(const HttpResponse& o) {
            if (o.code >= 300) {
                KARABO_LOG_FRAMEWORK_ERROR << "Failed to ping Inlfux DB: " << o.toString();
                updateState(State::ERROR, Hash("status", "Failed to ping InfluxDB."));
                return;
            }

            KARABO_LOG_FRAMEWORK_INFO << "X-Influxdb-Build: " << o.build << ", X-Influxdb-Version: " << o.version;
            showDatabases(bind_weak(&karabo::devices::InfluxDataLogger::onShowDatabases, this, _1));
        }


        void InfluxDataLogger::checkDb() {
            KARABO_LOG_FRAMEWORK_INFO << "PING InfluxDB server ...";
            m_clientWrite->getPingDb(bind_weak(&karabo::devices::InfluxDataLogger::onPingDb, this, _1));
        }


        void InfluxDataLogger::flushImpl(const boost::shared_ptr<SignalSlotable::AsyncReply>& aReplyPtr) {
            karabo::net::InfluxResponseHandler handler;
            if (aReplyPtr) {
                handler = [aReplyPtr](const HttpResponse & resp) {
                    if (resp.code >= 300) {
                        std::ostringstream errMsg;
                        errMsg << "Flush request failed - InfluxDb response code/message: " << resp.code
                                << " '" << resp.message << "'";
                        aReplyPtr->error(errMsg.str());
                    } else {
                        (*aReplyPtr)();
                    }
                };
            }
            m_clientWrite->flushBatch(handler);
        }
    }
}
