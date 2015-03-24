#include <map>
#include <boost/algorithm/string.hpp>
#include <karabo/io/Input.hh>
#include "DataLogReader.hh"
#include "karabo/io/FileTools.hh"

#define DATALOGGER_PREFIX "DataLogger-"
#define DATALOGREADER_PREFIX "DataLogReader-"

namespace karabo {
    namespace core {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;

        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<karabo::core::OkErrorFsm>, DataLogReader)


        void DataLogReader::expectedParameters(Schema& expected) {

            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue(5)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("archive")
                    .setNewDefaultValue(false)
                    .commit();

            OVERWRITE_ELEMENT(expected).key("heartbeatInterval")
                    .setNewDefaultValue(60)
                    .commit();

            PATH_ELEMENT(expected).key("directory")
                    .displayedName("Directory")
                    .description("The directory where the log files should be placed")
                    .assignmentMandatory()
                    .commit();

        }

        DataLogReader::DataLogReader(const Hash& input) : karabo::core::Device<karabo::core::OkErrorFsm>(input) {
            //set<int>("nThreads", 1);
        }

        DataLogReader::~DataLogReader() {
            KARABO_LOG_INFO << "dead.";
        }

        void DataLogReader::okStateOnEntry() {
            KARABO_SLOT(slotGetPropertyHistory, string /*deviceId*/, string /*key*/, Hash /*params*/);
            KARABO_SLOT(slotGetConfigurationFromPast, string /*deviceId*/, string /*timepoint*/)
        }

        void DataLogReader::slotGetPropertyHistory(const std::string& deviceId, const std::string& property, const Hash& params) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "slotGetPropertyHistory()";

                vector<Hash> result;

                Epochstamp from;
                if (params.has("from")) from = Epochstamp(params.get<string>("from"));
                Epochstamp to;
                if (params.has("to")) to = Epochstamp(params.get<string>("to"));
                unsigned int maxNumData = 0;
                if (params.has("maxNumData")) maxNumData = params.getAs<int>("maxNumData");
                
                int lastFileIndex = getFileIndex(deviceId);
                if (lastFileIndex < 0) {
                    KARABO_LOG_WARN << "File \"" << get<string>("directory") << "/" << deviceId << ".last\" not found. No data will be sent...";
                    reply(deviceId, property, result);
                    return;
                }

                KARABO_LOG_FRAMEWORK_DEBUG << "From (UTC): " << from.toIso8601Ext();
                KARABO_LOG_FRAMEWORK_DEBUG << "To (UTC):   " << to.toIso8601Ext();

                DataLoggerIndex idx = findNearestLoggerIndex(deviceId, from);
                KARABO_LOG_FRAMEWORK_DEBUG << "Event: \"" << idx.m_event << "\", epoch: " << idx.m_epoch.toIso8601Ext()
                        << ", pos: " << idx.m_position << ", fileindex: " << idx.m_fileindex;
                if (idx.m_fileindex == -1) {
                    KARABO_LOG_WARN << "Requested time point \"" << params.get<string>("from") << "\" for device configuration is earlier than anything logged";
                    reply(deviceId, property, result);
                    return;
                }

                KARABO_LOG_FRAMEWORK_DEBUG << "Index found: event: " << idx.m_event
                        << ", epochstamp: " << idx.m_epoch.toIso8601Ext()
                        << ", trainId: " << idx.m_train
                        << ", position: " << idx.m_position
                        << ", user: " << idx.m_user
                        << ", fileindex: " << idx.m_fileindex
                        << ", lastindex: " << lastFileIndex;

                Epochstamp epochstamp(0, 0);
                long position = idx.m_position;

