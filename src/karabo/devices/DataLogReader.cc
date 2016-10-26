#include <map>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <streambuf>

#include <boost/algorithm/string.hpp>

#include "karabo/io/Input.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/io/FileTools.hh"
#include "karabo/util/Version.hh"

#include "DataLogReader.hh"

namespace bf = boost::filesystem;
namespace bs = boost::system;

namespace karabo {
    namespace devices {

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


        IndexBuilderService::IndexBuilderService() : m_cache() {
            karabo::net::EventLoop::addThread();
        }


        IndexBuilderService::~IndexBuilderService() {
            // Clean up in the destructor which is called when m_instance goes
            // out of scope, i.e. if the program finishes (tested!).
            karabo::net::EventLoop::removeThread();
        }


        void IndexBuilderService::buildIndexFor(const std::string& commandLineArguments) {
            boost::mutex::scoped_lock lock(m_mutex);
            if (!m_cache.insert(commandLineArguments).second) {
                // such a request is in the queue
                return;
            }
            karabo::net::EventLoop::getIOService().post(bind_weak(&IndexBuilderService::build, this, commandLineArguments));
        }


        void IndexBuilderService::build(const std::string& commandLineArguments) {
            try {
                const std::string command = "karabo-idxbuild " + commandLineArguments;
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
                        ofstream out(propPath.c_str(), ios::out | ios::app);
                        out << property << "\n";
                        out.close();
                        m_mapPropFileInfo[deviceId]->properties.push_back(property);
                        m_mapPropFileInfo[deviceId]->filesize = bf::file_size(propPath);
                        m_mapPropFileInfo[deviceId]->lastwrite = bf::last_write_time(propPath);
                        rebuildIndex = true;
                    } else {
                        // check if the prop file was changed
                        time_t lastTime = bf::last_write_time(propPath);
                        size_t propsize = bf::file_size(propPath);

                        map<string, PropFileInfo::Pointer >::iterator mapit = m_mapPropFileInfo.find(deviceId);
                        // check if deviceId is new 
                        if (mapit == m_mapPropFileInfo.end()) {
                            m_mapPropFileInfo[deviceId] = PropFileInfo::Pointer(new PropFileInfo()); // filesize = 0
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
                                ofstream out(propPath.c_str(), ios::out | ios::app);
                                out << property << "\n";
                                out.close();
                            }
                            ptr->filesize = bf::file_size(propPath);
                            ptr->lastwrite = bf::last_write_time(propPath);
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

                karabo::util::MetaSearchResult msr = navigateMetaRange(deviceId, idxFrom.m_fileindex, idxTo.m_fileindex, property, from, to);

                KARABO_LOG_FRAMEWORK_DEBUG << "MetaSearchResult: from : filenum=" << msr.fromFileNumber << " record=" << msr.fromRecord
                        << ", to : filenum=" << msr.toFileNumber << " record=" << msr.toRecord << ", list: " << toString(msr.nrecList);

                // add together the number of data points in all files
                size_t ndata = 0;
                for (vector<size_t>::iterator it = msr.nrecList.begin(); it != msr.nrecList.end(); it++) ndata += *it;
                // reduction factor to skip data points - nothing skipped if zero
                const size_t reductionFactor = (maxNumData ? (ndata + maxNumData - 1) / maxNumData : 0);

                KARABO_LOG_FRAMEWORK_DEBUG << "slotGetPropertyHistory: total " << ndata << " data points and reductionFactor : " << reductionFactor;

                if (msr.toFileNumber < msr.fromFileNumber) {
                    KARABO_LOG_FRAMEWORK_ERROR << "MetaSearchResult: bad file range " << msr.fromFileNumber
                            << "-" << msr.toFileNumber << ", skip everything.";
                } else if (ndata) {
                    const unsigned int numFiles = (msr.toFileNumber - msr.fromFileNumber + 1);
                    if (msr.nrecList.size() != numFiles) {
                        KARABO_LOG_FRAMEWORK_ERROR << "MetaSearchResult mismatch: " << numFiles
                                << ", but list of records has " << msr.nrecList.size() << " entries.";
                        // Heal as good as we can (nrecList cannot be empty here - ndata would be zero).
                        if (msr.nrecList.size() > numFiles) {
                            msr.nrecList.resize(numFiles);
                        } else if (msr.nrecList.size() < numFiles) {
                            msr.toFileNumber -= (numFiles - msr.nrecList.size());
                        }
                    }

                    // Loop in parallel on index and raw data files:
                    ifstream df;
                    ifstream mf;
                    size_t indx = 0; // counter of processed records in index files
                    for (size_t fnum = msr.fromFileNumber; fnum <= msr.toFileNumber; ++fnum) {
                        if (mf && mf.is_open()) mf.close();
                        if (df && df.is_open()) df.close();
                        const string idxname = get<string>("directory") + "/" + deviceId + "/idx/archive_" + toString(fnum) + "-" + property + "-index.bin";
                        const string dataname = get<string>("directory") + "/" + deviceId + "/raw/archive_" + toString(fnum) + ".txt";
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

                        // Set start position in index file - i.e. file beginning except for first file.
                        const size_t idxpos = (fnum == msr.fromFileNumber ? msr.fromRecord : 0);
                        mf.seekg(idxpos * sizeof (MetaData::Record), ios::beg);
                        // Now loop to read all records in index file and eventually process raw file entries.
                        const size_t numRecords = msr.nrecList[fnum - msr.fromFileNumber];
                        for (size_t iRec = 0; iRec < numRecords; ++iRec) {
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

                                const unsigned long long trainId = fromString<unsigned long long>(tokens[2 + offset]);
                                const Epochstamp epochstamp(stringDoubleToEpochstamp(tokens[1]));
                                const Timestamp tst(epochstamp, Trainstamp(trainId));
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

            const Epochstamp target(timepoint);

            KARABO_LOG_FRAMEWORK_DEBUG << "findLoggerIndexTimepoint: Requested time point: " << timepoint;

            string contentpath = get<string>("directory") + "/" + deviceId + "/raw/archive_index.txt";
            if (!bf::exists(bf::path(contentpath))) {
                KARABO_LOG_FRAMEWORK_WARN << "findLoggerIndexTimepoint: path does not exist: " << contentpath;
                return entry;
            }

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
                    break;
                } else {
                    // store selected event
                    if (event == "+LOG" || event == "-LOG") {
                        entry.m_event = event;
                        entry.m_epoch = epochstamp;
                        tail.swap(line); // store for later usage, but avoid a copy
                    }
                }
            }
            ifs.close();
            if (!tail.empty()) {
                this->extractTailOfArchiveIndex(tail, entry);
            }

            KARABO_LOG_FRAMEWORK_DEBUG << "findLoggerIndexTimepoint - entry: " << entry.m_event << " "
                    << entry.m_position << " " << entry.m_user << " " << entry.m_fileindex;
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
                    this->extractTailOfArchiveIndex(line, nearest);
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


        karabo::util::MetaSearchResult DataLogReader::navigateMetaRange(const std::string& deviceId, size_t startnum, size_t tonum, const std::string& path,
                                                          const karabo::util::Epochstamp& efrom, const karabo::util::Epochstamp& eto) {
            MetaData::Record record;
            MetaSearchResult result;

            const double from = efrom.toTimestamp();
            const double to = eto.toTimestamp();

            // Index file names before and after file number:
            const std::string namePrefix(get<string>("directory") + "/" + deviceId + "/idx/archive_");
            const std::string nameSuffix("-" + path + "-index.bin");
            bool foundFirst = false;

            // Find record number of "from" in index file ..
            for (size_t fnum = startnum; fnum <= tonum; fnum++) {

                ifstream f;
                size_t filesize = 0;
                try {
                    f.open((namePrefix + toString(fnum) + nameSuffix).c_str(), ios::in | ios::binary | ios::ate);
                    if (!f || !f.is_open()) continue;
                    filesize = f.tellg();
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   " << e.what();
                }
                const size_t nrecs = filesize / sizeof (MetaData::Record);
                if (filesize % sizeof (MetaData::Record) != 0) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Index file " << fnum << " for '" << deviceId << "." << path
                            << "' corrupt, skip it.";
                    continue;
                }

                try {
                    // read last record
                    f.seekg(filesize - sizeof (MetaData::Record), ios::beg);
                    f.read((char*) &record, sizeof (MetaData::Record));
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   " << e.what();
                }
                if (ROUND1MS(from) > ROUND1MS(record.epochstamp)) {
                    // This file is too far in the past - try next if there is one.
                    continue;
                }

                // 'from' is in this file - look for the exact record and stop loop
                result.fromFileNumber = fnum;
                result.fromRecord = findPositionOfEpochstamp(f, from, 0, nrecs - 1, false);
                foundFirst = true;
                break;
            }

            if (!foundFirst) {
                return MetaSearchResult();
            }

            bool foundLast = false;
            // Loop backwards (to open as few files as possible) to find file of 'to' timestamp.
            // Loop on 'fnum + 1' to avoid decrementing 0 (of type size_t) which gives a large number and an endless loop
            for (size_t fnumPlus1 = tonum + 1; fnumPlus1 > result.fromFileNumber; --fnumPlus1) {
                result.toFileNumber = fnumPlus1 - 1; // best guess so far - to have for sure a result

                ifstream f;
                try {
                    f.open((namePrefix + toString(result.toFileNumber) + nameSuffix).c_str(), ios::in | ios::binary);
                    if (!f || !f.is_open()) continue;
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   " << e.what();
                }

                try {
                    // read first record
                    f.read((char*) &record, sizeof (MetaData::Record));
                    if (!f) continue; // reading failed (file too short?)
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   " << e.what();
                }

                if (ROUND1MS(record.epochstamp) > ROUND1MS(to)) {
                    continue; // Ignore file: it is completely after range.
                } else {
                    foundLast = true;
                    break;
                }
            }

            if (!foundLast) {
                return MetaSearchResult();
            }

            // Now find number of records in each file. Correct first/last file later.
            for (size_t iFile = result.fromFileNumber; iFile <= result.toFileNumber; ++iFile) {
                // nrecList must have one entry per file, put 0 and overwrite once we know better
                result.nrecList.push_back(0);

                bs::error_code ec;
                const size_t filesize = bf::file_size(namePrefix + toString(iFile) + nameSuffix, ec);
                if (ec) continue;

                const size_t nrecs = filesize / sizeof (MetaData::Record);
                if (filesize % sizeof (MetaData::Record) == 0) {
                    result.nrecList.back() = nrecs;
                } else {
                    KARABO_LOG_FRAMEWORK_ERROR << "Index file " << iFile << " for '" << deviceId << "." << path
                            << "' corrupt, skip its content.";
                }
            }

            // Find toRecord and correct number of records in last file.
            // Minimum possible size of nrecList is 1 here, so calling back() is safe.
            ifstream f;
            try {
                f.open((namePrefix + toString(result.toFileNumber) + nameSuffix).c_str(), ios::in | ios::binary);
                if (!f || !f.is_open()) result.nrecList.back() = 0;
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   " << e.what();
            }
            if (result.nrecList.back()) {
                // Do this before correcting nrecList[0] for fromRecord - first/last could be the same!
                result.toRecord = findPositionOfEpochstamp(f, to, 0, result.nrecList.back() - 1, true);
                result.nrecList.back() = result.toRecord + 1;
            }

            // Subtract records before fromRecords from first entry in list of number of records.
            // Take care of underflows for subtraction between size_t, i.e. unsigned:
            result.nrecList[0] -= std::min(result.fromRecord, result.nrecList[0]);

            return result;
        }


        size_t DataLogReader::findPositionOfEpochstamp(ifstream& f, double t, size_t left, size_t right, bool preferBefore) {
            MetaData::Record records[128];
            const double roundedT = ROUND1MS(t);
            // Recursively narrow the search until at most 128 records are left.
            while ((right - left) >= 128) {
                // divide by 2 and check middle point
                const size_t recnum = left + (right - left) / 2;
                try {
                    f.seekg(recnum * sizeof (MetaData::Record));
                    f.read((char*) records, sizeof (MetaData::Record));
                } catch (const std::exception& e) {
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
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   " << e.what();
            }
            // Loop and find record with best matching timestamp:
            for (size_t i = 0; i <= (right - left); ++i) {
                const double epoch = records[i].epochstamp;
                if (ROUND1MS(epoch) == roundedT) {
                    return left + i;
                } else if (epoch > t) {
                    if (preferBefore && i != 0) {
                        return left + i - 1;
                    } else {
                        return left + i;
                    }
                }
            }

            // No epoch in record range matches 't' or is larger than it. Return end of range.
            return right;
        }


        void DataLogReader::extractTailOfArchiveIndex(const std::string& tail, DataLoggerIndex& entry) const {
            stringstream ss(tail);
            ss >> entry.m_train;
            if (entry.m_epoch.getSeconds() == entry.m_train) {
                // If seconds == train, we very likely have old format from 1.4.X where seconds and fractions
                // follow as ULL after timestampAsDouble - then we have (as usual) train, position, user and index.
                // In case by chance we have trainId == seconds, we double check that there is a space in front
                // of each of the six words in the tail (sec, fraction, train, position, user, index):
                if (std::count(tail.begin(), tail.end(), ' ') == 6) {
                    ss >> entry.m_train >> entry.m_train; // get rid of fractions and fill train with real value
                } else {
                    KARABO_LOG_FRAMEWORK_WARN << "extractTailOfArchiveIndex: Value after timestamp as double equals "
                            << "full seconds (" << entry.m_epoch.getSeconds() << "), i.e. looks like 1.4.X format, "
                            << "but tail of line '" << tail << "' does not have six words with a space in front of each.";
                }
            }
            ss >> entry.m_position >> entry.m_user >> entry.m_fileindex;
        }
    }

#undef ROUND10MS
#undef ROUND1MS
}

boost::mutex karabo::devices::DataLogReader::m_propFileInfoMutex;
std::map<std::string, karabo::devices::PropFileInfo::Pointer > karabo::devices::DataLogReader::m_mapPropFileInfo;
