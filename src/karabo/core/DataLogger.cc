/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <streambuf>
#include <karabo/io/Input.hh>
#include "DataLogger.hh"
#include <karabo/karabo.hpp>
#include <karabo/io/TextSerializer.hh>
#include <karabo/util/Schema.hh>

namespace karabo {  
    namespace core {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<karabo::core::OkErrorFsm>, DataLogger)

        void DataLogger::expectedParameters(Schema& expected) {

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

            INT32_ELEMENT(expected).key("flushInterval")
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
                    .setNewDefaultValue(4)
                    .commit();

            // Slow beats
            OVERWRITE_ELEMENT(expected).key("heartbeatInterval")
                    .setNewDefaultValue(60)
                    .commit();
        }


        DataLogger::DataLogger(const Hash& input) : karabo::core::Device<karabo::core::OkErrorFsm>(input) {
            m_pendingLogin = true;
            m_idxprops.clear();
            m_propsize = 0;
            m_lasttime = 0;

            input.get("deviceToBeLogged", m_deviceToBeLogged);
            // start "flush" thread ...
            m_flushThread = boost::thread(boost::bind(&DataLogger::flushThread, this));
        }


        DataLogger::~DataLogger() {
            // stop and join "flush" thread if it is running...
            if (m_flushThread.joinable()) {
                m_flushThread.interrupt();
                m_flushThread.join();
            }
            slotTagDeviceToBeDiscontinued(true, 'L');
            KARABO_LOG_INFO << "dead.";
        }


        void DataLogger::okStateOnEntry() {

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

            // Register slots
            SLOT2(slotChanged, Hash /*changedConfig*/, string /*deviceId*/);
            SLOT2(slotSchemaUpdated, Schema /*changedSchema*/, string /*deviceId*/);
            SLOT2(slotTagDeviceToBeDiscontinued, bool /*wasValidUpToNow*/, char /*reason*/);

            connect(m_deviceToBeLogged, "signalChanged", "", "slotChanged", NO_TRACK);
            connect(m_deviceToBeLogged, "signalStateChanged", "", "slotChanged", NO_TRACK);
            connect(m_deviceToBeLogged, "signalSchemaUpdated", "", "slotSchemaUpdated", NO_TRACK);

            refreshDeviceInformation();

        }


        void DataLogger::refreshDeviceInformation() {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "refreshDeviceInformation " << m_deviceToBeLogged;
                requestNoWait(m_deviceToBeLogged, "slotGetSchema", "", "slotSchemaUpdated", false);
                requestNoWait(m_deviceToBeLogged, "slotGetConfiguration", "", "slotChanged");
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_INIT_EXCEPTION("Could not create new entry for " + m_deviceToBeLogged));
            }
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
                            << " " << m_lastDataTimestamp.getTrainId() << " " << position << " " << (m_user.empty()?".":m_user) << " " << m_lastIndex << "\n";
                    contentStream.close();
                    //KARABO_LOG_FRAMEWORK_DEBUG << "slotTagDeviceToBeDiscontinued index stream closed";

                    for (map<string, MetaData::Pointer>::iterator it = m_idxMap.begin(); it != m_idxMap.end(); it++) {
                        MetaData::Pointer mdp = it->second;
                        if (mdp && mdp->idxStream.is_open()) mdp->idxStream.close();
                    }
                    m_idxMap.clear();
                    //KARABO_LOG_FRAMEWORK_DEBUG << "slotTagDeviceToBeDiscontinued idxMap is cleaned";
                }
            } catch (...) {
                KARABO_RETHROW_AS(KARABO_LOGIC_EXCEPTION("Problems tagging " + m_deviceToBeLogged + " to be discontinued"));
            }
        }


        void DataLogger::slotChanged(const karabo::util::Hash& configuration, const std::string& deviceId) {

            // To write log I need schema ...
            if (m_currentSchema.empty()) return;

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
            configuration.getPaths(paths);

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
                const Hash::Node& leafNode = configuration.getNode(path);
                if (leafNode.getType() == Types::HASH) continue;
                // Skip those elements which should not be archived
                if (!m_currentSchema.has(path) || (m_currentSchema.hasArchivePolicy(path) && (m_currentSchema.getArchivePolicy(path) == Schema::NO_ARCHIVING))) continue;
                string value = leafNode.getValueAs<string>();
                string type = Types::to<ToLiteral>(leafNode.getType());
                Timestamp t = Timestamp::fromHashAttributes(leafNode.getAttributes());
                m_lastDataTimestamp = t;

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
                        << t.getTrainId() << "|" << path << "|" << type << "|"
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
                            << t.getTrainId() << " " << position << " " << (m_user.empty()? "." : m_user) << " " << m_lastIndex << "\n";
                    contentStream.close();

                    m_pendingLogin = false;
                }

                // check if we have property registered
                if (find(m_idxprops.begin(), m_idxprops.end(), path) == m_idxprops.end()) continue;

                // Check if we need to build index for this property by inspecting schema ... checking only existence
                if (m_currentSchema.has(path)) {
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
            }

            long maxFilesize = get<int>("maximumFileSize") * 1000000; // times to 1000000 because maximumFilesSize in MBytes
            long position = m_configStream.tellp();
            if (maxFilesize <= position) {
                this->ensureFileClosed();
            }
        }

        bool DataLogger::updatePropsToIndex()
        {
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

        void DataLogger::ensureFileClosed()
        {
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

        void DataLogger::flushThread() {
            //------------------------------------------------- make this thread sensible to external interrupts
            boost::this_thread::interruption_enabled(); // enable interruption +
            boost::this_thread::interruption_requested(); // request interruption = we need both!
            try {
                // iterate until interruption
                while (true) {
                    {
                        boost::mutex::scoped_lock lock(m_configMutex);
                        boost::this_thread::disable_interruption di; // disable interruption in this block
                        if (m_configStream.is_open()) {
                            m_configStream.flush();
                        }
                        for (map<string, MetaData::Pointer>::iterator it = m_idxMap.begin(); it != m_idxMap.end(); it++) {
                            MetaData::Pointer mdp = it->second;
                            if (mdp && mdp->idxStream.is_open()) mdp->idxStream.flush();
                        }
                    }
                    // here the interruption enabled again
                    int millis = get<int>("flushInterval") * 1000;
                    boost::this_thread::sleep(boost::posix_time::milliseconds(millis));
                }
            } catch (const boost::thread_interrupted&) {
            }
        }


        void DataLogger::slotSchemaUpdated(const karabo::util::Schema& schema, const std::string& deviceId) {            
            KARABO_LOG_FRAMEWORK_DEBUG << "slotSchemaUpdated: Schema for " << deviceId << " arrived...";
            m_currentSchema = schema;
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