                for (int i = idx.m_fileindex; i <= lastFileIndex && epochstamp <= to; i++, position = 0) {
                    string filename = get<string>("directory") + "/" + deviceId + "_configuration_" + karabo::util::toString(i) + ".txt";
                    if (!boost::filesystem::exists(boost::filesystem::path(filename))) {
                        KARABO_LOG_WARN << "Configuration history file \"" << filename << "\" does not exist. Skip ...";
                        continue;
                    }

                    ifstream ifs(filename.c_str());
                    ifs.seekg(position);

                    string line;
                    while (getline(ifs, line)) {
                        vector<string> tokens;
                        boost::split(tokens, line, boost::is_any_of("|"));
                        if (tokens.size() != 10)
                            continue; // This record is corrupted -- skip it

                        //ifs >> timestampAsIso8601 >> timestampAsDouble >> seconds >> fraction >> trainId >> path >> type >> value >> user >> flag;

                        const string& flag = tokens[9];
                        if ((flag == "LOGIN" || flag == "LOGOUT") && result.size() > 0) {
                            result[result.size() - 1].setAttribute("v", "isLast", 'L');
                        }

                        const string& path = tokens[5];
                        if (path != property)
                            continue;

                        unsigned long long seconds = fromString<unsigned long long>(tokens[2]);
                        unsigned long long fraction = fromString<unsigned long long>(tokens[3]);
                        epochstamp = Epochstamp(seconds, fraction);
                        if (epochstamp > to)
                            break;

                        Hash hash;
                        const string& type = tokens[6];
                        const string& value = tokens[7];
                        Hash::Node& node = hash.set<string>("v", value);
                        node.setType(Types::from<FromLiteral>(type));

                        unsigned long long trainId = fromString<unsigned long long>(tokens[4]);
                        Timestamp timestamp(epochstamp, Trainstamp(trainId));
                        Hash::Attributes& attrs = hash.getAttributes("v");
                        timestamp.toHashAttributes(attrs);
                        result.push_back(hash);
                    }
                    ifs.close();
                }

                // Perform data reduction here if needed
                if (maxNumData && (result.size() > maxNumData)) {
                    int factor = result.size() / maxNumData;
                    // Special case: maxNumData is not even half as small, still skip every 2nd element
                    if (factor == 1) factor = 2;
                    size_t returnSize = result.size() / factor;
                    vector<Hash> reduced;
                    reduced.reserve(returnSize * 1.1); // This fudge factor is intended for the flagged values which have to stay
                    KARABO_LOG_FRAMEWORK_DEBUG << "Reducing data by a factor of " << factor << ". Will return ~" << returnSize << " data points";
                    size_t idx = 0;
                    for (vector<Hash>::const_iterator it = result.begin(); it != result.end(); it++) {
                        if (it->hasAttribute("v", "isLast")) reduced.push_back(*it);
                        else if (idx++ % factor == 0) reduced.push_back(*it);
                    }
                    result.swap(reduced);
                }

