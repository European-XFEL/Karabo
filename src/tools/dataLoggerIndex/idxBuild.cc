/*
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

#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include "karabo/data/io/TextSerializer.hh"
#include <karabo/util/DataLogUtils.hh>
#include "karabo/data/time/Epochstamp.hh"
#include "karabo/data/types/Exception.hh"
#include "karabo/data/types/Schema.hh"
#include <boost/regex.hpp>
#include <sstream>
#include <vector>

namespace bf = std::filesystem;
using namespace std;
using namespace karabo::util;
using namespace karabo::data;
using namespace karabo::data;

static std::map<string, MetaData::Pointer> idxMap;


struct SchemaHistoryRange {
    unsigned long long fromSeconds;
    unsigned long long fromFraction;
    unsigned long long fromTrainId;
    std::string fromSchemaArchive;
    unsigned long long toSeconds;
    unsigned long long toFraction;
    unsigned long long toTrainId;
    std::string toSchemaArchive;
};

static SchemaHistoryRange schemaRange;

bool byLastFileModificationTime(const bf::path& lhs, const bf::path& rhs);
void processNextFile(const std::string& deviceId, size_t number, const std::string& historyDir, std::ifstream& schema,
                     bool contentFlag, const std::vector<std::string>& properties);


void findDevices(const std::string& root, const std::string& prefix, std::vector<std::string>& devices) {
    bf::path dirpath = root;

    if (!prefix.empty()) dirpath += "/" + prefix;

    if (!bf::exists(dirpath)) return;

    for (bf::directory_iterator it(dirpath); it != bf::directory_iterator(); ++it) {
        // we expect that entry should be a directory
        if (!bf::is_directory(it->path())) continue;

        // build candidate
        string candidate;

        if (prefix.empty()) candidate.assign(it->path().filename().string());
        else candidate.assign(prefix + "/" + it->path().filename().string());

        // check if it fits the requirement(s)
        bf::path archiveLast(root + "/" + candidate + "/raw/archive.last");

        if (bf::exists(archiveLast)) {
            // we found the deviceId!
            devices.push_back(candidate);
        } else {
            // try to dig deeper ...
            findDevices(root, candidate, devices);
        }
    }
}


/*
 *
 */
