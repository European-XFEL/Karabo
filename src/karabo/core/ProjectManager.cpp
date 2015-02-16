/* 
 * File:   ProjectManager.cpp
 * Author: heisenb
 * 
 * Created on January 9, 2015, 1:17 PM
 */

#include <karabo/karabo.hpp>
#include "ProjectManager.hh"
#include <stdio.h>

namespace karabo {
    namespace core {

        KARABO_NAMESPACES;


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, ProjectManager)


        void ProjectManager::expectedParameters(Schema& expected) {

            PATH_ELEMENT(expected).key("directory")
                    .displayedName("Directory")
                    .description("The directory where the project files should be placed")
                    .assignmentOptional().defaultValue("projects")
                    .commit();
            
            OVERWRITE_ELEMENT(expected).key("deviceId")
                    .setNewDefaultValue("Karabo_ProjectManager")
                    .commit();

            // Do not archive the archivers (would lead to infinite recursion)
            OVERWRITE_ELEMENT(expected).key("archive")
                    .setNewDefaultValue(false)
                    .commit();

            // Hide the loggers from the standard view in clients
            OVERWRITE_ELEMENT(expected).key("visibility")
                    .setNewDefaultValue(5)
                    .commit();

            // Slow beats
            OVERWRITE_ELEMENT(expected).key("heartbeatInterval")
                    .setNewDefaultValue(60)
                    .commit();
        }


        ProjectManager::ProjectManager(const karabo::util::Hash& input) : Device<>(input) {

            registerInitialFunction(boost::bind(&karabo::core::ProjectManager::initialize, this));

            KARABO_SLOT(slotGetAvailableProjects)
            KARABO_SLOT(slotNewProject, string /*author*/, string /*projectname*/, vector<char> /*data*/)
            KARABO_SLOT(slotLoadProject, string /*username*/, string /*projectName*/)
            KARABO_SLOT(slotSaveProject, string /*username*/, string /*projectName*/, vector<char> /*data*/)
            KARABO_SLOT(slotCloseProject, string /*username*/, string /*projectName*/)
        }


        ProjectManager::~ProjectManager() {
        }


        void ProjectManager::initialize() {

            if (!boost::filesystem::exists(get<string>("directory"))) {
                boost::filesystem::create_directory(get<string>("directory"));
            }

        }


        bool ProjectManager::updateProjectFile(const std::string& projectName, karabo::util::Hash& metaData) {
            KARABO_LOG_DEBUG << "updateProjectFile " << projectName;
            
            string filename = get<string>("directory") + "/" + projectName;
            ifstream projectFile(filename.c_str(), ios::in | ios::binary);
            if (projectFile.is_open()) {
                KARABO_LOG_DEBUG << "Opened project file " << projectName;

                // Get full file length
                projectFile.seekg(0, projectFile.end);
                int fileLength = projectFile.tellg();
                projectFile.seekg(0, projectFile.beg);
                
                // Get meta data length
                std::string line;
                int metaDataLength = 0;
                while (std::getline(projectFile, line)) {
                    if (*line.c_str() == char(26)) {
                        metaDataLength = projectFile.tellg();
                        break;
                    }
                }
                
                projectFile.seekg(metaDataLength, projectFile.beg);
                int dataLength = fileLength - metaDataLength;
                
                // Read only data bytes
                vector<char> data;
                data.resize(dataLength);
                projectFile.read(&data[0], dataLength);
                
                projectFile.close();
                
                return saveProject(projectName, metaData, data);
            }
            return false;
        }
        

