/* 
 * File:   idxBuild.cc
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on July 28, 2015, 2:35 PM
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp> 
#include <karabo/util/Epochstamp.hh>
#include <karabo/util/Exception.hh>
#include <karabo/core/DataLoggerStructs.hh>
#include <karabo/io/TextSerializer.hh>
#include <karabo/util/Schema.hh>

namespace bf = boost::filesystem;
namespace bs = boost::system;
using namespace std;
using namespace karabo::core;
using namespace karabo::util;
using namespace karabo::io;

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


/*
 * 
 */
int main(int argc, char** argv) {

    if (argc < 2) {
        cout << "\nUsage: " << argv[0] << " <karabo_history_dir>\n" << endl;
        return 1;
    }

    string karaboHistory(argv[1]);

    cout << "\nInput parameters: karaboHistory=\"" << karaboHistory << "\"\n" << endl;

    bf::path history(karaboHistory);
    if (!bf::exists(history))
        throw KARABO_PARAMETER_EXCEPTION("Directory \"" + history.string() + "\" does not exist!");
    if (!bf::is_directory(history))
        throw KARABO_PARAMETER_EXCEPTION("File \"" + history.string() + "\" is not a directory!");

    // convert to the new directory structure if needed
    {
        // check if we have old raw directory
        bf::path rawdir(karaboHistory + "/raw/");
        if (bf::exists(rawdir) && bf::is_directory(rawdir)) {
            // build the list of devices
            vector<string> devices;
            for (bf::directory_iterator it(rawdir); it != bf::directory_iterator(); ++it) {
                if (it->path().extension() != ".last") continue;
                string deviceId = it->path().stem().string();
                devices.push_back(deviceId);
            }
            // create new dir structure and copy files ...
            for (vector<string>::iterator it = devices.begin(); it != devices.end(); ++it) {
                string deviceId = *it;
                bf::path deviceDir(karaboHistory + "/" + deviceId);
                bf::path deviceRawDir(karaboHistory + "/" + deviceId + "/raw/");
                if (!bf::exists(deviceDir)) {
                    bf::create_directory(deviceDir);
                    bf::create_directory(deviceRawDir);
                }
                for (bf::directory_iterator ii(rawdir); ii != bf::directory_iterator(); ++ii) {
                    if (boost::starts_with(ii->path().filename().string(), deviceId)) {
                        bf::copy_file(ii->path(), bf::path(deviceRawDir.string() + ii->path().filename().string()));
                    }
                }
            }
            // remove old raw directory
            bf::remove_all(rawdir);
        }
        // remove old idx directory
        bf::path idxdir(karaboHistory + "/idx/");
        if (bf::exists(idxdir))
            bf::remove_all(idxdir);
    }
    
    
    vector<string> devices;
    for (bf::directory_iterator it(history); it != bf::directory_iterator(); ++it) {
        if (!bf::is_directory(it->path())) continue;
        string deviceId = it->path().filename().string();
        if (deviceId == "raw" || deviceId == "idx") continue;
        devices.push_back(deviceId);
        
        bf::path rawdir(karaboHistory + "/" + deviceId + "/raw");
        if (!bf::exists(rawdir))
            throw KARABO_PARAMETER_EXCEPTION("Directory \"" + rawdir.string() + "\" does not exist!");
        if (!bf::is_directory(rawdir))
            throw KARABO_PARAMETER_EXCEPTION("File \"" + rawdir.string() + "\" is not a directory!");
        
        bf::path idxdir(karaboHistory + "/" + deviceId + "/idx");
        bf::remove_all(idxdir);
        bf::create_directory(idxdir);
    
    }
    
    cout << devices.size() << " devices have to be processed..." << endl;

    for (vector<string>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
        string deviceId = *it;

        cout << "Process the device : \"" << deviceId << "\"" << endl;
        
        bf::path schemaPath(karaboHistory + "/" + deviceId + "/raw/" + deviceId + "_schema.txt");
        if (!bf::exists(schemaPath)) {
            cout << "WARNING: No schema file found for the device: \"" << deviceId << "\". Skip this device..." << endl;
            continue;
        }

        bf::path rawdir(karaboHistory + "/" + deviceId + "/raw/");
        
        vector<bf::path> rawtxt;
        for (bf::directory_iterator dit(rawdir); dit != bf::directory_iterator(); ++dit) {
            if (dit->path().extension() != ".txt") continue;
            string filename = dit->path().filename().string();
            string pattern = deviceId + "_configuration_";
            if (filename.substr(0, pattern.length()) != pattern) continue;
            //cout << "Filename : " << dit->path().filename() << endl;
            rawtxt.push_back(dit->path());
        }

        // Sort this list by time of last file modification
        sort(rawtxt.begin(), rawtxt.end(), byLastFileModificationTime);

        // rename "content" file
        bf::path cfile(history.string() + "/" + deviceId + "/raw/" + deviceId + "_index.txt");
        bool buildContentFile = false;
        if (!bf::exists(cfile)) buildContentFile = true;

        // Load properties file into vector
        vector<string> idxprops;
        {
            bf::path propPath(history.string() + "/" + deviceId + "/raw/" + deviceId + "_properties_with_index.txt");
            if (bf::exists(propPath)) {
                ifstream in(propPath.c_str());
                in.seekg(0, ios::end);
                string content(in.tellg(), ' ');
                in.seekg(0, ios::beg);
                content.assign((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
                in.close();
                boost::split(idxprops, content, boost::is_any_of("\n"));
            }
        }
        
        // Check completeness of the data set
        for (size_t i = 0; i < rawtxt.size(); ++i) {
            string fn = deviceId + "_configuration_" + toString(i) + ".txt";
            if (rawtxt[i].filename().string() != fn) {
                throw KARABO_PARAMETER_EXCEPTION("Data set is not complete: found file: \"" + rawtxt[i].filename().string()
                                                 + "\", expected: \"" + fn + "\"");
            }
        }

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

        for (size_t i = 0; i < rawtxt.size(); ++i) {
            cout << "\tFile : " << rawtxt[i].filename() << endl;
            processNextFile(deviceId, i, karaboHistory, sfs, buildContentFile, idxprops);
        }

        sfs.close();
    }

    return 0;
}


bool byLastFileModificationTime(const bf::path& lhs, const bf::path& rhs) {
    return bf::last_write_time(lhs) < bf::last_write_time(rhs);
}


void processNextFile(const std::string& deviceId, size_t number, const std::string& historyDir,
                     ifstream& sfs, bool buildContent, const vector<string>& idxprops) {

    TextSerializer<Schema>::Pointer serializer = TextSerializer<Schema>::create(Hash("Xml"));
    Schema::Pointer schema(new Schema);
    serializer->load(*schema, schemaRange.fromSchemaArchive);

    string infile = historyDir + "/" + deviceId + "/raw/" + deviceId + "_configuration_" + toString(number) + ".txt";
    ifstream irs(infile.c_str());
    bool newFileFlag = true;
    unsigned int expNum = 0x0F0A1A2A;
    unsigned int runNum = 0x0F0B1B2B;

    size_t recnum = 0;

    while (irs.good()) {
        string line;
        size_t position = irs.tellg();
        if (getline(irs, line)) {
            if (line.empty()) continue;

            vector<string> tokens;
            boost::split(tokens, line, boost::is_any_of("|"));
            if (tokens.size() != 10) {
                cout << "*** slotGetPropertyHistory: skip corrupted record: token.size() = " << tokens.size() << endl;
                continue; // This record is corrupted -- skip it
            }
            recnum++;

            // tokens[0] => epochISO8601
            // tokens[1] => epochDouble
            // tokens[2] => epochSeconds
            // tokens[3] => epochFraction
            // tokens[4] => sTrainId
            // tokens[5] => property name
            // tokens[6] => property type
            // tokens[7] => property value
            // tokens[8] => user
            // tokens[9] => flag

            unsigned long long seconds = fromString<unsigned long long>(tokens[2]);
            unsigned long long fraction = fromString<unsigned long long>(tokens[3]);

            //cout << "*** " << recnum << " *** "
            //        "\t" << "position in input : " << position << ", epoch: " << seconds << "." << fraction << endl;

            while (seconds > schemaRange.toSeconds || (seconds == schemaRange.toSeconds && fraction > schemaRange.toFraction)) {
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
            //cout << "*** Current schema for range from " << schemaRange.fromSeconds << "." << schemaRange.fromFraction
            //        << " to " << schemaRange.toSeconds << "." << schemaRange.toFraction << endl;

            if ((tokens[9] == "LOGIN" || tokens[9] == "LOGOUT" || newFileFlag)) {
                newFileFlag = false;
                if (buildContent) {
                    string contentFile = historyDir + "/" + deviceId + "/raw/" + deviceId + "_index.txt";
                    ofstream ocs(contentFile.c_str(), ios::out | ios::app);
                    if (tokens[9] == "LOGIN")
                        ocs << "+LOG ";
                    else if (tokens[9] == "LOGOUT")
                        ocs << "-LOG ";
                    else
                        ocs << "=NEW ";

                    ocs << tokens[0] << " " << tokens[1] << " " << tokens[2] << " " << tokens[3] << " " << tokens[4]
                        << " " << position << " " << tokens[8] << " " << number << "\n";

                    ocs.close();
                }
                
                //cout << tokens[0] << " " << tokens[1] << " " << tokens[2] << " " << tokens[3] << " " << tokens[4]
                //        << " " << position << " " << tokens[8] << " " << number << endl;

                if (tokens[9] == "LOGOUT") {
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

            // token(5) is a property name
            if (tokens[5] == ".") continue;

            // check if we have any property registered
            if (idxprops.empty()) continue;    // no interest for building binary index
            if (find(idxprops.begin(), idxprops.end(), tokens[5]) == idxprops.end()) continue;  // property is not in the prop file
            
            // Check if we need to build index for this property by inspecting schema and filter it
            if (schema->has(tokens[5]) && schema->isAccessReadOnly(tokens[5])) {
                map<string, MetaData::Pointer>::iterator it = idxMap.find(tokens[5]);
                MetaData::Pointer mdp;
                if (it == idxMap.end()) {
                    mdp = MetaData::Pointer(new MetaData);
                    mdp->idxFile = historyDir + "/" + deviceId + "/idx/" + deviceId + "_configuration_" + toString(number) + "-" + tokens[5] + "-index.bin";
                    mdp->record.epochstamp = fromString<double>(tokens[1]);
                    mdp->record.trainId = fromString<unsigned long long>(tokens[4]);
                    mdp->record.positionInRaw = position;
                    mdp->record.extent1 = (expNum & 0xFFFFFF);
                    mdp->record.extent2 = (runNum & 0xFFFFFF);
                    idxMap[tokens[5]] = mdp;
                    // defer writing: write only if more changes come
                } else {
                    mdp = it->second;
                    if (!mdp->idxStream.is_open()) {
                        mdp->idxStream.open(mdp->idxFile.c_str(), ios::out | ios::trunc | ios::binary);
                        if (mdp->marker) {
                            mdp->marker = false;
                            mdp->record.extent2 |= (1 << 30);
                        }
                        // write (flush) deferred record
                        mdp->idxStream.write((char*) &mdp->record, sizeof (MetaData::Record));
                    }
                    mdp->record.epochstamp = fromString<double>(tokens[1]);
                    mdp->record.trainId = fromString<unsigned long long>(tokens[4]);
                    mdp->record.positionInRaw = position;
                    mdp->record.extent1 = (expNum & 0xFFFFFF);
                    mdp->record.extent2 = (runNum & 0xFFFFFF);
                    if (mdp->marker) {
                        mdp->marker = false;
                        mdp->record.extent2 |= (1 << 30);
                    }
                    mdp->idxStream.write((char*) &mdp->record, sizeof (MetaData::Record));
                }
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