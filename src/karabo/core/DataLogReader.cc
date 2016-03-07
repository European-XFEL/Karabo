#include <map>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <streambuf>
#include <karabo/io/Input.hh>
#include "DataLogReader.hh"
#include "karabo/core/DataLogUtils.hh"
#include "karabo/io/FileTools.hh"
#include "karabo/util/Version.hh"

namespace bf = boost::filesystem;
namespace bs = boost::system;

namespace karabo {
    namespace core {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<karabo::core::OkErrorFsm>, DataLogReader)

        
        IndexBuilderService::Pointer IndexBuilderService::m_instance;
        
        
        IndexBuilderService::Pointer IndexBuilderService::getInstance() {
            if (!m_instance) m_instance.reset(new IndexBuilderService());
            return m_instance;
        }
        
        
        IndexBuilderService::~IndexBuilderService() {
            // Clean up in the destructor which is called when m_instance goes
            // out of scope, i.e. if the program finishes (tested!).
            m_svc.stop();
            m_thread.join();
        }
        
        
        void IndexBuilderService::buildIndexFor(const std::string& commandLineArguments) {
            boost::mutex::scoped_lock lock(m_mutex);
            if (!m_cache.insert(commandLineArguments).second) {
                // such a request is in the queue
               return;
            }
            m_svc.post(boost::bind(&IndexBuilderService::build, this, commandLineArguments));
        }
        
        
        void IndexBuilderService::build(const std::string& commandLineArguments) {
            try {
                const std::string karabo(Version::getPathToKaraboInstallation());
                const std::string command = karabo + "/bin/idxbuild " + commandLineArguments;
                KARABO_LOG_FRAMEWORK_INFO << "********* Index File Building *********\n"
                        << "*** Execute :\n \"" << command << "\"\n***";
                const int ret = system(command.c_str());
                KARABO_LOG_FRAMEWORK_INFO << "*** Index file building command finished with return code " << ret;
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_INFO << "*** Standard Exception in 'build' method : " << e.what();
            }
            // Remove the request to allow another try even if we failed here.
            boost::mutex::scoped_lock lock(m_mutex);
            m_cache.erase(commandLineArguments);
        }
        
        
        void DataLogReader::expectedParameters(Schema& expected) {

            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue(4)
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
            m_ibs = IndexBuilderService::getInstance();
            //set<int>("nThreads", 1);
        }


        DataLogReader::~DataLogReader() {
            KARABO_LOG_FRAMEWORK_DEBUG << this->getInstanceId() << " being destructed.";
        }


        void DataLogReader::okStateOnEntry() {
            KARABO_SLOT(slotGetPropertyHistory, string /*deviceId*/, string /*key*/, Hash /*params*/);
            KARABO_SLOT(slotGetConfigurationFromPast, string /*deviceId*/, string /*timepoint*/)
        }


        void DataLogReader::slotGetPropertyHistory(const std::string& deviceId, const std::string& property, const Hash& params) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "slotGetPropertyHistory(" << deviceId << ", " << property << ", from/to parameters)";
                
                // Safety check that the directory contains something about 'deviceId'
                try {
                    bf::path dirPath(get<string>("directory") + "/" + deviceId + "/raw/");
                    if (!bf::exists(dirPath) || !bf::is_directory(dirPath)) {
                        KARABO_LOG_FRAMEWORK_WARN << "slotGetPropertyHistory: " << dirPath
                                << " not existing or not a directory";
                        // We know nothing about requested 'deviceId', just return empty reply
                        reply(deviceId, property, vector<Hash>());
                        return;
                    }
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "slotGetPropertyHistory Standard Exception while checking deviceId path : " << e.what();
                    reply(deviceId, property, vector<Hash>());
                    return;
                }

                TimeProfiler p("processingForTrendline");
                p.open();

                p.startPeriod("reaction");

                
                bool rebuildIndex = false;
                