        bool ProjectManager::saveProject(const std::string& projectName, const karabo::util::Hash& metaData, const std::vector<char>& data) {
            karabo::io::TextSerializer<karabo::util::Hash>::Pointer ts = karabo::io::TextSerializer<Hash>::create("Xml");
            string hashXml;
            ts->save(metaData, hashXml);
            
            string filename = get<string>("directory") + "/" + projectName;
            std::ofstream file(filename.c_str(), std::ios::binary);
            std::ostream& result1 = file.write(hashXml.c_str(), hashXml.size());
            // Add end-of-file character to differentiate between header and binary content
            file << char(26);
            file << "\n";
            std::ostream& result2 = file.write(const_cast<const char*> (&data[0]), data.size());
            file.close();
            
            return result1.good() && result2.good();
        }

        
        void ProjectManager::slotGetAvailableProjects() {
            KARABO_LOG_DEBUG << "slotGetAvailableProjects";
            
            // Hash to store all project names and meta data as attributes
            karabo::util::Hash projects;
            TextSerializer<Hash>::Pointer ts = TextSerializer<Hash>::create("Xml");
            
            // Check project directory for all projects
            boost::filesystem::path directory(get<string> ("directory"));
            boost::filesystem::directory_iterator end_iter;
            for (boost::filesystem::directory_iterator iter(directory); iter != end_iter; ++iter) {
                std::string relativePath = iter->path().relative_path().string();
                // Get meta-data from project file
                ifstream projectFile(relativePath.c_str(), ios::in | ios::binary);
                if (projectFile.is_open()) {
                    std::string path = iter->path().stem().string();
                    projects.set(path, Hash());
                    
                    KARABO_LOG_DEBUG << "Opened project file " << path;
                    
                    std::string headerString;
                    std::string line;
                    while (std::getline(projectFile, line)) {
                        if (*line.c_str() == char(26)) break;
                        headerString.append(line);
                    }
                    
                    KARABO_LOG_DEBUG << "Project meta data\n";
                    KARABO_LOG_DEBUG << headerString;
                    
                    Hash p;
                    ts->load(p, headerString);
                    projects.set(path, p);
                    
                    std::string projectName = iter->path().filename().string();
                    // Save meta data to datastructure
                    m_projectMetaData[projectName] = p;
                    
                    projectFile.close();
                } else {
                    KARABO_LOG_DEBUG << "Not able to open project file " << relativePath;
                }
            }
            
            reply(projects);
        }
        
        void ProjectManager::slotNewProject(const std::string& author, const std::string& projectName, const std::vector<char>& data) {
            KARABO_LOG_DEBUG << "slotNewProject " << projectName;
            
            Hash metaData;
            metaData.set("version", "1.3.0");
            metaData.set("author", author);
            
            karabo::util::Epochstamp epoch;
            double timestamp = epoch.toTimestamp();
            metaData.set("creationDate", timestamp);
            metaData.set("lastModified", timestamp);
            metaData.set("checkedOut", true);
            metaData.set("checkedOutBy", author);
            
            //string key = projectName.substr(0, projectName.length()-4);
            m_projectMetaData[projectName] = metaData;
            
            reply(projectName, saveProject(projectName, metaData, data));
        }
        
        
        void ProjectManager::slotLoadProject(const std::string& userName, const std::string& projectName) {
            KARABO_LOG_DEBUG << "slotLoadProject " << projectName;
            
            std::vector<char> data;
            karabo::io::loadFromFile(data, get<string>("directory") + "/" + projectName);
            
            Hash &metaData = m_projectMetaData[projectName];
            reply(projectName, metaData, data);
            
            // Update meta data, if this is the first time this project is loaded
            if (!metaData.get<bool >("checkedOut")) {
                metaData.set("checkedOut", true);
                metaData.set("checkedOutBy", userName);
                
                // Update project file with new meta data
                updateProjectFile(projectName, metaData);
            }
        }


        void ProjectManager::slotSaveProject(const std::string& userName, const std::string& projectName, const std::vector<char>& data) {
            KARABO_LOG_DEBUG << "slotSaveProject " << userName << " " << projectName;
            
            // Update meta data and save project
            Hash &metaData = m_projectMetaData[projectName];
            
            karabo::util::Epochstamp epoch;
            double timestamp = epoch.toTimestamp();
            metaData.set("lastModified", timestamp);
            
            reply(projectName, saveProject(projectName, metaData, data));
        }


        void ProjectManager::slotCloseProject(const std::string& userName, const std::string& projectName) {
            KARABO_LOG_DEBUG << "slotCloseProject " << userName << " " << projectName;
            
            Hash &metaData = m_projectMetaData[projectName];
            KARABO_LOG_DEBUG << metaData;
            
            bool success = True;
            if (userName == metaData.get<string >("checkedOutBy")) {
                metaData.set("checkedOut", false);
                metaData.set("checkedOutBy", "");

                // Update project file with new meta data
                success = updateProjectFile(projectName, metaData);
            }
                        
            reply(projectName, success);
        }

    }
}
