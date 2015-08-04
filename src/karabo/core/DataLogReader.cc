#include <map>
#include <boost/algorithm/string.hpp>
#include <karabo/io/Input.hh>
#include "DataLogReader.hh"
#include "karabo/io/FileTools.hh"

#define DATALOGGER_PREFIX "DataLogger-"
#define DATALOGREADER_PREFIX "DataLogReader-"

namespace bf = boost::filesystem;
namespace bs = boost::system;

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
                TimeProfiler p("processingForTrendline");
                p.open();

                p.startPeriod("reaction");

                KARABO_LOG_FRAMEWORK_DEBUG << "slotGetPropertyHistory(" << deviceId << ", " << property << ", from/to parameters)";

                vector<Hash> result;

                Epochstamp from;
                if (params.has("from")) from = Epochstamp(params.get<string>("from"));
                Epochstamp to;
                if (params.has("to")) to = Epochstamp(params.get<string>("to"));
                unsigned int maxNumData = 0;
                if (params.has("maxNumData")) maxNumData = params.getAs<int>("maxNumData");

                int lastFileIndex = getFileIndex(deviceId);
                if (lastFileIndex < 0) {
                    KARABO_LOG_WARN << "File \"" << get<string>("directory") << "/raw/" << deviceId << ".last\" not found. No data will be sent...";
                    reply(deviceId, property, result);
                    return;
                }

                KARABO_LOG_FRAMEWORK_DEBUG << "From (UTC): " << from.toIso8601Ext();
                KARABO_LOG_FRAMEWORK_DEBUG << "To (UTC):   " << to.toIso8601Ext();

                p.startPeriod("findingNearestIndex");
                DataLoggerIndex idxFrom = findNearestLoggerIndex(deviceId, from);
                DataLoggerIndex idxTo = findNearestLoggerIndex(deviceId, to);
                p.stopPeriod("findingNearestIndex");

                KARABO_LOG_FRAMEWORK_DEBUG << "Event: \"" << idxFrom.m_event << "\", epoch: " << idxFrom.m_epoch.toIso8601Ext()
                        << ", pos: " << idxFrom.m_position << ", fileindex: " << idxFrom.m_fileindex;
                if (idxFrom.m_fileindex == -1) {
                    KARABO_LOG_WARN << "Requested time point \"" << params.get<string>("from") << "\" for device configuration is earlier than anything logged";
                    reply(deviceId, property, result);
                    return;
                }

                p.startPeriod("navigation");
                MetaSearchResult msr = navigateMetaRange(deviceId, idxFrom.m_fileindex, idxTo.m_fileindex, property, from, to);
                p.stopPeriod("navigation");

                KARABO_LOG_FRAMEWORK_DEBUG << "MetaSearchResult: from : filenum=" << msr.fromFileNumber << " record=" << msr.fromRecord
                        << ", to : filenum=" << msr.toFileNumber << " record=" << msr.toRecord << ", list: " << toString(msr.nrecList);

                size_t ndata = 0;
                for (vector<size_t>::iterator it = msr.nrecList.begin(); it != msr.nrecList.end(); it++) ndata += *it;
                size_t reductionFactor = (ndata + maxNumData - 1) / maxNumData;

                KARABO_LOG_FRAMEWORK_DEBUG << "slotGetPropertyHistory: total " << ndata << " data points and reductionFactor : " << reductionFactor;

                if (ndata) {

                    p.startPeriod("collection");

                    ifstream df;
                    ifstream mf;
                    size_t indx = 0;
                    size_t idxpos = msr.fromRecord;
                    for (size_t fnum = msr.fromFileNumber, ii = 0; fnum <= msr.toFileNumber && ii < msr.nrecList.size(); fnum++, ii++) {
                        if (mf.is_open()) mf.close();
                        if (df.is_open()) df.close();
                        string idxname = get<string>("directory") + "/idx/" + deviceId + "_configuration_" + toString(fnum) + "-" + property + "-index.bin";
                        string dataname = get<string>("directory") + "/raw/" + deviceId + "_configuration_" + toString(fnum) + ".txt";
                        if (!bf::exists(bf::path(idxname))) continue;
                        if (!bf::exists(bf::path(dataname))) continue;
                        mf.open(idxname.c_str(), ios::in | ios::binary);
                        df.open(dataname.c_str());
                        if (!mf.is_open() || !df.is_open()) continue;
                        if (ii != 0) idxpos = 0;

                        //                    KARABO_LOG_FRAMEWORK_DEBUG << "slotGetPropertyHistory: property : \"" << property << "\", fileNumber : " << fnum
                        //                                               << ", ii=" << ii << ", idx start position=" << idxpos;

                        mf.seekg(idxpos * sizeof (MetaData::Record), ios::beg);
                        for (size_t i = 0; i < msr.nrecList[ii]; i++) {
                            MetaData::Record record;
                            mf.read((char*) &record, sizeof (MetaData::Record));
                            if ((indx++ % reductionFactor) != 0 && (record.extent2 & (1 << 30)) == 0)
                                continue; // skip data point

                            df.seekg(record.positionInRaw, ios::beg);
                            string line;
                            if (getline(df, line)) {
                                if (line.empty()) continue;
                                vector<string> tokens;
                                boost::split(tokens, line, boost::is_any_of("|"));
                                if (tokens.size() != 10) {
                                    KARABO_LOG_FRAMEWORK_DEBUG << "slotGetPropertyHistory: skip corrupted record: token.size() = " << tokens.size();
                                    continue; // This record is corrupted -- skip it
                                }

                                //df >> timestampAsIso8601 >> timestampAsDouble >> seconds >> fraction >> trainId >> path >> type >> value >> user >> flag;

                                const string& flag = tokens[9];
                                if ((flag == "LOGIN" || flag == "LOGOUT") && result.size() > 0) {
                                    result[result.size() - 1].setAttribute("v", "isLast", 'L');
                                }

                                const string& path = tokens[5];
                                if (path != property)
                                    throw KARABO_PARAMETER_EXCEPTION("Wrong property read in raw file");

                                Hash hash;
                                const string& type = tokens[6];
                                const string& value = tokens[7];
                                Hash::Node& node = hash.set<string>("v", value);
                                node.setType(Types::from<FromLiteral>(type));

                                unsigned long long trainId = fromString<unsigned long long>(tokens[4]);
                                unsigned long long seconds = fromString<unsigned long long>(tokens[2]);
                                unsigned long long fraction = fromString<unsigned long long>(tokens[3]);
                                Timestamp tst(Epochstamp(seconds, fraction), Trainstamp(trainId));
                                Hash::Attributes& attrs = hash.getAttributes("v");
                                tst.toHashAttributes(attrs);
                                result.push_back(hash);
                            }
                            //else {
                            //    KARABO_LOG_FRAMEWORK_DEBUG << "slotGetPropertyHistory: getline returns FALSE";
                            //}
                        }
                    }
                    if (mf.is_open()) mf.close();
                    if (df.is_open()) df.close();

                    p.stopPeriod("collection");
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
                bf::path schemaPath(get<string>("directory") + "/raw/" + deviceId + "_schema.txt");
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
                    KARABO_LOG_WARN << "File \"" << get<string>("directory") << "/raw/" << deviceId << ".last\" not found. No data will be sent...";
                    return;
                }

                {
                    Epochstamp current(0, 0);
                    long position = index.m_position;
                    for (int i = index.m_fileindex; i <= lastFileIndex && current <= target; i++, position = 0) {
                        string filename = get<string>("directory") + "/raw/" + deviceId + "_configuration_" + karabo::util::toString(i) + ".txt";
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

            string contentpath = get<string>("directory") + "/raw/" + deviceId + "_index.txt";
            if (!bf::exists(bf::path(contentpath)))
                return entry;

            ifstream ifs(contentpath.c_str());
            while (ifs >> event >> timestampAsIso8061 >> timestampAsDouble >> seconds >> fraction) {
                // read the rest of the line (upto '\n')
                string line;
                if (!getline(ifs, line)) {
                    ifs.close();
                    throw KARABO_IO_EXCEPTION("Premature EOF while reading index file \"" + contentpath + "\"");
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

            string contentpath = get<string>("directory") + "/raw/" + deviceId + "_index.txt";
            if (!bf::exists(bf::path(contentpath))) return nearest;
            ifstream contentstream(contentpath.c_str());

            while (contentstream >> event >> timestampAsIso8061 >> timestampAsDouble >> seconds >> fraction) {
                // read the rest of the line (upto '\n')
                string line;
                if (!getline(contentstream, line)) {
                    contentstream.close();
                    KARABO_LOG_WARN << "Premature EOF while reading index file \"" << contentpath + "\" in findNearestLoggerIndex";
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
            contentstream.close();
            return nearest;
        }


        int DataLogReader::getFileIndex(const std::string& deviceId) {
            string filename = get<string>("directory") + "/raw/" + deviceId + ".last";
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
            size_t endnum = getFileIndex(deviceId);

            result.fromFileNumber = startnum;
            result.toFileNumber = tonum;
            result.nrecList.clear();

            double from = efrom.toTimestamp();
            double to = eto.toTimestamp();

            //KARABO_LOG_FRAMEWORK_DEBUG << "navigateMetaRange for \"" << deviceId << "\", startnum: " << startnum
            //        << ", endnum: " << endnum << ", for property \""
            //        << path << "\", in range from " << efrom.toIso8601Ext() << " (" << fixed << from << ")"
            //        << " to " << eto.toIso8601Ext() << " (" << fixed << to << ")";

            if (endnum < startnum)
                throw KARABO_PARAMETER_EXCEPTION("start file number greater than end file number.");

            ifstream f;
            size_t fnum = startnum;
            double epochLeft = 0.0;
            double epochRight = 0.0;
            double firstEpochInFile = 0.0;
            double lastEpochInFile = 0.0;
            size_t recLeft = 0;
            size_t recRight = 0;
            size_t nrecs = 0;
            size_t filesize = 0;
            string fname = "";
            bs::error_code ec;

            // Find record number of "from" in index file ..
            for (; fnum <= tonum; fnum++) {
                if (f.is_open()) f.close();

                fname = get<string>("directory") + "/idx/" + deviceId + "_configuration_" + toString(fnum) + "-" + path + "-index.bin";
                f.open(fname.c_str(), ios::in | ios::binary | ios::ate);
                if (!f.is_open()) continue;
                filesize = f.tellg();
                nrecs = filesize / sizeof (MetaData::Record);
                assert(filesize % sizeof (MetaData::Record) == 0);

                // read first record
                f.seekg(0, ios::beg);
                f.read((char*) &record, sizeof (MetaData::Record));
                epochLeft = firstEpochInFile = record.epochstamp;
                recLeft = 0;

                if (ROUND1MS(from) <= ROUND1MS(epochLeft)) {
                    result.fromFileNumber = fnum;
                    result.fromRecord = recLeft;
                    break;
                }

                // read last record
                f.seekg(filesize - sizeof (MetaData::Record), ios::beg);
                f.read((char*) &record, sizeof (MetaData::Record));
                epochRight = lastEpochInFile = record.epochstamp;
                recRight = nrecs - 1;
                if (ROUND1MS(from) == ROUND1MS(epochRight)) {
                    result.fromFileNumber = fnum;
                    result.fromRecord = recRight;
                    break;
                }

                if (from > epochRight) continue;

                // epochLeft < from < epochRight
                result.fromFileNumber = fnum;
                result.fromRecord = findPositionOfEpochstamp(f, from, recLeft, recRight);
                break;
            }

            // Try to find position in the index file of "to" timestamp
            if (fnum == tonum) {
                recLeft = result.fromRecord;
                recRight = nrecs - 1;
                if (ROUND1MS(to) <= ROUND1MS(lastEpochInFile)) {
                    result.toFileNumber = fnum;
                    if (ROUND1MS(to) == ROUND1MS(lastEpochInFile))
                        result.toRecord = nrecs - 1;
                    else
                        result.toRecord = findPositionOfEpochstamp(f, to, recLeft, recRight);
                    result.nrecList.push_back(result.toRecord + 1 - result.fromRecord);
                    if (f.is_open()) f.close();
                    return result;
                }
            }

            fnum++;
            if ((nrecs - result.fromRecord) > 0)
                result.nrecList.push_back(nrecs - result.fromRecord);

            // ... check next files
            for (; fnum <= endnum; fnum++) {
                if (f.is_open()) f.close();

                fname = get<string>("directory") + "/idx/" + deviceId + "_configuration_" + toString(fnum) + "-" + path + "-index.bin";
                filesize = bf::file_size(fname, ec);
                if (ec) continue;
                nrecs = filesize / sizeof (MetaData::Record);
                assert(filesize % sizeof (MetaData::Record) == 0);
                if (fnum < tonum) {
                    result.nrecList.push_back(nrecs);
                    continue;
                }

                f.open(fname.c_str(), ios::in | ios::binary);
                if (!f.is_open()) continue;

                // read first record
                f.read((char*) &record, sizeof (MetaData::Record));
                epochLeft = firstEpochInFile = record.epochstamp;
                recLeft = 0;
                if (ROUND1MS(to) <= ROUND1MS(epochLeft)) {
                    result.toFileNumber = fnum;
                    result.toRecord = 0;
                    result.nrecList.push_back(result.toRecord + 1);
                    break;
                }

                // read last record
                f.seekg(filesize - sizeof (MetaData::Record), ios::beg);
                f.read((char*) &record, sizeof (MetaData::Record));
                epochRight = lastEpochInFile = record.epochstamp;
                recRight = nrecs - 1;
                if (ROUND1MS(to) == ROUND1MS(epochRight)) {
                    result.toFileNumber = fnum;
                    result.toRecord = recRight;
                    result.nrecList.push_back(nrecs);
                    break;
                }
                if (to > epochRight) {
                    // next file
                    result.nrecList.push_back(nrecs);
                    continue;
                }

                // epochLeft < to < epochRight
                result.toFileNumber = fnum;
                result.toRecord = findPositionOfEpochstamp(f, to, recLeft, recRight);
                result.nrecList.push_back(result.toRecord + 1);
                break;
            }
            if (f.is_open()) f.close();
            return result;
        }


        size_t DataLogReader::findPositionOfEpochstamp(ifstream& f, double t, size_t& left, size_t& right) {
            MetaData::Record records[128];
            while ((right - left) > 128) {
                // divide by 2 and check middle point
                size_t recnum = left + (right - left) / 2;
                f.seekg(recnum * sizeof (MetaData::Record));
                f.read((char*) records, sizeof (MetaData::Record));
                double epoch = records[0].epochstamp;
                if (ROUND1MS(t) == ROUND1MS(epoch))
                    return recnum;
                if (t < epoch) {
                    right = recnum;
                } else {
                    left = recnum;
                }
            }

            left++;
            f.seekg(left * sizeof (MetaData::Record));
            f.read((char*) records, (right - left) * sizeof (MetaData::Record));
            for (size_t i = 0; i < (right + 1 - left); i++) {
                double epoch = records[i].epochstamp;
                if (ROUND1MS(t) >= ROUND1MS(epoch)) return (left + i);
            }
            throw KARABO_PARAMETER_EXCEPTION("Epochstamp was not found! Timestamp " + toString(t) + " is wrong!");
        }
    }

#undef ROUND10MS
#undef ROUND1MS
}

