/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <streambuf>

#include "karabo/io/Input.hh"
#include "karabo/io/TextSerializer.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/xms/SlotElement.hh"

#include "DataLogger.hh"

namespace karabo {
    namespace devices {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace karabo::xms;


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, DataLogger)

        void DataLogger::expectedParameters(Schema& expected) {

            OVERWRITE_ELEMENT(expected).key("state")
                    .setNewOptions(State::INIT, State::NORMAL)
                    .setNewDefaultValue(State::INIT)
                    .commit();

            STRING_ELEMENT(expected).key("deviceToBeLogged")
                    .displayedName("Device to be logged")
                    .description("The device that should be logged by this logger instance")
                    .assignmentMandatory()
                    .commit();

            PATH_ELEMENT(expected).key("directory")
                    .displayedName("Directory")
                    .description("The directory where the log files should be placed")
                    .assignmentMandatory()
                    .commit();

            INT32_ELEMENT(expected).key("maximumFileSize")
                    .displayedName("Maximum file size")
                    .description("After any archived file has reached this size it will be time-stamped and not appended anymore")
                    .unit(Unit::BYTE)
                    .metricPrefix(MetricPrefix::MEGA)
                    .assignmentMandatory()
                    .commit();

            UINT32_ELEMENT(expected).key("flushInterval")
                    .displayedName("Flush interval")
                    .description("The interval after which the memory accumulated data is made persistent")
                    .unit(Unit::SECOND)
                    .assignmentOptional().defaultValue(60)
                    .reconfigurable()
                    .commit();

            // Do not archive the archivers (would lead to infinite recursion)
            OVERWRITE_ELEMENT(expected).key("archive")
                    .setNewDefaultValue(false)
                    .commit();

            // Hide the loggers from the standard view in clients
            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue<int>(Schema::AccessLevel::ADMIN)
                    .commit();

            // Slow beats
            OVERWRITE_ELEMENT(expected).key("heartbeatInterval")
                    .setNewDefaultValue(60)
                    .commit();

            SLOT_ELEMENT(expected).key("flush")
                    .displayedName("Flush")
                    .description("Persist buffered data")
                    .allowedStates(State::NORMAL)
                    .commit();
        }


        DataLogger::DataLogger(const Hash& input)
            : karabo::core::Device<>(input)
            , m_currentSchemaChanged(true)
            , m_pendingLogin(true)
            , m_propsize(0)
            , m_lasttime(0)
            , m_flushDeadline(karabo::net::EventLoop::getIOService())
            , m_doFlushFiles(true)
            , m_numChangedConnected(0) {

            m_idxprops.clear();

            input.get("deviceToBeLogged", m_deviceToBeLogged);
            // start "flush" actor ...
            input.get("flushInterval", m_flushInterval); // in seconds

            // Register slots in constructor to ensure existence when sending instanceNew
            KARABO_SLOT(slotChanged, Hash /*changedConfig*/, string /*deviceId*/);
            KARABO_SLOT(slotSchemaUpdated, Schema /*changedSchema*/, string /*deviceId*/);
            KARABO_SLOT(slotTagDeviceToBeDiscontinued, bool /*wasValidUpToNow*/, char /*reason*/);
            KARABO_SLOT(flush);

            KARABO_INITIAL_FUNCTION(initialize)
        }


        DataLogger::~DataLogger() {
            m_doFlushFiles = false;
            if (m_flushDeadline.cancel())
                doFlush();

            slotTagDeviceToBeDiscontinued(true, 'L');
            KARABO_LOG_FRAMEWORK_INFO << this->getInstanceId() << ": dead.";
        }