                reply(deviceId, property, result);               

            } catch (...) {
                KARABO_RETHROW
            }
        }

        void DataLogReader::slotGetConfigurationFromPast(const std::string& deviceId, const std::string& timepoint) {
            try {

                Hash hash;
                Schema schema;
                Epochstamp target(timepoint);

                KARABO_LOG_FRAMEWORK_DEBUG << "Requested time point: " << target.getSeconds();

                // Retrieve proper Schema
                boost::filesystem::path schemaPath = boost::filesystem::path(get<string>("directory") + "/" + deviceId + "_schema.txt");
                if (boost::filesystem::exists(schemaPath)) {
                    std::ifstream schemastream(schemaPath.string().c_str());
                    unsigned long long seconds;
                    unsigned long long fraction;
                    unsigned long long trainId;
                    string archived;
                    while (schemastream >> seconds >> fraction >> trainId) {
                        Epochstamp current(seconds, fraction);
                        if (current <= target) {
                            archived.clear();
                            if (!getline(schemastream, archived))
                                break;
                        } else
                            break;
                    }
                    schemastream.close();
                    if (archived.empty()) {
                        reply(Hash(), Schema()); // Requested time is before any logger data
                        KARABO_LOG_WARN << "Requested time point for device configuration is earlier than anything logged";
                        return;
                    }
                    TextSerializer<Schema>::Pointer serializer = TextSerializer<Schema>::create("Xml");
                    serializer->load(schema, archived);
                }
                vector<string> paths = schema.getPaths();

                DataLoggerIndex index = findLoggerIndexTimepoint(deviceId, timepoint);

                if (index.m_fileindex == -1 || index.m_event == "-LOG") {
                    reply(Hash(), Schema()); // Requested time is out of any logger data
                    KARABO_LOG_WARN << "Requested time point for device configuration is out of any valid logged data";
                    return;
                }


                int lastFileIndex = getFileIndex(deviceId);
                if (lastFileIndex < 0) {
                    reply(Hash(), Schema());
                    KARABO_LOG_WARN << "File \"" << get<string>("directory") << "/" << deviceId << ".last\" not found. No data will be sent...";
                    return;
                }

                {
                    Epochstamp current(0, 0);
                    long position = index.m_position;
                    for (int i = index.m_fileindex; i <= lastFileIndex && current <= target; i++, position = 0) {
                        string filename = get<string>("directory") + "/" + deviceId + "_configuration_" + karabo::util::toString(i) + ".txt";
                        ifstream file(filename.c_str());
                        file.seekg(position);

                        string line;
                        while (getline(file, line)) {
                            // file >> timestampAsIso8601 >> timestampAsDouble >> seconds >> fraction >> train >> path >> type >> val >> user >> flag;
                            vector<string> tokens;
                            boost::split(tokens, line, boost::is_any_of("|"));

                            if (tokens.size() != 10)
                                continue; // skip corrupted line

                            const string& flag = tokens[9];
                            if (flag == "LOGOUT")
                                break;

                            const string& path = tokens[5];
                            if (!schema.has(path)) continue;

                            unsigned long long seconds = fromString<unsigned long long>(tokens[2]);
                            unsigned long long fraction = fromString<unsigned long long>(tokens[3]);
                            unsigned long long train = fromString<unsigned long long>(tokens[4]);
                            current = Epochstamp(seconds, fraction);
                            if (current > target)
                                break;
                            Timestamp timestamp(current, Trainstamp(train));
                            const string& type = tokens[6];
                            const string& val = tokens[7];
                            Hash::Node& node = hash.set<string>(path, val);
                            node.setType(Types::from<FromLiteral>(type));
                            Hash::Attributes& attrs = node.getAttributes();
                            timestamp.toHashAttributes(attrs);
                        }
                        file.close();
                    }
                }
                reply(hash, schema);
            } catch (...) {
                KARABO_RETHROW
            }
        }

        DataLoggerIndex DataLogReader::findLoggerIndexTimepoint(const std::string& deviceId, const std::string& timepoint) {
            string timestampAsIso8061;
            double timestampAsDouble;
            unsigned long long seconds, fraction;
            string event;
            string tail;
            DataLoggerIndex entry;

            Epochstamp target(timepoint);

            KARABO_LOG_FRAMEWORK_DEBUG << "findLoggerIndexTimepoint: Requested time point: " << timepoint;

            string indexpath = get<string>("directory") + "/" + deviceId + "_index.txt";
            if (!boost::filesystem::exists(boost::filesystem::path(indexpath)))
                return entry;

            ifstream ifs(indexpath.c_str());
            while (ifs >> event >> timestampAsIso8061 >> timestampAsDouble >> seconds >> fraction) {
                // read the rest of the line (upto '\n')
                string line;
                if (!getline(ifs, line)) {
                    ifs.close();
                    throw KARABO_IO_EXCEPTION("Premature EOF while reading index file \"" + indexpath + "\"");
                }

                Epochstamp epochstamp(seconds, fraction);
                if (epochstamp > target) {
                    if (!tail.empty()) {
                        stringstream ss(tail);
                        ss >> entry.m_train >> entry.m_position >> entry.m_user >> entry.m_fileindex;
                    }
                    break;
                } else {
                    // store selected event
                    if (event == "+LOG" || event == "-LOG") {
                        entry.m_event = event;
                        entry.m_epoch = epochstamp;
                        tail = line;
                    }
                }
            }
            ifs.close();
            return entry;
        }

        DataLoggerIndex DataLogReader::findNearestLoggerIndex(const std::string& deviceId, const karabo::util::Epochstamp& target) {
            string timestampAsIso8061;
            double timestampAsDouble;
            unsigned long long seconds, fraction;
            string event;

            DataLoggerIndex nearest;

            string indexpath = get<string>("directory") + "/" + deviceId + "_index.txt";
            if (!boost::filesystem::exists(boost::filesystem::path(indexpath))) return nearest;
            ifstream ifs(indexpath.c_str());

            while (ifs >> event >> timestampAsIso8061 >> timestampAsDouble >> seconds >> fraction) {
                // read the rest of the line (upto '\n')
                string line;
                if (!getline(ifs, line)) {
                    ifs.close();
                    KARABO_LOG_WARN << "Premature EOF while reading index file \"" << indexpath + "\" in findNearestLoggerIndex";
                    return nearest;
                }
                Epochstamp epochstamp(seconds, fraction);
                if (epochstamp <= target || nearest.m_fileindex == -1) {  
                    //in case of time point before target time or there is no record before target time point, hence, use this one
                    nearest.m_event = event;
                    nearest.m_epoch = epochstamp;
                    stringstream ss(line);
                    ss >> nearest.m_train >> nearest.m_position >> nearest.m_user >> nearest.m_fileindex;
                }
                // Stop loop if greater than target time point
                if (epochstamp > target)
                    break;
            }
            ifs.close();
            return nearest;
        }

        int DataLogReader::getFileIndex(const std::string& deviceId) {
            string filename = get<string>("directory") + "/" + deviceId + ".last";
            if (!boost::filesystem::exists(boost::filesystem::path(filename))) return -1;
            ifstream ifs(filename.c_str());
            int idx;
            ifs >> idx;
            ifs.close();
            return idx;
        }
    }
}