int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "\nUsage: " << argv[0] << " <karabo_history_dir> [deviceId [property [filenum]]]\n" << endl;
        return 1;
    }

    string karaboHistory(argv[1]);
    string requestedDeviceId = ""; // means "all devices found in karaboHistory"
    string requestedProperty = ""; // means "all registered properties"
    int requestedFilenum = -1;     // means "all file numbers found in raw subdirectory"

    if (argc > 2) {
        requestedDeviceId.assign(argv[2]);
        if (argc > 3) {
            requestedProperty.assign(argv[3]);
            if (argc > 4) requestedFilenum = fromString<int>(string(argv[4]));
        }
    }


    cout << "\nInput parameters are ...\n\tkaraboHistory =\t\"" << karaboHistory << "\"\n"
         << "\tdeviceId =\t\"" << requestedDeviceId << "\"\n"
         << "\tproperty =\t\"" << requestedProperty << "\"\n"
         << "\tfile_num =\t\"" << requestedFilenum << "\"\n"
         << endl;

    bf::path history(karaboHistory);
    if (!bf::exists(history))
        throw KARABO_PARAMETER_EXCEPTION("Directory \"" + history.string() + "\" does not exist!");
    if (!bf::is_directory(history))
        throw KARABO_PARAMETER_EXCEPTION("File \"" + history.string() + "\" is not a directory!");

    // convert to the new directory structure if needed
    //    {
    //        // check if we have old raw directory
    //        bf::path rawdir(karaboHistory + "/raw/");
    //        if (bf::exists(rawdir) && bf::is_directory(rawdir)) {
    //            // build the list of devices
    //            vector<string> devices;
    //            for (bf::directory_iterator it(rawdir); it != bf::directory_iterator(); ++it) {
    //                if (it->path().extension() != ".last") continue;
    //                string deviceId = it->path().stem().string();
    //                devices.push_back(deviceId);
    //            }
    //            // create new dir structure and copy files ...
    //            for (vector<string>::iterator it = devices.begin(); it != devices.end(); ++it) {
    //                string deviceId = *it;
    //                bf::path deviceDir(karaboHistory + "/" + deviceId);
    //                bf::path deviceRawDir(karaboHistory + "/" + deviceId + "/raw/");
    //                if (!bf::exists(deviceDir)) {
    //                    bf::create_directory(deviceDir);
    //                    bf::create_directory(deviceRawDir);
    //                }
    //                for (bf::directory_iterator ii(rawdir); ii != bf::directory_iterator(); ++ii) {
    //                    if (boost::starts_with(ii->path().filename().string(), deviceId)) {
    //                        bf::copy_file(ii->path(), bf::path(deviceRawDir.string() +
    //                        ii->path().filename().string()));
    //                    }
    //                }
    //            }
    //            // remove old raw directory
    //            bf::remove_all(rawdir);
    //        }
    //        // remove old idx directory
    //        bf::path idxdir(karaboHistory + "/idx/");
    //        if (bf::exists(idxdir))
    //            bf::remove_all(idxdir);
    //    }


    vector<string> devices;
    if (requestedDeviceId.empty()) {
        findDevices(karaboHistory, "", devices);
    } else {
        devices.push_back(requestedDeviceId);
    }

    for (vector<string>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
        const string& deviceId = *it;
        bf::path rawdir(karaboHistory + "/" + deviceId + "/raw");
        if (!bf::exists(rawdir))
            throw KARABO_PARAMETER_EXCEPTION("Directory \"" + rawdir.string() + "\" does not exist!");
        if (!bf::is_directory(rawdir))
            throw KARABO_PARAMETER_EXCEPTION("File \"" + rawdir.string() + "\" is not a directory!");

        bf::path idxdir(karaboHistory + "/" + deviceId + "/idx");
    }

    cout << devices.size() << " devices to process found... process only properties that require indexing ..." << endl;

    for (vector<string>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
        string deviceId = *it;

        // cout << "Process the device : \"" << deviceId << "\"" << endl;

        bf::path oldSchemaPath(karaboHistory + "/" + deviceId + "/raw/archive_schema.txt");

        bf::path schemaPath(karaboHistory + "/" + deviceId + "/raw/archive_schema.txt");
        if (!bf::exists(schemaPath)) {
            cout << "WARNING: No schema file found for the device: \"" << deviceId << "\". Skip this device..." << endl;
            continue;
        }

        bf::path rawdir(karaboHistory + "/" + deviceId + "/raw/");

        vector<bf::path> rawtxt;
        for (bf::directory_iterator dit(rawdir); dit != bf::directory_iterator(); ++dit) {
            if (dit->path().extension() != ".txt") continue;
            string filename = dit->path().filename().string();
            // skip archive_index.txt & archive_schema.txt
            if (filename == "archive_index.txt" || filename == "archive_schema.txt") continue;
            string pattern = "archive_";
            if (filename.substr(0, pattern.length()) != pattern) continue;
            // cout << "Filename being processed : " << dit->path().filename() << endl;
            rawtxt.push_back(dit->path());
        }

        // Sort this list by time of last file modification
        sort(rawtxt.begin(), rawtxt.end(), byLastFileModificationTime);

        // rename "content" file
        bf::path cfile(history.string() + "/" + deviceId + "/raw/archive_index.txt");
        bool buildContentFile = false;
        if (!bf::exists(cfile)) buildContentFile = true;

        // Load properties file into vector
        vector<string> idxprops;
        {
            bf::path propPath(history.string() + "/" + deviceId + "/raw/properties_with_index.txt");
            if (bf::exists(propPath)) {
                if (requestedProperty.empty()) {
                    ifstream in(propPath.c_str());
                    in.seekg(0, ios::end);
                    string content(in.tellg(), ' ');
                    in.seekg(0, ios::beg);
                    content.assign((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
                    in.close();
                    boost::split(idxprops, content, boost::is_any_of("\n\t\r "), boost::token_compress_on);
                } else idxprops.push_back(requestedProperty);
            }
        }

        if (!buildContentFile && idxprops.empty()) continue; // nothing to rebuild

        cout << "Process the device : \"" << deviceId << "\"" << endl;

        ifstream sfs(schemaPath.c_str());
        {
            sfs >> schemaRange.fromSeconds >> schemaRange.fromFraction >> schemaRange.fromTrainId;
            sfs.seekg(1, ios::cur); // skip 'space' character
            getline(sfs, schemaRange.fromSchemaArchive, '\n');
        }
        {
            sfs >> schemaRange.toSeconds >> schemaRange.toFraction >> schemaRange.toTrainId;
            sfs.seekg(1, ios::cur);
            getline(sfs, schemaRange.toSchemaArchive, '\n');
        }

        string pattern = "archive_";
        // Process most recent file first as it is most likely what is needed
        // first from user that triggers indexing
        for (vector<bf::path>::reverse_iterator i = rawtxt.rbegin(); i != rawtxt.rend(); ++i) {
            // extract filenum from file name
            int filenum = fromString<size_t>(i->filename().stem().string().substr(pattern.size()));
            if (requestedFilenum < 0 || requestedFilenum == filenum) {
                cout << "\tFile : " << i->filename() << endl;
                processNextFile(deviceId, filenum, karaboHistory, sfs, buildContentFile, idxprops);
            }
        }

        sfs.close();
    }

    return 0;
}


bool byLastFileModificationTime(const bf::path& lhs, const bf::path& rhs) {
    return bf::last_write_time(lhs) < bf::last_write_time(rhs);
}


void processNextFile(const std::string& deviceId, size_t number, const std::string& historyDir, ifstream& sfs,
                     bool buildContent, const vector<string>& idxprops) {
    TextSerializer<Schema>::Pointer serializer = TextSerializer<Schema>::create(Hash("Xml"));
    Schema::Pointer schema(new Schema);
    serializer->load(*schema, schemaRange.fromSchemaArchive);
    boost::regex lineRegex(karabo::util::DATALOG_LINE_REGEX, boost::regex::extended);

    string infile = historyDir + "/" + deviceId + "/raw/archive_" + toString(number) + ".txt";
    ifstream irs(infile.c_str());
    bool newFileFlag = true;
    unsigned int expNum = 0x0F0A1A2A;
    unsigned int runNum = 0x0F0B1B2B;

    size_t recnum = 0;

    while (irs.good()) {
        string line;
        std::istream::pos_type position = irs.tellg();

        if (getline(irs, line)) {
            if (line.empty() || position == -1) {
                // Skips the writing of the index entry if the log
                // entry to be indexed was empty or its position in
                // the log file could not be obtained.
                if (position == -1) {
                    cout << "Skip processing of record " << recnum + 1 << " of file '" << infile << "':\n"
                         << "\tProcessing that record would result on an entry with position -1 in the "
                            "archive_index.txt file"
                         << std::endl;
                }
                continue;
            }
            boost::smatch tokens;
            bool search_res = boost::regex_search(line, tokens, lineRegex);
            if (!search_res) {
                cout << "*** idxBuild: skip corrupted record : line = " << line << ", token.size() = " << tokens.size()
                     << endl;
                continue; // This record is corrupted -- skip it
            }
            recnum++;

            // With offset = 2 for raw files from 1.4.X and offset = 0 for later releases, we have:
            const string& epochISO8601 = tokens[1];
            const string& epochDoubleStr = tokens[2];
            const string& trainIdStr = tokens[3];
            const string& property = tokens[4];
            // tokens[5] => property type
            // tokens[6] => property value
            const string& user = tokens[7];
            const string& flag = tokens[8];

            const Epochstamp epstamp(karabo::util::stringDoubleToEpochstamp(epochDoubleStr));

            // cout << "*** " << recnum << " *** "
            //         "\t" << "position in input : " << position << ", epoch: " << seconds << "." << fraction << endl;

            while (epstamp.getSeconds() > schemaRange.toSeconds ||
                   (epstamp.getSeconds() == schemaRange.toSeconds &&
                    epstamp.getFractionalSeconds() > schemaRange.toFraction)) {
                if (sfs.fail()) break;
                schemaRange.fromSeconds = schemaRange.toSeconds;
                schemaRange.fromFraction = schemaRange.toFraction;
                schemaRange.fromTrainId = schemaRange.toTrainId;
                schemaRange.fromSchemaArchive = schemaRange.toSchemaArchive;
                sfs >> schemaRange.toSeconds >> schemaRange.toFraction >> schemaRange.toTrainId;
                sfs.seekg(1, ios::cur);
                getline(sfs, schemaRange.toSchemaArchive, '\n');
                schema = Schema::Pointer(new Schema);
                serializer->load(*schema, schemaRange.fromSchemaArchive);
            }
            // cout << "*** Current schema for range from " << schemaRange.fromSeconds << "." <<
            // schemaRange.fromFraction
            //         << " to " << schemaRange.toSeconds << "." << schemaRange.toFraction << endl;

            if ((flag == "LOGIN" || flag == "LOGOUT" || newFileFlag)) {
                newFileFlag = false;
                if (buildContent) {
                    string contentFile = historyDir + "/" + deviceId + "/raw/archive_index.txt";
                    ofstream ocs(contentFile.c_str(), ios::out | ios::app);
                    if (flag == "LOGIN") ocs << "+LOG ";
                    else if (flag == "LOGOUT") ocs << "-LOG ";
                    else ocs << "=NEW ";

                    ocs << epochISO8601 << " " << epochDoubleStr << " "
                        << " " << trainIdStr << " " << position << " " << (user.empty() ? "." : user) << " " << number
                        << "\n";

                    ocs.close();
                }

                // cout << epochISO8601 << " " << epochDouble << " " << trainIdStr
                //         << " " << position << " " << user << " " << number << endl;

                if (flag == "LOGOUT") {
                    map<string, MetaData::Pointer>::iterator ii = idxMap.begin();
                    while (ii != idxMap.end()) {
                        MetaData::Pointer mdp = ii->second;
                        if (mdp && mdp->idxStream.is_open()) {
                            mdp->marker = true; // mark the record by "LOGOUT" event
                            ++ii;
                        } else {
                            idxMap.erase(ii++); // forget about "not opened" entries to mimic DataLogger behavior
                        }
                    }
                } else {
                    // set the marker up
                    for (map<string, MetaData::Pointer>::iterator it = idxMap.begin(); it != idxMap.end(); it++) {
                        MetaData::Pointer mdp = it->second;
                        mdp->marker = true;
                    }
                }
            }

            if (property == ".") continue;

            // check if we have any property registered
            if (idxprops.empty()) continue; // no interest for building binary index
            if (find(idxprops.begin(), idxprops.end(), property) == idxprops.end())
                continue; // property is not in the prop file

            // Check if we need to build index for this property by inspecting schema ... checking only existence
            if (schema->has(property)) {
                MetaData::Pointer& mdp = idxMap[property]; // Pointer by reference!
                if (!mdp) {
                    // a property not yet indexed - create meta data and set file
                    mdp = MetaData::Pointer(new MetaData);
                    mdp->idxFile = historyDir + "/" + deviceId + "/idx/archive_" + toString(number) + "-" + property +
                                   "-index.bin";
                }
                if (!mdp->idxStream.is_open()) {
                    mdp->idxStream.open(mdp->idxFile.c_str(), ios::out | ios::trunc | ios::binary);
                }
                mdp->record.epochstamp = fromString<double>(epochDoubleStr);
                mdp->record.trainId = fromString<unsigned long long>(trainIdStr);
                mdp->record.positionInRaw = position;
                mdp->record.extent1 = (expNum & 0xFFFFFF);
                mdp->record.extent2 = (runNum & 0xFFFFFF);
                if (mdp->marker) {
                    mdp->marker = false;
                    mdp->record.extent2 |= (1 << 30);
                }
                mdp->idxStream.write((char*)&mdp->record, sizeof(MetaData::Record));
            }
        }
    }
    if (irs.is_open()) irs.close();
    for (map<string, MetaData::Pointer>::iterator ii = idxMap.begin(); ii != idxMap.end(); ++ii) {
        MetaData::Pointer mdp = ii->second;
        if (mdp && mdp->idxStream.is_open()) mdp->idxStream.close();
    }
    idxMap.clear();
}