        void DataLogger::initialize() {

            boost::system::error_code ec;

            m_user = "."; //TODO:  Define proper user running a device. The dot is unknown user?

            if (!boost::filesystem::exists(get<string>("directory")))
                boost::filesystem::create_directory(get<string>("directory"));
            if (!boost::filesystem::exists(get<string>("directory") + "/" + m_deviceToBeLogged)) {
                boost::filesystem::create_directories(get<string>("directory") + "/" + m_deviceToBeLogged, ec);
                if (ec) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Failed to create directories : " << m_deviceToBeLogged
                            << ". code = " << ec.value() << " -- " << ec.message();
                    throw KARABO_INIT_EXCEPTION("Failed to create directories  \"" + get<string>("directory")
                                                + "/" + m_deviceToBeLogged + "\" : " + ec.message());
                }
            }
            if (!boost::filesystem::exists(get<string>("directory") + "/" + m_deviceToBeLogged + "/raw"))
                boost::filesystem::create_directory(get<string>("directory") + "/" + m_deviceToBeLogged + "/raw");
            if (!boost::filesystem::exists(get<string>("directory") + "/" + m_deviceToBeLogged + "/idx"))
                boost::filesystem::create_directory(get<string>("directory") + "/" + m_deviceToBeLogged + "/idx");

            m_lastIndex = determineLastIndex(m_deviceToBeLogged);

            // First try to establish p2p before connecting signals - i.e. don't to spam the broker with signalChanged.
            if (std::getenv("KARABO_DISABLE_LOGGER_P2P") == NULL) {
                // copy to avoid capture of bare 'this'
                const std::string deviceToBeLogged = m_deviceToBeLogged;
                auto successHandler = [deviceToBeLogged] () {
                    KARABO_LOG_FRAMEWORK_INFO << "Going to establish p2p to '" << deviceToBeLogged << "'";
                };
                auto failureHandler = [deviceToBeLogged] () {
                    try {
                        throw;
                    } catch (const std::exception& e) {
                        // As of now (2017-09-20), this is expected for middlelayer...
                        KARABO_LOG_FRAMEWORK_WARN << "Cannot establish p2p to '" << deviceToBeLogged << "' since:\n"
                                << e.what();
                    }
                };
                asyncConnectP2p(m_deviceToBeLogged, successHandler, failureHandler);
            } else {
                KARABO_LOG_FRAMEWORK_WARN << "Data logging via p2p has been disabled for loggers!";
            }