                // Register a property in prop file for indexing if it is not there
                try {
                    boost::mutex::scoped_lock lock(m_propFileInfoMutex);
                    bf::path propPath(get<string>("directory") + "/" + deviceId + "/raw/properties_with_index.txt");
                    if (!bf::exists(propPath)) {
                        // create prop file 
                        m_mapPropFileInfo[deviceId] = PropFileInfo::Pointer(new PropFileInfo());
                        //boost::mutex::scoped_lock lock(m_mapPropFileInfo[deviceId]->filelock);
                        ofstream out(propPath.c_str(), ios::out|ios::app);
                        out << property << "\n";
                        out.close();
                        m_mapPropFileInfo[deviceId]->properties.push_back(property);
                        m_mapPropFileInfo[deviceId]->filesize = bf::file_size(propPath);
                        m_mapPropFileInfo[deviceId]->lastwrite= bf::last_write_time(propPath);
                        rebuildIndex = true;
                    } else {
                        // check if the prop file was changed
                        time_t lastTime = bf::last_write_time(propPath);
                        size_t propsize = bf::file_size(propPath);
             
                        map<string, PropFileInfo::Pointer >::iterator mapit = m_mapPropFileInfo.find(deviceId);
                        // check if deviceId is new 
                        if (mapit == m_mapPropFileInfo.end()) {
                            m_mapPropFileInfo[deviceId] = PropFileInfo::Pointer(new PropFileInfo());    // filesize = 0
                            mapit = m_mapPropFileInfo.find(deviceId);
                        } 
                        
                        PropFileInfo::Pointer ptr = mapit->second;
                        
                        if (ptr->filesize != propsize || ptr->lastwrite != lastTime) {
                            // prop file was changed by another thread, so re-read properties ...
                            ptr->properties.clear();
                            string content(propsize, ' ');
                            ifstream in(propPath.c_str());
                            content.assign((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
                            in.close();
                            boost::split(ptr->properties, content, boost::is_any_of("\n"));
                            ptr->filesize = propsize;
                            ptr->lastwrite = lastTime;
                        }
                        
                        if (find(ptr->properties.begin(), ptr->properties.end(), property) == ptr->properties.end()) {
                            // not found, then add to vector
                            ptr->properties.push_back(property);
                            {
                                //boost::mutex::scoped_lock lock(ptr->filelock);
                                ofstream out(propPath.c_str(), ios::out|ios::app);
                                out << property << "\n";
                                out.close();
                            }
                            ptr->filesize = bf::file_size(propPath);
                            ptr->lastwrite= bf::last_write_time(propPath);
                            rebuildIndex = true;
                        }
                    }
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "slotGetPropertyHistory Standard Exception while registering property file : " << e.what();
                    reply(deviceId, property, vector<Hash>());
                    return;
                }

                vector<Hash> result;

                Epochstamp from;
                if (params.has("from")) from = Epochstamp(params.get<string>("from"));
                Epochstamp to;
                if (params.has("to")) to = Epochstamp(params.get<string>("to"));
                unsigned int maxNumData = 0;
                if (params.has("maxNumData")) maxNumData = params.getAs<int>("maxNumData");

                int lastFileIndex = getFileIndex(deviceId);
                if (lastFileIndex < 0) {
                    KARABO_LOG_WARN << "File \"" << get<string>("directory") << "/" << deviceId << "/raw/archive.last\" not found. No data will be sent...";
                    reply(deviceId, property, result);
                    return;
                }
                
                // start rebuilding index for deviceId, property and all files
                if (rebuildIndex) {
                    rebuildIndex = false;
                    int idx = lastFileIndex;
                    while (idx >= 0) {
                        m_ibs->buildIndexFor(get<string>("directory") + " " + deviceId + " " + property + " " + toString(idx));
                        idx--;
                    }
                }

                KARABO_LOG_FRAMEWORK_DEBUG << "From (UTC): " << from.toIso8601Ext();
                KARABO_LOG_FRAMEWORK_DEBUG << "To (UTC):   " << to.toIso8601Ext();

                p.startPeriod("findingNearestIndex");
                DataLoggerIndex idxFrom = findNearestLoggerIndex(deviceId, from, true); // before
                DataLoggerIndex idxTo = findNearestLoggerIndex(deviceId, to, false); // after
                p.stopPeriod("findingNearestIndex");

                KARABO_LOG_FRAMEWORK_DEBUG << "From - Event: \"" << idxFrom.m_event << "\", epoch: " << idxFrom.m_epoch.toIso8601Ext()
                        << ", pos: " << idxFrom.m_position << ", fileindex: " << idxFrom.m_fileindex
                        << ", To - Event: \"" << idxTo.m_event << "\", epoch: " << idxTo.m_epoch.toIso8601Ext()
                        << ", pos: " << idxTo.m_position << ", fileindex: " << idxTo.m_fileindex;
                if (idxFrom.m_fileindex == -1) {
                    KARABO_LOG_WARN << "Requested time point \"" << params.get<string>("from") << "\" for device configuration is earlier than anything logged";
                    reply(deviceId, property, result);
                    return;
                }

                MetaSearchResult msr = navigateMetaRange(deviceId, idxFrom.m_fileindex, idxTo.m_fileindex, property, from, to);

                KARABO_LOG_FRAMEWORK_DEBUG << "MetaSearchResult: from : filenum=" << msr.fromFileNumber << " record=" << msr.fromRecord
                        << ", to : filenum=" << msr.toFileNumber << " record=" << msr.toRecord << ", list: " << toString(msr.nrecList);
                const unsigned int numFiles = (msr.toFileNumber - msr.fromFileNumber + 1);
                if (msr.nrecList.size() != numFiles) {
                    KARABO_LOG_FRAMEWORK_ERROR << "MetaSearchResult mismatch: " << numFiles
                            << ", but list of records has " << msr.nrecList.size() << " entries.";
                }

                // add together the number of data points in all files
                size_t ndata = 0;
                for (vector<size_t>::iterator it = msr.nrecList.begin(); it != msr.nrecList.end(); it++) ndata += *it;
                // reduction factor to skip data points - nothing skipped if zero
                const size_t reductionFactor = (maxNumData ? (ndata + maxNumData - 1) / maxNumData : 0);

                KARABO_LOG_FRAMEWORK_DEBUG << "slotGetPropertyHistory: total " << ndata << " data points and reductionFactor : " << reductionFactor;

                if (ndata) {

                    ifstream df;
                    ifstream mf;
                    size_t indx = 0;
                    size_t idxpos = msr.fromRecord;
                    for (size_t fnum = msr.fromFileNumber, ii = 0; fnum <= msr.toFileNumber && ii < msr.nrecList.size(); fnum++, ii++) {
                        if (mf && mf.is_open()) mf.close();
                        if (df && df.is_open()) df.close();
                        string idxname = get<string>("directory") + "/" + deviceId + "/idx/archive_" + toString(fnum) + "-" + property + "-index.bin";
                        string dataname = get<string>("directory") + "/" + deviceId + "/raw/archive_" + toString(fnum) + ".txt";
                        if (!bf::exists(bf::path(idxname))) {
                            KARABO_LOG_FRAMEWORK_WARN << "Miss file " << idxname;
                            continue;
                        }
                        if (!bf::exists(bf::path(dataname))) {
                            KARABO_LOG_FRAMEWORK_WARN << "Miss file " << dataname;
                            continue;
                        }
                        mf.open(idxname.c_str(), ios::in | ios::binary);
                        df.open(dataname.c_str());
                        if (!mf || !mf.is_open() || !df || !df.is_open()) {
                            KARABO_LOG_FRAMEWORK_WARN << "Either " << dataname << " or "
                                    << idxname << " could not be opened";
                            continue;
                        }
                        if (ii != 0) idxpos = 0;

                        //                    KARABO_LOG_FRAMEWORK_DEBUG << "slotGetPropertyHistory: property : \"" << property << "\", fileNumber : " << fnum
                        //                                               << ", ii=" << ii << ", idx start position=" << idxpos;

                        mf.seekg(idxpos * sizeof (MetaData::Record), ios::beg);
                        for (size_t i = 0; i < msr.nrecList[ii]; i++) {
                            MetaData::Record record;
                            mf.read((char*) &record, sizeof (MetaData::Record));
                            if (reductionFactor && (indx++ % reductionFactor) != 0 && (record.extent2 & (1 << 30)) == 0)
                                continue; // skip data point

                            df.seekg(record.positionInRaw, ios::beg);
                            string line;
                            if (getline(df, line)) {
                                if (line.empty()) continue;
                                vector<string> tokens;
                                boost::split(tokens, line, boost::is_any_of("|"));
                                unsigned int offset = 0;
                                if (tokens.size() != 8) {
                                    if (tokens.size() == 10) {
                                        // old format from 1.4.X: repeats seconds and fractions at indices 2 and 3
                                        offset = 2;
                                    } else {
                                        KARABO_LOG_FRAMEWORK_DEBUG << "slotGetPropertyHistory: skip corrupted record: tokens.size() = " << tokens.size();
                                        continue; // This record is corrupted -- skip it
                                    }
                                }

                                //df >> timestampAsIso8601 >> timestampAsDouble >> seconds >> fraction >> trainId >> path >> type >> value >> user >> flag;

                                const string& flag = tokens[7 + offset];
                                if ((flag == "LOGIN" || flag == "LOGOUT") && result.size() > 0) {
                                    result[result.size() - 1].setAttribute("v", "isLast", 'L');
                                }

                                const string& path = tokens[3 + offset];
                                if (path != property) {
                                    // if you don't like the index record (for example, it pointed to the wrong property) just skip it
                                    KARABO_LOG_FRAMEWORK_WARN << "The index for \"" << deviceId << "\", property : \"" << property
                                            << "\" and file number : " << fnum << " points out to the wrong property in the raw file. Skip it ...";
                                    // TODO: Here we can start index rebuilding for fnum != lastFileIndex
                                    continue;
                                }
                                
                                Hash hash;
                                const string& type = tokens[4 + offset];
                                const string& value = tokens[5 + offset];
                                Hash::Node& node = hash.set<string>("v", value);
                                node.setType(Types::from<FromLiteral>(type));

                                unsigned long long trainId = fromString<unsigned long long>(tokens[2 + offset]);
                                const Epochstamp epochstamp(stringDoubleToEpochstamp(tokens[1]));
                                Timestamp tst(epochstamp, Trainstamp(trainId));
                                Hash::Attributes& attrs = hash.getAttributes("v");
                                tst.toHashAttributes(attrs);
                                result.push_back(hash);
                            }
                            //else {
                            //    KARABO_LOG_FRAMEWORK_DEBUG << "slotGetPropertyHistory: getline returns FALSE";
                            //}
                        }
                    }
                    if (mf && mf.is_open()) mf.close();
                    if (df && df.is_open()) df.close();

                }

                reply(deviceId, property, result);

                p.stopPeriod("reaction");
                p.close();

                KARABO_LOG_FRAMEWORK_DEBUG << "slotGetPropertyHistory: sent " << result.size()
                        << " data points. Request processing time : " << p.getPeriod("reaction").getDuration() << " [s]";

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
                bf::path schemaPath(get<string>("directory") + "/" + deviceId + "/raw/archive_schema.txt");
                if (bf::exists(schemaPath)) {
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
                    KARABO_LOG_WARN << "File \"" << get<string>("directory") << "/" << deviceId << "/raw/archive.last\" not found. No data will be sent...";
                    return;
                }

                {
                    Epochstamp current(0, 0);
                    long position = index.m_position;
                    for (int i = index.m_fileindex; i <= lastFileIndex && current <= target; i++, position = 0) {
                        string filename = get<string>("directory") + "/" + deviceId + "/raw/archive_" + karabo::util::toString(i) + ".txt";
                        ifstream file(filename.c_str());
                        file.seekg(position);

                        string line;
                        while (getline(file, line)) {
                            // file >> timestampAsIso8601 >> timestampAsDouble >> seconds >> fraction >> train >> path >> type >> val >> user >> flag;
                            vector<string> tokens;
                            boost::split(tokens, line, boost::is_any_of("|"));

                            unsigned int offset = 0;
                            if (tokens.size() != 8) {
                                if (tokens.size() == 10) {
                                    // old format from 1.4.X: repeats seconds and fractions at indices 2 and 3
                                    offset = 2;
                                } else {
                                    continue; // skip corrupted line
                                }
                            }

                            const string& flag = tokens[7 + offset];
                            if (flag == "LOGOUT")
                                break;

                            const string& path = tokens[3 + offset];
                            if (!schema.has(path)) continue;
                            current = stringDoubleToEpochstamp(tokens[1]);
                            unsigned long long train = fromString<unsigned long long>(tokens[2]);
                            if (current > target)
                                break;
                            Timestamp timestamp(current, Trainstamp(train));
                            const string& type = tokens[4 + offset];
                            const string& val = tokens[5 + offset];
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
            string timestampAsDouble;
            string event;
            string tail;
            DataLoggerIndex entry;

            Epochstamp target(timepoint);

            KARABO_LOG_FRAMEWORK_DEBUG << "findLoggerIndexTimepoint: Requested time point: " << timepoint;

            string contentpath = get<string>("directory") + "/" + deviceId + "/raw/archive_index.txt";
            if (!bf::exists(bf::path(contentpath)))
                return entry;

            ifstream ifs(contentpath.c_str());
            while (ifs >> event >> timestampAsIso8061 >> timestampAsDouble) {
                // read the rest of the line (upto '\n')
                string line;
                if (!getline(ifs, line)) {
                    ifs.close();
                    throw KARABO_IO_EXCEPTION("Premature EOF while reading index file \"" + contentpath + "\"");
                }
                const Epochstamp epochstamp(stringDoubleToEpochstamp(timestampAsDouble));
                if (epochstamp > target) {
                    if (!tail.empty()) {
                        stringstream ss(tail);
                        unsigned long long dummy;
                        ss >> dummy;
                        // If seconds == dummy, we very likely have old format from 1.4.X where seconds and fractions
                        // follow as ULL after timestampAsDouble - then we have (as usual) train, position, user and index.
                        // In case by chance we have trainId == seconds, we double check that there is a space in front
                        // of each of the six words in the tail (sec, fraction, train, position, user, index):
                        if (epochstamp.getSeconds() == dummy && std::count(tail.begin(), tail.end(), ' ') == 6) {
                                ss >> dummy; // get rid of fractions
                        } else {
                            if (epochstamp.getSeconds() == dummy) {
                                KARABO_LOG_FRAMEWORK_WARN << "findLoggerIndexTimepoint: Value after timestamp as double "
                                        << "equals full seconds (" << dummy << "), i.e. looks like 1.4.X format, but tail of line '"
                                        << tail << "' does not have six words with a space in front of each.";
                            }
                            entry.m_train = dummy;
                        }
                        ss >> entry.m_position >> entry.m_user >> entry.m_fileindex;
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

        DataLoggerIndex DataLogReader::findNearestLoggerIndex(const std::string& deviceId,
                const karabo::util::Epochstamp& target, const bool before) {
            string timestampAsIso8061;
            string timestampAsDouble;
            string event;

            DataLoggerIndex nearest;

            string contentpath = get<string>("directory") + "/" + deviceId + "/raw/archive_index.txt";
            if (!bf::exists(bf::path(contentpath))) return nearest;
            ifstream contentstream(contentpath.c_str());

            bool gotAfter = false;
            while (contentstream >> event >> timestampAsIso8061 >> timestampAsDouble) {
                // read the rest of the line (upto '\n')
                string line;
                if (!getline(contentstream, line)) {
                    contentstream.close();
                    KARABO_LOG_WARN << "Premature EOF while reading index file \"" << contentpath + "\" in findNearestLoggerIndex";
                    return nearest;
                }
                const Epochstamp epochstamp(stringDoubleToEpochstamp(timestampAsDouble));

                if (epochstamp <= target || nearest.m_fileindex == -1 || (!before && !gotAfter)) {
                    // We are here since
                    // 1) target time is larger than current timestamp
                    // 2) or we did not yet have any result
                    // 3) or we search the first line with a timestamp larger than target, but did not yet find it
                    if (!(epochstamp <= target || nearest.m_fileindex == -1)) {
                        // We have case 3 - and will get what we want now.
                        gotAfter = true;
                    }
                    nearest.m_event = event;
                    nearest.m_epoch = epochstamp;
                    stringstream ss(line);
                    unsigned long long dummy;
                    ss >> dummy;
                    // If seconds == dummy, we very likely have old format from 1.4.X where seconds and fractions
                    // follow as ULL after timestampAsDouble - then we have (as usual) train, position, user and index.
                    // In case by chance we have trainId == seconds, we double check that there is a space in front
                    // of each of the six words in the tail (sec, fraction, train, position, user, index):
                    if (epochstamp.getSeconds() == dummy && std::count(line.begin(), line.end(), ' ') == 6) {
                        ss >> dummy; // get rid of fractions
                    } else {
                        if (epochstamp.getSeconds() == dummy) {
                            KARABO_LOG_FRAMEWORK_WARN << "findNearestLoggerIndex: Value after timestamp as double "
                                        << "equals full seconds (" << dummy << "), i.e. looks like 1.4.X format, but tail of "
                                        << "line '" << line << "' does not have six words with spaces in front of each.";
                        }
                        nearest.m_train = dummy;
                    }
                    ss >> nearest.m_position >> nearest.m_user >> nearest.m_fileindex;
                }
                // Stop loop if greater than target time point or we search the first after the target and got it.
                if (epochstamp > target && (before || gotAfter)) break;
            }
            contentstream.close();
            return nearest;
        }


        int DataLogReader::getFileIndex(const std::string& deviceId) {
            string filename = get<string>("directory") + "/" + deviceId + "/raw/archive.last";
            if (!bf::exists(bf::path(filename))) return -1;
            ifstream ifs(filename.c_str());
            int idx;
            ifs >> idx;
            ifs.close();
            return idx;
        }

#define ROUND10MS(x) std::floor(x*100 + 0.5)/100
#define ROUND1MS(x)  std::floor(x*1000 + 0.5)/1000


        MetaSearchResult DataLogReader::navigateMetaRange(const std::string& deviceId, size_t startnum, size_t tonum, const std::string& path,
                                                          const karabo::util::Epochstamp& efrom, const karabo::util::Epochstamp& eto) {
            MetaData::Record record;
            MetaSearchResult result;

            result.toFileNumber = tonum;
            result.nrecList.clear();

            const double from = efrom.toTimestamp();
            const double to = eto.toTimestamp();

            ifstream f;
            size_t fnum = startnum;

            // Find record number of "from" in index file ..
            for (; fnum <= tonum; fnum++) {
                try {
                    if (f && f.is_open()) f.close();
                } catch(const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   " << e.what();
                }

                const std::string fname(get<string>("directory") + "/" + deviceId + "/idx/archive_" + toString(fnum) + "-" + path + "-index.bin");
                size_t filesize = 0;
                try {
                    f.open(fname.c_str(), ios::in | ios::binary | ios::ate);
                    if (!f || !f.is_open()) continue;
                    filesize = f.tellg();
                } catch(const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   " << e.what();
                }
                const size_t nrecs = filesize / sizeof (MetaData::Record);
                assert(filesize % sizeof (MetaData::Record) == 0);

                try {
                    // read last record
                    f.seekg(filesize - sizeof (MetaData::Record), ios::beg);
                    f.read((char*) &record, sizeof (MetaData::Record));
                } catch(const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   " << e.what();
                }
                if (ROUND1MS(from) > ROUND1MS(record.epochstamp)) {
                    // This file is too far in the past - try next if there is one.
                    if (fnum == tonum) {
                        // last time stamp of last file larger than out 'from' => give up!
                        return result; // sum of result.nrecList is 0
                    }
                    continue;
                }

                // 'from' is in this file - look for the exact record and stop loop
                result.fromFileNumber = fnum;
                result.fromRecord = findPositionOfEpochstamp(f, from, 0, nrecs - 1, false);
                break;
            }

            // ... check current and next files for 'to' timestamp
            for (; fnum <= tonum; fnum++) {
                try {
                    if (f && f.is_open()) f.close();
                } catch(const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   " << e.what();
                }

                const std::string fname = get<string>("directory") + "/" + deviceId + "/idx/archive_" + toString(fnum) + "-" + path + "-index.bin";
                // nrecList musty have one entry per file, put 0 and overwrite once we know better
                result.nrecList.push_back(0);
                bs::error_code ec;
                const size_t filesize = bf::file_size(fname, ec);
                if (ec) continue;
                const size_t nrecs = filesize / sizeof (MetaData::Record);
                assert(filesize % sizeof (MetaData::Record) == 0);
                if (fnum < tonum) {
                    result.nrecList.back() = nrecs;
                    continue;
                }
                // Now we know: It is the last file! Find toRecord and number of points until it
                result.toFileNumber = fnum;
                try {
                    f.open(fname.c_str(), ios::in | ios::binary);
                    if (!f || !f.is_open()) continue;
                    //                    // read first record
                    //                    f.read((char*) &record, sizeof (MetaData::Record));
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   " << e.what();
                }

                result.toRecord = findPositionOfEpochstamp(f, to, 0, nrecs - 1, true);
                result.nrecList.back() = result.toRecord + 1;
                break;
            }
            
            try {
                if (f && !f.is_open()) f.close();
            } catch(const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   " << e.what();
            }

            // Subtract records before fromRecords from first entry in list of number of records.
            result.nrecList[0] -= result.fromRecord;
            if (result.nrecList[0] < 0) result.nrecList[0] = 0; //case of trouble with bf::file_size

            return result;
        }


        size_t DataLogReader::findPositionOfEpochstamp(ifstream& f, double t, size_t left, size_t right, bool preferBefore) {
            MetaData::Record records[128];
            const double roundedT = ROUND1MS(t);
            // Recursively narrow the search until at most 128 records are left.
            while ((right - left) > 128) {
                // divide by 2 and check middle point
                const size_t recnum = left + (right - left) / 2;
                try {
                    f.seekg(recnum * sizeof (MetaData::Record));
                    f.read((char*) records, sizeof (MetaData::Record));
                } catch(const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   " << e.what();
                }
                const double epoch = records[0].epochstamp;
                if (ROUND1MS(epoch) == roundedT) {
                    return recnum;
                } else if (t < epoch) {
                    right = recnum;
                } else {
                    left = recnum;
                }
            }

            try {
                // Load all records from left to (including) right:
                f.seekg(left * sizeof (MetaData::Record));
                f.read((char*) records, (right - left + 1) * sizeof (MetaData::Record));
            } catch(const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   " << e.what();
            }
            // Loop and find record with best matching timestamp:
            size_t i = 0;
            for ( ; i <= (right - left); ++i) {
                const double epoch = records[i].epochstamp;
                // In case we never reach the return or break below, the input 'right' is wrong!
                if (ROUND1MS(epoch) == roundedT) {
                    return left + i;
                } else if (epoch > t) {
                    break;
                }
            }
            return left + i - (preferBefore ? 1 : 0);
        }
    }

#undef ROUND10MS
#undef ROUND1MS
}

boost::mutex karabo::core::DataLogReader::m_propFileInfoMutex;
std::map<std::string, karabo::core::PropFileInfo::Pointer > karabo::core::DataLogReader::m_mapPropFileInfo;
