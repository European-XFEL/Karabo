/*
 * File:   FileLogReader.cc
 *
 * Created on November 8, 2019, 3:40 AM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "FileLogReader.hh"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <cstdlib>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <streambuf>
#include <vector>

#include "DataLogReader.hh"
#include "karabo/core/Device.hh"
#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/time/Epochstamp.hh"
#include "karabo/data/time/TimeDuration.hh"
#include "karabo/data/types/FromLiteral.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/util/TimeProfiler.hh"
#include "karabo/util/Version.hh"

namespace bf = std::filesystem;
namespace bs = boost::system;
KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::Device, karabo::devices::DataLogReader, karabo::devices::FileLogReader)

namespace karabo {
    namespace devices {

        using namespace std;
        using namespace karabo::data;
        using namespace karabo::util;
        using json = nlohmann::json;


        IndexBuilderService::Pointer IndexBuilderService::m_instance;


        IndexBuilderService::Pointer IndexBuilderService::getInstance() {
            if (!m_instance) m_instance.reset(new IndexBuilderService());
            return m_instance;
        }


        IndexBuilderService::IndexBuilderService()
            : m_cache(),
              m_idxBuildStrand(std::make_shared<karabo::net::Strand>(karabo::net::EventLoop::getIOService())) {}


        IndexBuilderService::~IndexBuilderService() {}


        void IndexBuilderService::buildIndexFor(const std::string& commandLineArguments) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_cache.insert(commandLineArguments).second) {
                // such a request is in the queue
                return;
            }
            // Do not post directly to EventLoop to avoid that hundreds of jobs
            // access the disk in parallel
            m_idxBuildStrand->post(bind_weak(&IndexBuilderService::build, this, commandLineArguments));
        }


        void IndexBuilderService::build(const std::string& commandLineArguments) {
            // The 'system' call is blocking, so better add a thread to keep event loop alive.
            karabo::net::EventLoop::addThread();
            try {
                const std::string command = "karabo-idxbuild " + commandLineArguments;
                KARABO_LOG_FRAMEWORK_INFO << "********* Index File Building *********\n"
                                          << "*** Execute :\n \"" << command << "\"\n***";
                const int ret = system(command.c_str());
                KARABO_LOG_FRAMEWORK_INFO << "*** Index file building command finished with return code " << ret;
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_INFO << "*** Standard Exception in 'build' method : " << e.what();
            }
            karabo::net::EventLoop::removeThread(); // ... and remove the thread again

            // Remove the request to allow another try even if we failed here.
            std::lock_guard<std::mutex> lock(m_mutex);
            m_cache.erase(commandLineArguments);
        }

        const std::regex FileLogReader::m_lineRegex(karabo::util::DATALOG_LINE_REGEX);

        const std::regex FileLogReader::m_lineLogRegex(karabo::util::DATALOG_LOGOUT_REGEX);

        const std::regex FileLogReader::m_indexLineRegex(karabo::util::DATALOG_INDEX_LINE_REGEX);

        const std::regex FileLogReader::m_indexTailRegex(karabo::util::DATALOG_INDEX_TAIL_REGEX);


        void FileLogReader::expectedParameters(Schema& expected) {
            STRING_ELEMENT(expected)
                  .key("directory")
                  .displayedName("Directory")
                  .description("The directory where the log files should be placed")
                  .assignmentOptional()
                  .defaultValue("karaboHistory")
                  .commit();
        }


        FileLogReader::FileLogReader(const Hash& input) : karabo::devices::DataLogReader(input) {
            m_serializer = TextSerializer<Hash>::create("Xml");
            m_schemaSerializer = TextSerializer<Schema>::create("Xml");
            m_ibs = IndexBuilderService::getInstance();
        }


        FileLogReader::~FileLogReader() {
            KARABO_LOG_FRAMEWORK_DEBUG << this->getInstanceId() << " being destructed.";
        }


        void FileLogReader::slotGetPropertyHistoryImpl(const std::string& deviceId, const std::string& property,
                                                       const Hash& params) {
            try {
                KARABO_LOG_FRAMEWORK_DEBUG << "slotGetPropertyHistory(" << deviceId << ", " << property
                                           << ", from/to parameters)";

                // Safety check that the directory contains something about 'deviceId'
                try {
                    bf::path dirPath(get<string>("directory") + "/" + deviceId + "/raw/");
                    if (!bf::exists(dirPath) || !bf::is_directory(dirPath)) {
                        KARABO_LOG_FRAMEWORK_WARN << "slotGetPropertyHistory: " << dirPath
                                                  << " not existing or not a directory";
                        throw KARABO_FILENOTFOUND_IO_EXCEPTION(getInstanceId() +
                                                               " misses data directory: " + dirPath.string());
                    }
                } catch (const std::exception& e) {
                    onException("slotGetPropertyHistory Standard Exception while checking deviceId path");
                    throw KARABO_SYSTEM_EXCEPTION(getInstanceId() + " fails accessing raw directory path");
                }

                TimeProfiler p("processingForTrendline");
                p.open();

                p.startPeriod("reaction");


                bool rebuildIndex = false;

                vector<Hash> result;

                int lastFileIndex = getFileIndex(deviceId);

                // Register a property in prop file for indexing if it is not there
                // touching properties_with_index.txt file will cause the DataLogger to close current raw file
                // and increment the content of archive.last
                try {
                    std::lock_guard<std::mutex> lock(m_propFileInfoMutex);
                    bf::path propPath(get<string>("directory") + "/" + deviceId + "/raw/properties_with_index.txt");
                    if (!bf::exists(propPath)) {
                        // create prop file
                        m_mapPropFileInfo[deviceId] = PropFileInfo::Pointer(new PropFileInfo());
                        // std::lock_guard<std::mutex> lock(m_mapPropFileInfo[deviceId]->filelock);
                        ofstream out(propPath.c_str(), ios::out | ios::app);
                        out << property << "\n";
                        out.close();
                        m_mapPropFileInfo[deviceId]->properties.push_back(property);
                        m_mapPropFileInfo[deviceId]->filesize = bf::file_size(propPath);
                        m_mapPropFileInfo[deviceId]->lastwrite = bf::last_write_time(propPath);
                        rebuildIndex = true;
                    } else {
                        // check if the prop file was changed
                        auto lastTime = bf::last_write_time(propPath);
                        size_t propsize = bf::file_size(propPath);

                        map<string, PropFileInfo::Pointer>::iterator mapit = m_mapPropFileInfo.find(deviceId);
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
                                // std::lock_guard<std::mutex> lock(ptr->filelock);
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
                    onException("slotGetPropertyHistory Standard Exception while registering property file");
                    throw KARABO_LOGIC_EXCEPTION(getInstanceId() + " fails registering property file");
                }

                Epochstamp from;
                if (params.has("from")) from = Epochstamp(params.get<string>("from"));
                Epochstamp to;
                if (params.has("to")) to = Epochstamp(params.get<string>("to"));
                unsigned int maxNumData = 0;
                if (params.has("maxNumData")) maxNumData = params.getAs<int>("maxNumData");

                // start rebuilding index for deviceId, property and all files
                if (rebuildIndex) {
                    // We use previously read value of lastFileIndex as we do not want to  trigger rebuilding of the
                    // very last index file, i.e. the one that the DataLogger will start to write from now on!
                    // (See also comment about DataLogReader in DataLogger::slotChanged.)
                    int idx = lastFileIndex;
                    while (idx >= 0) {
                        // files are processed starting from the most recent as we arbitrarily assume user is more
                        // likely interested to recent data
                        m_ibs->buildIndexFor(get<string>("directory") + " " + deviceId + " " + property + " " +
                                             toString(idx));
                        idx--;
                    }
                    throw KARABO_NOT_SUPPORTED_EXCEPTION(getInstanceId() + " cannot fulfill first history request to " +
                                                         deviceId + "." + property +
                                                         ". Try again once index building is done.");
                }

                KARABO_LOG_FRAMEWORK_DEBUG << "From (UTC): " << from.toIso8601Ext();
                KARABO_LOG_FRAMEWORK_DEBUG << "To (UTC):   " << to.toIso8601Ext();

                p.startPeriod("findingNearestIndex");
                FileLoggerIndex idxFrom = findNearestLoggerIndex(deviceId, from, true); // before
                FileLoggerIndex idxTo = findNearestLoggerIndex(deviceId, to, false);    // after
                p.stopPeriod("findingNearestIndex");

                KARABO_LOG_FRAMEWORK_DEBUG
                      << "From - Event: \"" << idxFrom.m_event << "\", epoch: " << idxFrom.m_epoch.toIso8601Ext()
                      << ", pos: " << idxFrom.m_position << ", fileindex: " << idxFrom.m_fileindex << ", To - Event: \""
                      << idxTo.m_event << "\", epoch: " << idxTo.m_epoch.toIso8601Ext() << ", pos: " << idxTo.m_position
                      << ", fileindex: " << idxTo.m_fileindex;

                if (idxFrom.m_fileindex == -1) {
                    const std::string reason("Requested time point '" + params.get<string>("from") +
                                             "' for device configuration is earlier than anything logged");
                    KARABO_LOG_FRAMEWORK_WARN << reason;
                    throw KARABO_LOGIC_EXCEPTION(getInstanceId() + ": " + reason);
                }

                karabo::util::MetaSearchResult msr =
                      navigateMetaRange(deviceId, idxFrom.m_fileindex, idxTo.m_fileindex, property, from, to);

                KARABO_LOG_FRAMEWORK_DEBUG << "MetaSearchResult: from : filenum=" << msr.fromFileNumber
                                           << " record=" << msr.fromRecord << ", to : filenum=" << msr.toFileNumber
                                           << " record=" << msr.toRecord << ", list: " << toString(msr.nrecList);

                // add together the number of data points in all files
                size_t ndata = 0;
                for (vector<size_t>::iterator it = msr.nrecList.begin(); it != msr.nrecList.end(); ++it) ndata += *it;
                // reduction factor to skip data points - nothing skipped if zero
                const size_t reductionFactor = (maxNumData ? (ndata + maxNumData - 1) / maxNumData : 0);

                KARABO_LOG_FRAMEWORK_DEBUG << "slotGetPropertyHistory: total " << ndata
                                           << " data points and reductionFactor : " << reductionFactor;

                if (msr.toFileNumber < msr.fromFileNumber) {
                    KARABO_LOG_FRAMEWORK_ERROR << "MetaSearchResult: bad file range " << msr.fromFileNumber << "-"
                                               << msr.toFileNumber << ", skip everything.";
                } else if (ndata) {
                    const unsigned int numFiles = (msr.toFileNumber - msr.fromFileNumber + 1);
                    if (msr.nrecList.size() != numFiles) {
                        KARABO_LOG_FRAMEWORK_ERROR << "MetaSearchResult mismatch: " << numFiles
                                                   << ", but list of records has " << msr.nrecList.size()
                                                   << " entries.";
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
                        const string idxname = get<string>("directory") + "/" + deviceId + "/idx/archive_" +
                                               toString(fnum) + "-" + property + "-index.bin";
                        const string dataname =
                              get<string>("directory") + "/" + deviceId + "/raw/archive_" + toString(fnum) + ".txt";
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
                            KARABO_LOG_FRAMEWORK_WARN << "Either " << dataname << " or " << idxname
                                                      << " could not be opened";
                            continue;
                        }

                        // Set start position in index file - i.e. file beginning except for first file.
                        const size_t idxpos = (fnum == msr.fromFileNumber ? msr.fromRecord : 0);
                        mf.seekg(idxpos * sizeof(MetaData::Record), ios::beg);
                        // Now loop to read all records in index file and eventually process raw file entries.
                        const size_t numRecords = msr.nrecList[fnum - msr.fromFileNumber];
                        for (size_t iRec = 0; iRec < numRecords; ++iRec) {
                            MetaData::Record record;
                            mf.read((char*)&record, sizeof(MetaData::Record));
                            if (reductionFactor && (indx++ % reductionFactor) != 0 && (record.extent2 & (1 << 30)) == 0)
                                continue; // skip data point

                            df.seekg(record.positionInRaw, ios::beg);
                            string line;
                            if (getline(df, line)) {
                                if (line.empty()) continue;
                                std::smatch tokens;
                                bool search_res = std::regex_search(line, tokens, m_lineRegex);
                                if (!search_res) {
                                    // attempt to parse login/logout line instead
                                    search_res = std::regex_search(line, tokens, m_lineLogRegex);
                                }
                                if (search_res) {
                                    const string& flag = tokens[8];
                                    if ((flag == "LOGIN" || flag == "LOGOUT") && result.size() > 0) {
                                        result[result.size() - 1].setAttribute("v", "isLast", 'L');
                                    }
                                    const string& path = tokens[4];
                                    if (path != property) {
                                        // if you don't like the index record (for example, it pointed to the
                                        // wrong property) just skip it.
                                        KARABO_LOG_FRAMEWORK_WARN
                                              << "The index for \"" << deviceId << "\", property : \"" << property
                                              << "\" and file number : " << fnum
                                              << " points out to the wrong property in the raw file. Skip it ...";
                                        // TODO: Here we can start index rebuilding for fnum != lastFileIndex
                                        continue;
                                    }
                                    const Epochstamp epochstamp(stringDoubleToEpochstamp(tokens[2]));
                                    // tokens[3] is trainId
                                    const Timestamp tst(epochstamp, TimeId(fromString<unsigned long long>(tokens[3])));

                                    if (result.size() == 1) {
                                        // Special case: there's already one history record and it may have a timepoint
                                        // before the requested timeframe. If that's the case, remove that record before
                                        // adding the new one.
                                        const auto& firstRecAttrs = result[0].getAttributes("v");
                                        Epochstamp recEpoch = Epochstamp::fromHashAttributes(firstRecAttrs);
                                        if (recEpoch < from) {
                                            result.clear();
                                        }
                                    }

                                    result.push_back(Hash());
                                    // tokens[5] and [6] are type and value, respectively
                                    readToHash(result.back(), "v", tst, tokens[5], tokens[6]);
                                } else {
                                    KARABO_LOG_FRAMEWORK_DEBUG
                                          << "slotGetPropertyHistory: skip corrupted record or old format '" << line
                                          << "'";
                                }
                            }
                        }
                    }
                    if (mf && mf.is_open()) mf.close();
                    if (df && df.is_open()) df.close();
                }

                reply(deviceId, property, result);
                onOk();

                p.stopPeriod("reaction");
                p.close();

                KARABO_LOG_FRAMEWORK_DEBUG
                      << "slotGetPropertyHistory: sent " << result.size()
                      << " data points. Request processing time : " << p.getPeriod("reaction").getDuration() << " [s]";

            } catch (...) {
                KARABO_RETHROW
            }
        }


        void FileLogReader::slotGetConfigurationFromPastImpl(const std::string& deviceId,
                                                             const std::string& timepoint) {
            // Go directly to event loop to avoid blocking the slot
            AsyncReply aReply(this);
            karabo::net::EventLoop::getIOService().post(
                  bind_weak(&FileLogReader::getConfigurationFromPast, this, deviceId, timepoint, aReply));
        }


        void FileLogReader::getConfigurationFromPast(const std::string& deviceId, const std::string& timepoint,
                                                     SignalSlotable::AsyncReply& aReply) {
            try {
                Hash hash;
                Schema schema;
                const Epochstamp target(timepoint);
                bool configAtTimepoint = false;
                Epochstamp configTimepoint(0, 0); // configTimepoint initialized to the Epoch.

                KARABO_LOG_FRAMEWORK_DEBUG << "Requested time point: " << target;
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
                            if (!getline(schemastream, archived)) break;
                        } else break;
                    }
                    schemastream.close();
                    if (archived.empty()) {
                        KARABO_LOG_FRAMEWORK_WARN << "Requested time point for configuration of '" << deviceId
                                                  << "' is earlier than anything logged";
                        aReply.error("Requested time point for device configuration is earlier than anything logged.");
                        return;
                    }
                    m_schemaSerializer->load(schema, archived);
                } else {
                    KARABO_LOG_FRAMEWORK_WARN << "Schema archive file does not exist: " << schemaPath;
                    aReply.error("Schema archive file does not exist.");
                    return;
                }

                auto result = findLoggerIndexTimepoint(deviceId, timepoint);
                configAtTimepoint = result.first;
                FileLoggerIndex index = result.second;

                if (index.m_fileindex == -1) {
                    KARABO_LOG_FRAMEWORK_WARN << "Requested time point, " << timepoint
                                              << ", precedes any logged data for device '" << deviceId << "'.";
                    aReply.error("Requested time point precedes any logged data.");
                    return;
                } else if (index.m_event != "+LOG") {
                    KARABO_LOG_FRAMEWORK_WARN
                          << "Unexpected event type '" << index.m_event
                          << "' found as for the initial sweeping of last known good configuration.\n"
                             "Event type should be '+LOG ";
                    aReply.error("Unexpected event type '" + index.m_event + "' found - should be '+LOG'.");
                    return;
                }

                int lastFileIndex = getFileIndex(deviceId);

                {
                    Epochstamp current(0, 0);
                    long position = index.m_position;
                    for (int i = index.m_fileindex; i <= lastFileIndex && current <= target; i++) {
                        string filename = get<string>("directory") + "/" + deviceId + "/raw/archive_" +
                                          karabo::data::toString(i) + ".txt";
                        ifstream file(filename.c_str());
                        file.seekg(position);

                        string line;
                        while (getline(file, line)) {
                            std::smatch tokens;
                            bool search_res = std::regex_search(line, tokens, m_lineRegex);
                            if (!search_res) {
                                // attempt to parse login/logout line instead
                                search_res = std::regex_search(line, tokens, m_lineLogRegex);
                            }
                            if (search_res) {
                                const string& flag = tokens[8];
                                if (flag == "LOGOUT") break;
                                const string& path = tokens[4];
                                if (!schema.has(path)) continue;
                                current = stringDoubleToEpochstamp(tokens[2]);
                                if (current > target) break;
                                // configTimepoint is the stamp for the latest logged property value that
                                // precedes the input timepoint.
                                if (current > configTimepoint) {
                                    configTimepoint = current;
                                }
                                // tokens[3] is trainId
                                const Timestamp timestamp(current, fromString<unsigned long long>(tokens[3]));
                                // tokens[5] and [6] are type and value, respectively
                                readToHash(hash, path, timestamp, tokens[5], tokens[6]);
                            } else {
                                KARABO_LOG_FRAMEWORK_DEBUG
                                      << "slotGetPropertyHistory: skip corrupted record or old format: " << line;
                            }
                        }
                        file.close();
                        position = 0; // Puts the cursor at the start of the next log file to be searched.
                    }
                }

                string configTimepointStr(configTimepoint.toIso8601Ext());
                aReply(hash, schema, configAtTimepoint, configTimepointStr);
                onOk();
                KARABO_LOG_FRAMEWORK_DEBUG << "sent result";

            } catch (const std::exception& e) {
                const std::string msg = onException("getConfigurationFromPast error");
                aReply.error(msg);
            }
            KARABO_LOG_FRAMEWORK_DEBUG << "end of slot";
        }


        void FileLogReader::readToHash(Hash& hashOut, const std::string& path, const Timestamp& timestamp,
                                       const std::string& typeString, const string& value) const {
            using karabo::util::DATALOG_NEWLINE_MANGLE;
            Types::ReferenceType type = Types::UNKNOWN;
            std::string unknownError = "";
            try {
                type = Types::from<FromLiteral>(typeString);
            } catch (const ParameterException& e) {
                unknownError.assign(e.what());
            }

            Hash::Node* node = nullptr;

            switch (type) {
                case Types::VECTOR_HASH: {
                    node = &hashOut.set<vector<Hash>>(path, vector<Hash>());
                    // Re-mangle new line characters of any string value inside any of the Hashes,
                    // see DataLogger. But only when needed to avoid copies in "normal" cases.
                    const bool mangle = (value.find(DATALOG_NEWLINE_MANGLE) != std::string::npos);
                    m_serializer->load(
                          node->getValue<vector<Hash>>(),
                          (mangle ? boost::algorithm::replace_all_copy(value, DATALOG_NEWLINE_MANGLE, "\n") : value));
                    break;
                }
                case Types::UNKNOWN: {
                    if (typeString == "VECTOR_STRING_BASE64") {
                        // New format for VECTOR_STRING data.
                        // Convert value (base64) from base64 -> JSON -> vector<string> ...
                        node = &hashOut.set(path, std::vector<std::string>());
                        std::vector<unsigned char> decoded;
                        base64Decode(value, decoded);
                        json j = json::parse(decoded.begin(), decoded.end());
                        for (json::iterator ii = j.begin(); ii != j.end(); ++ii) {
                            node->getValue<std::vector<std::string>>().push_back(*ii);
                        }
                        node->setType(Types::VECTOR_STRING);
                    } else {
                        throw KARABO_PARAMETER_EXCEPTION(unknownError);
                    }
                    break;
                }
                case Types::VECTOR_STRING: {
                    // Old format for VECTOR_STRING data (for backward compatibility)
                    node = &hashOut.set(path, std::vector<std::string>());
                    // Empty value could come from an empty vector of strings or from a vector with a single empty
                    // string. We choose here to interprete as empty vector: It appears more often, e.g. as a default,
                    // and was the interpretation in the past.
                    // This ambiguity and other mangling issues led to the new format.
                    if (!value.empty()) {
                        std::vector<std::string>& valref = node->getValue<std::vector<std::string>>();
                        // Re-mangle new line characters, see DataLogger :-|
                        const std::string unmangled =
                              boost::algorithm::replace_all_copy(value, DATALOG_NEWLINE_MANGLE, "\n");
                        boost::split(valref, unmangled, boost::is_any_of(","));
                    }
                    node->setType(type);
                    break;
                }
#define HANDLE_VECTOR_TYPE(VectorType, ElementType)                                    \
    case Types::VectorType: {                                                          \
        node = &hashOut.set(path, std::vector<ElementType>());                         \
        std::vector<ElementType>& valref = node->getValue<std::vector<ElementType>>(); \
        if (!value.empty()) {                                                          \
            valref = std::move(fromString<ElementType, std::vector>(value, ","));      \
        }                                                                              \
        break;                                                                         \
    }

                    HANDLE_VECTOR_TYPE(VECTOR_BOOL, bool);
                    HANDLE_VECTOR_TYPE(VECTOR_CHAR, char);
                    HANDLE_VECTOR_TYPE(VECTOR_INT8, signed char);
                    HANDLE_VECTOR_TYPE(VECTOR_UINT8, unsigned char);
                    HANDLE_VECTOR_TYPE(VECTOR_INT16, short);
                    HANDLE_VECTOR_TYPE(VECTOR_UINT16, unsigned short);
                    HANDLE_VECTOR_TYPE(VECTOR_INT32, int);
                    HANDLE_VECTOR_TYPE(VECTOR_UINT32, unsigned int);
                    HANDLE_VECTOR_TYPE(VECTOR_INT64, long long);
                    HANDLE_VECTOR_TYPE(VECTOR_UINT64, unsigned long long);
                    HANDLE_VECTOR_TYPE(VECTOR_FLOAT, float);
                    HANDLE_VECTOR_TYPE(VECTOR_DOUBLE, double);
                    HANDLE_VECTOR_TYPE(VECTOR_COMPLEX_FLOAT, std::complex<float>);
                    HANDLE_VECTOR_TYPE(VECTOR_COMPLEX_DOUBLE, std::complex<double>);
#undef HANDLE_VECTOR_TYPE
                case Types::STRING: {
                    node = &hashOut.set(path, value);
                    // Re-mangle new line characters, see DataLogger :-|
                    boost::algorithm::replace_all(node->getValue<string>(), DATALOG_NEWLINE_MANGLE, "\n");
                    break;
                }
                default: {
                    node = &hashOut.set<string>(path, value);
                    node->setType(type);
                }
            }
            Hash::Attributes& attrs = node->getAttributes();
            timestamp.toHashAttributes(attrs);
        }


        std::pair<bool, FileLoggerIndex> FileLogReader::findLoggerIndexTimepoint(const std::string& deviceId,
                                                                                 const std::string& timepoint) {
            string timestampAsIso8061;
            string timestampAsDouble;
            string event;
            FileLoggerIndex lastLogPlusEntry, lastLogMinusEntry;
            string tail;
            bool configAtTimepoint = false;

            const Epochstamp target(timepoint);

            KARABO_LOG_FRAMEWORK_DEBUG << "findLoggerIndexTimepoint: Requested time point: " << timepoint;

            string contentpath = get<string>("directory") + "/" + deviceId + "/raw/archive_index.txt";
            if (!bf::exists(bf::path(contentpath))) {
                KARABO_LOG_FRAMEWORK_WARN << "findLoggerIndexTimepoint: path does not exist: " << contentpath;
                return make_pair(configAtTimepoint, lastLogPlusEntry);
            }

            ifstream ifs(contentpath.c_str());

            unsigned long lineNum = 0;
            string line;
            while (getline(ifs, line)) {
                lineNum++;
                // If any parsing or processing problem happens for the current line, proceed to the next line.
                try {
                    std::smatch indexFields;
                    bool matches = std::regex_search(line, indexFields, m_indexLineRegex);
                    if (!matches) {
                        // The line doesn't have the required values; ignore it and go to the next line.
                        KARABO_LOG_FRAMEWORK_ERROR
                              << "DataLogReader (" << contentpath << ", ln. " << lineNum << "):"
                              << " line should start with an event followed by two white space separated timestamps.";
                        continue;
                    } else {
                        event = indexFields[1];
                        timestampAsIso8061 = indexFields[2];
                        timestampAsDouble = indexFields[3];

                        const Epochstamp epochstamp(stringDoubleToEpochstamp(timestampAsDouble));
                        if (epochstamp > target) {
                            KARABO_LOG_FRAMEWORK_DEBUG << "findLoggerIndexTimepoint: done looping. Line tail:" << tail;
                            break;
                        } else {
                            if (event == "+LOG") {
                                lastLogPlusEntry.m_event = event;
                                lastLogPlusEntry.m_epoch = epochstamp;
                                // store tail for later usage.
                                tail = indexFields[4];
                            } else if (event == "-LOG") {
                                lastLogMinusEntry.m_event = event;
                                lastLogMinusEntry.m_epoch = epochstamp;
                                // There's no need to store the tail for the -LOG event; only its epoch is needed.
                            } // else {
                            // We ignore "=NEW" entries here since we have to read all update lines from the last +LOG
                            // anyway: Otherwise we may miss updates of rarely changing parameters.
                            // }
                        }
                    }
                } catch (const exception& e) {
                    std::ostringstream oss;
                    oss << "FileLogReader (" << contentpath << ", ln. " << lineNum << ")";
                    onException(oss.str());
                    continue;
                }
            }
            ifs.close();

            if (!tail.empty()) {
                try {
                    this->extractTailOfArchiveIndex(tail, lastLogPlusEntry);
                } catch (const exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "DataLogReader - error extracting tail of selected event: "
                                               << e.what();
                }

                // If the tail is not empty, it means a 'device became online event' (LOG+) has been found. If there's
                // no 'device became offline' event (LOG-) that comes after the 'became online' event, it means the
                // device was being logged at the timepoint.
                // FIXME: If -LOG is missing since logger crashed/was killed with -9, we might be fooled here...
                //        But I see no way to fix this since any exact information is lost. Two consecutive +LOG events
                //        (separated by =NEW lines only) are a hint that we _might_ be fooled.
                // NOTE: The -LOG event has the timestamp of the latest update of the device (see ~FileDeviceData() in
                //       FileDataLogger.cc). If now the device was silent for a long time after the last update and then
                //       logging stops,  the timespan between the last update and the stop of logging will erroneously
                //       be considered as 'configAtTimepoint = false'.
                //       But if the logger stores the time point that it stops logging, that may come from the clock
                //       of the data logger machine that might be completely off and searching in the _index.txt file
                //       will not be reliable.
                if (lastLogMinusEntry.m_event.empty() ||
                    (lastLogMinusEntry.m_event == "-LOG" && lastLogMinusEntry.m_epoch < lastLogPlusEntry.m_epoch)) {
                    configAtTimepoint = true;
                }
            }

            KARABO_LOG_FRAMEWORK_DEBUG << "findLoggerIndexTimepoint - entry: " << lastLogPlusEntry.m_event << " "
                                       << lastLogPlusEntry.m_position << " " << lastLogPlusEntry.m_user << " "
                                       << lastLogPlusEntry.m_fileindex;

            return make_pair(configAtTimepoint, lastLogPlusEntry);
        }


        FileLoggerIndex FileLogReader::findNearestLoggerIndex(const std::string& deviceId,
                                                              const karabo::data::Epochstamp& target,
                                                              const bool before) {
            string timestampAsIso8061;
            string timestampAsDouble;
            string event;

            FileLoggerIndex nearest;

            string contentpath = get<string>("directory") + "/" + deviceId + "/raw/archive_index.txt";
            if (!bf::exists(bf::path(contentpath))) return nearest;
            ifstream contentstream(contentpath.c_str());

            bool gotAfter = false;
            unsigned long lineNum = 0;
            string line;

            while (getline(contentstream, line)) {
                lineNum++;
                // If any parsing or processing problem happens for the current line, proceed to the next line.
                try {
                    std::smatch indexFields;
                    bool matches = std::regex_search(line, indexFields, m_indexLineRegex);
                    if (!matches) {
                        // The line doesn't have the expected values; ignore it and go to the next line.
                        KARABO_LOG_FRAMEWORK_ERROR
                              << "DataLogReader (" << contentpath << ", ln. " << lineNum << "): "
                              << "line should start with an event followed by two white space separated timestamps.";
                        continue;
                    } else {
                        event = indexFields[1];
                        timestampAsIso8061 = indexFields[2];
                        timestampAsDouble = indexFields[3];
                        // Stores the rest of the line as the tail contents to be further processed.
                        string tail = indexFields[4];

                        const Epochstamp epochstamp(stringDoubleToEpochstamp(timestampAsDouble));

                        if (epochstamp <= target || nearest.m_fileindex == -1 || (!before && !gotAfter)) {
                            // We are here since
                            // 1) target time is larger than current timestamp
                            // 2) or we did not yet have any result
                            // 3) or we search the first line with a timestamp larger than target,
                            //    but did not yet find it
                            if (!(epochstamp <= target || nearest.m_fileindex == -1)) {
                                // We have case 3 - and will get what we want now.
                                gotAfter = true;
                            }
                            nearest.m_event = event;
                            nearest.m_epoch = epochstamp;
                            this->extractTailOfArchiveIndex(tail, nearest);
                        }

                        // Stop if greater than target time point or we search the first after the target and got it.
                        if (epochstamp > target && (before || gotAfter)) break;
                    }
                } catch (const exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "DataLogReader (" << contentpath << ", ln. " << lineNum
                                               << "): " << e.what();
                    continue;
                }
            }
            contentstream.close();
            return nearest;
        }


        int FileLogReader::getFileIndex(const std::string& deviceId) {
            string filename = get<string>("directory") + "/" + deviceId + "/raw/archive.last";
            if (!bf::exists(bf::path(filename))) {
                KARABO_LOG_FRAMEWORK_WARN << "File \"" << get<string>("directory") << "/" << deviceId
                                          << "/raw/archive.last\" not found.";
                throw KARABO_FILENOTFOUND_IO_EXCEPTION(getInstanceId() + " misses file " + filename);
            }
            ifstream ifs(filename.c_str());
            int idx;
            ifs >> idx;
            ifs.close();
            return idx;
        }

#define ROUND10MS(x) std::floor(x * 100 + 0.5) / 100
#define ROUND1MS(x) std::floor(x * 1000 + 0.5) / 1000


        karabo::util::MetaSearchResult FileLogReader::navigateMetaRange(const std::string& deviceId, size_t startnum,
                                                                        size_t tonum, const std::string& path,
                                                                        const karabo::data::Epochstamp& efrom,
                                                                        const karabo::data::Epochstamp& eto) {
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
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   "
                                               << e.what();
                }
                const size_t nrecs = filesize / sizeof(MetaData::Record);
                if (filesize % sizeof(MetaData::Record) != 0) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Index file " << fnum << " for '" << deviceId << "." << path
                                               << "' corrupt, skip it.";
                    continue;
                }

                try {
                    // read last record
                    f.seekg(filesize - sizeof(MetaData::Record), ios::beg);
                    f.read((char*)&record, sizeof(MetaData::Record));
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   "
                                               << e.what();
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
            // Loop on 'fnum + 1' to avoid decrementing 0 (of type size_t) which gives a large number and an endless
            // loop
            for (size_t fnumPlus1 = tonum + 1; fnumPlus1 > result.fromFileNumber; --fnumPlus1) {
                result.toFileNumber = fnumPlus1 - 1; // best guess so far - to have for sure a result

                ifstream f;
                try {
                    f.open((namePrefix + toString(result.toFileNumber) + nameSuffix).c_str(), ios::in | ios::binary);
                    if (!f || !f.is_open()) continue;
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   "
                                               << e.what();
                }

                try {
                    // read first record
                    f.read((char*)&record, sizeof(MetaData::Record));
                    if (!f) continue; // reading failed (file too short?)
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   "
                                               << e.what();
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

                const size_t nrecs = filesize / sizeof(MetaData::Record);
                if (filesize % sizeof(MetaData::Record) == 0) {
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
                KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   "
                                           << e.what();
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


        size_t FileLogReader::findPositionOfEpochstamp(ifstream& f, double t, size_t left, size_t right,
                                                       bool preferBefore) {
            MetaData::Record records[128];
            const double roundedT = ROUND1MS(t);
            // Recursively narrow the search until at most 128 records are left.
            while ((right - left) >= 128) {
                // divide by 2 and check middle point
                const size_t recnum = left + (right - left) / 2;
                try {
                    f.seekg(recnum * sizeof(MetaData::Record));
                    f.read((char*)records, sizeof(MetaData::Record));
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   "
                                               << e.what();
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
                f.seekg(left * sizeof(MetaData::Record));
                f.read((char*)records, (right - left + 1) * sizeof(MetaData::Record));
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "Standard exception in " << __FILE__ << ":" << __LINE__ << "   :   "
                                           << e.what();
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


        void FileLogReader::extractTailOfArchiveIndex(const std::string& tail, FileLoggerIndex& entry) const {
            // Match tail fields;
            std::smatch tailFields;
            bool matches = std::regex_search(tail, tailFields, m_indexTailRegex);
            if (matches) {
                // Assign tail fields.
                entry.m_train = karabo::data::fromString<unsigned long long>(tailFields[1]);
                entry.m_position = karabo::data::fromString<long>(tailFields[2]);
                entry.m_user = tailFields[3];
                entry.m_fileindex = karabo::data::fromString<int>(tailFields[4]);
            } else {
                throw KARABO_PARAMETER_EXCEPTION("Invalid format in index line tail: \"" + tail + "\".");
            }
        }

    } // namespace devices

#undef ROUND10MS
#undef ROUND1MS

} // namespace karabo

std::mutex karabo::devices::FileLogReader::m_propFileInfoMutex;
std::map<std::string, karabo::devices::PropFileInfo::Pointer> karabo::devices::FileLogReader::m_mapPropFileInfo;