            // Then connect to schema updates and afterwards request Schema (in other order we might miss an update).
            asyncConnect(m_deviceToBeLogged, "signalSchemaUpdated", "", "slotSchemaUpdated",
                         util::bind_weak(&DataLogger::handleSchemaConnected, this),
                         util::bind_weak(&DataLogger::errorToDieHandle, this, "Failed to connect to signalSchemaUpdated")
                         );
            // Final steps until DataLogger is properly initialised are treated in a chain of async handlers:
            // - If signalSchemaUpdated connected, request current schema;
            // - if that arrived, connect to both signal(State)Changed;
            // - if these two are connected, request initial configuration, start flushing and update state.
        }


        void DataLogger::handleSchemaConnected() {
            KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << ": Requesting slotGetSchema (receiveAsync)";

            request(m_deviceToBeLogged, "slotGetSchema", false)
                    .receiveAsync<karabo::util::Schema, std::string>
                    (util::bind_weak(&DataLogger::handleSchemaReceived, this, _1, _2),
                     util::bind_weak(&DataLogger::errorToDieHandle, this, "Failed to request schema")
                     );
        }


        void DataLogger::handleSchemaReceived(const karabo::util::Schema& schema, const std::string& deviceId) {

            // Set initial Schema - needed for receiving properly in slotChanged
            slotSchemaUpdated(schema, deviceId);

            // Now connect concurrently both, signalStateChanged and signalChanged, to the same slot.
            // The same pollConfig is callback (in case of success) for both connection requests. The second
            // time it is called it will request the configuration.
            const std::string failMsgBegin("Failed to connect to ");
            asyncConnect(m_deviceToBeLogged, "signalStateChanged", "", "slotChanged",
                         util::bind_weak(&DataLogger::handleConfigConnected, this),
                         util::bind_weak(&DataLogger::errorToDieHandle, this, failMsgBegin + "signalStateChanged")
                         );
            asyncConnect(m_deviceToBeLogged, "signalChanged", "", "slotChanged",
                         util::bind_weak(&DataLogger::handleConfigConnected, this),
                         util::bind_weak(&DataLogger::errorToDieHandle, this, failMsgBegin + "signalChanged")
                         );
        }


        void DataLogger::handleConfigConnected() {
            boost::mutex::scoped_lock lock(m_numChangedConnectedMutex);
            if (++m_numChangedConnected == 2) {
                KARABO_LOG_FRAMEWORK_INFO << getInstanceId() << ": Requesting slotGetConfiguration (no wait)";
                requestNoWait(m_deviceToBeLogged, "slotGetConfiguration", "", "slotChanged");

                m_flushDeadline.expires_from_now(boost::posix_time::seconds(m_flushInterval));
                m_flushDeadline.async_wait(util::bind_weak(&DataLogger::flushActor, this, boost::asio::placeholders::error));

                // Done with initialisation:
                updateState(State::NORMAL);
            }
        }


        void DataLogger::errorToDieHandle(const std::string& reason) const {
            try {
                throw; // This will tell us which exception triggered the call to this error handler.
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_WARN << "Reason '" << reason << "' causes '" << getInstanceId()
                        << "' to kill itself after exception: " << e.what();
            }
            call("", "slotKillDevice");
        }


        void DataLogger::slotTagDeviceToBeDiscontinued(const bool wasValidUpToNow, const char reason) {
            KARABO_LOG_FRAMEWORK_DEBUG << "slotTagDeviceToBeDiscontinued " << wasValidUpToNow << " '" << reason << "'";
            try {
                boost::mutex::scoped_lock lock(m_configMutex);
                if (m_configStream.is_open()) {
                    m_configStream << m_lastDataTimestamp.toIso8601Ext() << "|" << fixed << m_lastDataTimestamp.toTimestamp()
                            << "|" << m_lastDataTimestamp.getTrainId() << "|.|||" << m_user << "|LOGOUT\n";
                    long position = m_configStream.tellp();
                    m_configStream.close();
                    string contentPath = get<string>("directory") + "/" + m_deviceToBeLogged + "/raw/archive_index.txt";
                    ofstream contentStream(contentPath.c_str(), ios::app);
                    contentStream << "-LOG " << m_lastDataTimestamp.toIso8601Ext() << " " << fixed << m_lastDataTimestamp.toTimestamp()
                            << " " << m_lastDataTimestamp.getTrainId() << " " << position << " " << (m_user.empty() ? "." : m_user) << " " << m_lastIndex << "\n";
                    contentStream.close();
                    //KARABO_LOG_FRAMEWORK_DEBUG << "slotTagDeviceToBeDiscontinued index stream closed";

                    for (map<string, MetaData::Pointer>::iterator it = m_idxMap.begin(); it != m_idxMap.end(); it++) {
                        MetaData::Pointer mdp = it->second;
                        if (mdp && mdp->idxStream.is_open()) mdp->idxStream.close();
                    }
                    m_idxMap.clear();
                    //KARABO_LOG_FRAMEWORK_DEBUG << "slotTagDeviceToBeDiscontinued idxMap is cleaned";
                    disconnectP2P(m_deviceToBeLogged);
                }
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_LOGIC_EXCEPTION("Problems tagging " + m_deviceToBeLogged + " to be discontinued"));
            }
        }


        void DataLogger::slotChanged(const karabo::util::Hash& configuration, const std::string& deviceId) {

            // To write log I need schema ...
            // Copy once under mutex protection and then use copy without further need to think about races
            {
                boost::mutex::scoped_lock lock(m_currentSchemaMutex);
                if (m_currentSchemaChanged) {
                    m_schemaForSlotChanged = m_currentSchema;
                    if (!m_schemaForSlotChanged.empty()) {
                        m_currentSchemaChanged = false;
                    }
                }
            }
            if (m_schemaForSlotChanged.empty()) {
                // DEBUG only since can happen when initialising, i.e. slot is connected, but Schema did not yet arrive.
                KARABO_LOG_FRAMEWORK_DEBUG << getInstanceId() << ": slotChanged called with configuration of size "
                        << configuration.size() << ", but no schema yet - ignore!";
                return;
            }

            if (deviceId != m_deviceToBeLogged) {
                KARABO_LOG_ERROR << "slotChanged called from " << deviceId
                        << ", but logging only " << m_deviceToBeLogged;
                return;
            }

            const bool newPropToIndex = this->updatePropsToIndex();

            // TODO: Define these variables: each of them uses 24 bits
            int expNum = 0x0F0A1A2A;
            int runNum = 0x0F0B1B2B;

            m_user = getSenderInfo("slotChanged")->getUserIdOfSender();
            if (m_user.size() == 0) m_user = "";

            vector<string> paths;
            getLeaves(configuration, m_schemaForSlotChanged, paths);
            
            boost::mutex::scoped_lock lock(m_configMutex);
            if (newPropToIndex) {
                // DataLogReader got request for history of a property not indexed
                // so far, that means it triggered the creation of an index file
                // for that property. Since we cannot be sure that the index creation
                // has finished when we want to add a new index entry, we close the
                // file and thus won't touch this new index file (but start a new one).
                this->ensureFileClosed(); // must be protected by m_configMutex
            }

            for (size_t i = 0; i < paths.size(); ++i) {
                const string& path = paths[i];
                
                // Skip those elements which should not be archived
                if (!m_schemaForSlotChanged.has(path)
                    || (m_schemaForSlotChanged.hasArchivePolicy(path) && (m_schemaForSlotChanged.getArchivePolicy(path) == Schema::NO_ARCHIVING))) {
                    continue;
                }

                const Hash::Node& leafNode = configuration.getNode(path);
                if (leafNode.getType() == Types::HASH) continue;

                // Check for timestamp ...
                if (!Timestamp::hashAttributesContainTimeInformation(leafNode.getAttributes())) {
                    KARABO_LOG_WARN << "Skip '" << path << "' - it lacks time information attributes.";
                    continue;
                }

                Timestamp t = Timestamp::fromHashAttributes(leafNode.getAttributes());
                m_lastDataTimestamp = t;
                string type = Types::to<ToLiteral>(leafNode.getType());
                string value = "";   // "value" should be a string, so convert depending on type ...
                if (leafNode.getType() == Types::VECTOR_HASH) {
                    // Represent any vector<Hash> as XML string ...
                    TextSerializer<Hash>::Pointer serializer = TextSerializer<Hash>::create(Hash("Xml.indentation", -1));
                    serializer->save(leafNode.getValue<vector<Hash>>(), value);
                } else if (Types::isVector(leafNode.getType())) {
                    // ... and any other vector as a comma separated text string of vector elements
                    value = toString(leafNode.getValueAs<string,vector>());
                } else {
                    value = leafNode.getValueAs<string>();
                }

                bool newFile = false;
                if (!m_configStream.is_open()) {
                    string configName = get<string>("directory") + "/" + deviceId + "/raw/archive_" + toString(m_lastIndex) + ".txt";
                    m_configStream.open(configName.c_str(), ios::out | ios::app);
                    if (!m_configStream.is_open()) {
                        KARABO_LOG_ERROR << "Failed to open \"" << configName << "\". Check permissions.";
                        return;
                    }
                    if (m_configStream.tellp() > 0) {
                        // Make sure that the file contains '\n' (newline) at the end of previous round
                        m_configStream << '\n';
                    } else
                        newFile = true;
                }

                size_t position = m_configStream.tellp(); // get current file size

                m_configStream << t.toIso8601Ext() << "|" << fixed << t.toTimestamp() << "|"
                        << t.getTrainId() << "|" << path << "|" << type << "|" << scientific
                        << value << "|" << m_user;
                if (m_pendingLogin) m_configStream << "|LOGIN\n";
                else m_configStream << "|VALID\n";

                if (m_pendingLogin || newFile) {
                    string contentPath = get<string>("directory") + "/" + deviceId + "/raw/archive_index.txt";
                    ofstream contentStream(contentPath.c_str(), ios::app);
                    if (m_pendingLogin) {
                        contentStream << "+LOG ";
                    } else {
                        contentStream << "=NEW ";
                    }

                    contentStream << t.toIso8601Ext() << " " << fixed << t.toTimestamp() << " "
                            << t.getTrainId() << " " << position << " " << (m_user.empty() ? "." : m_user) << " " << m_lastIndex << "\n";
                    contentStream.close();

                    m_pendingLogin = false;
                }

                // check if we have property registered
                if (find(m_idxprops.begin(), m_idxprops.end(), path) == m_idxprops.end()) continue;

                // m_configMutex (for use of m_idxMap) already locked above
                MetaData::Pointer& mdp = m_idxMap[path]; //Pointer by reference!
                bool first = false;
                if (!mdp) {
                    // a property not yet indexed - create meta data and set file
                    mdp = MetaData::Pointer(new MetaData);
                    mdp->idxFile = get<string>("directory") + "/" + deviceId + "/idx/archive_" + toString(m_lastIndex)
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
            long position = m_configStream.tellp();
            if (maxFilesize <= position) {
                this->ensureFileClosed();
            }
        }


        bool DataLogger::updatePropsToIndex() {
            boost::filesystem::path propPath(get<string>("directory") + "/" + m_deviceToBeLogged + "/raw/properties_with_index.txt");
            if (boost::filesystem::exists(propPath)) {
                const size_t propsize = boost::filesystem::file_size(propPath);
                const time_t lasttime = boost::filesystem::last_write_time(propPath);
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
                    // m_idxprops now really differs from old content...
                    return true;
                }
            }
            return false;
        }


        void DataLogger::ensureFileClosed() {
            // We touch m_configStream and m_idxMap. Thus we have to rely that the
            // code calling this method is protected by the 'm_configMutex'
            // (as requested by documentation).
            if (m_configStream.is_open()) {
                // increment index number for configuration file
                m_lastIndex = this->incrementLastIndex(m_deviceToBeLogged);
                m_configStream.close();
            }

            for (std::map<std::string, MetaData::Pointer>::iterator it = m_idxMap.begin(), endIt = m_idxMap.end();
                 it != endIt; ++it) {
                MetaData::Pointer mdp = it->second;
                if (mdp && mdp->idxStream.is_open()) mdp->idxStream.close();
            }
            m_idxMap.clear();
        }


        void DataLogger::flush() {
            // If the related asynchronous operation cannot be cancelled, the flush might already be running
            if (m_flushDeadline.cancel()) {
                doFlush();
                m_flushDeadline.expires_from_now(boost::posix_time::seconds(m_flushInterval));
                m_flushDeadline.async_wait(util::bind_weak(&DataLogger::flushActor, this, boost::asio::placeholders::error));
            }
        }


        void DataLogger::flushActor(const boost::system::error_code& e) {
            if (e == boost::asio::error::operation_aborted || !m_doFlushFiles)
                return;
            doFlush();
            // arm timer again
            m_flushDeadline.expires_from_now(boost::posix_time::seconds(m_flushInterval));
            m_flushDeadline.async_wait(util::bind_weak(&DataLogger::flushActor, this, boost::asio::placeholders::error));
        }


        void DataLogger::doFlush() {
            boost::mutex::scoped_lock lock(m_configMutex);
            if (m_configStream.is_open()) {
                m_configStream.flush();
            }
            for (map<string, MetaData::Pointer>::iterator it = m_idxMap.begin(); it != m_idxMap.end(); it++) {
                MetaData::Pointer mdp = it->second;
                if (mdp && mdp->idxStream.is_open()) mdp->idxStream.flush();
            }
        }


        void DataLogger::slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId) {
            KARABO_LOG_FRAMEWORK_DEBUG << "slotSchemaUpdated: Schema for " << deviceId << " arrived...";
            {
                boost::mutex::scoped_lock lock(m_currentSchemaMutex);
                m_currentSchema = schema;
                m_currentSchemaChanged = true;
            }
            string filename = get<string>("directory") + "/" + deviceId + "/raw/archive_schema.txt";
            fstream fileout(filename.c_str(), ios::out | ios::app);
            if (fileout.is_open()) {
                Timestamp t;
                TextSerializer<Schema>::Pointer serializer = TextSerializer<Schema>::create(Hash("Xml.indentation", -1));
                string archive;
                serializer->save(schema, archive);
                fileout << t.getSeconds() << " " << t.getFractionalSeconds() << " " << t.getTrainId() << " " << archive << "\n";
                fileout.close();
            } else
                throw KARABO_IO_EXCEPTION("Failed to open \"" + filename + "\". Check permissions.");
        }


        int DataLogger::determineLastIndex(const std::string& deviceId) {
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


        int DataLogger::incrementLastIndex(const std::string& deviceId) {
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


    }
}
