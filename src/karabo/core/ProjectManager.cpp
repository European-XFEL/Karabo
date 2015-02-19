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

        /**
         The project file with the given \projectName is updated.

        \param[in] projectName Name of the project.
        \param[in] metaData    Hash including the new meta data of the project.
        \param[in,out] newData Vector of char is filled with concated data.
         
        \return \true, if updating successful, otherwise \false.
        */
        bool ProjectManager::updateProjectFile(const std::string& projectName,
                                               karabo::util::Hash& metaData,
                                               std::vector<char>& newData) {
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
                
                return saveProject(projectName, metaData, data, newData);
            }
            return false;
        }


        /**
        A project including meta and binary data is written to the directory.

        \param[in] projectName Name of the project.
        \param[in] metaData    Hash including the meta data of the project.
        \param[in] data        Vector of char including the binary data of the project.
        \param[in,out] newData Vector of char is filled with concated data.
        
        \return \true, if saving successful, otherwise \false.
        */
        bool ProjectManager::saveProject(const std::string& projectName,
                                         const karabo::util::Hash& metaData,
                                         const std::vector<char>& data,
                                         std::vector<char>& newData) {
            karabo::io::TextSerializer<karabo::util::Hash>::Pointer ts = karabo::io::TextSerializer<Hash>::create("Xml");
            string hashXml;
            ts->save(metaData, hashXml);
            
            // Write data into project file
            string filename = get<string>("directory") + "/" + projectName;
            std::fstream file(filename.c_str(), ios::out | ios::binary);
            std::ostream& result1 = file.write(hashXml.c_str(), hashXml.size());
            // Add end-of-file character to differentiate between header and binary content
            file << char(26);
            file << "\n";
            std::ostream& result2 = file.write(const_cast<const char*> (&data[0]), data.size());
            file.close();
            
            // Read data into newData
            file.open(filename.c_str(), ios::in | ios::binary);
            std::stringstream os;
            os << file.rdbuf();
            os.seekg(0);
            newData.resize(os.rdbuf()->in_avail());
            os.read(&newData[0], newData.size());
            file.close();
            
            return result1.good() && result2.good();
        }

        /**
         This registered slot parses the project directory and answers back to
         the registered callback function (availableProjects) in the
         GuiServerDevice sending a Hash with all project (meta) data.
        */
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
        

        /**
         This registered slot creates the meta data for the new project and
         stores the project including meta data and binary data in one file to
         the project directory.
         The new binary data of the project is send back to the registered
         callback function (projectNew) GuiServerDevice.
         
         \param[in] author      Author of the project.
         \param[in] projectName Name of the project.
         \param[in] data        Binary data of the project.
        */
        void ProjectManager::slotNewProject(const std::string& author,
                                            const std::string& projectName,
                                            const std::vector<char>& data) {
            KARABO_LOG_DEBUG << "slotNewProject " << projectName;
            
            Hash metaData;
            metaData.set("version", "1.3.0");
            metaData.set("author", author);
            
            karabo::util::Epochstamp epoch;
            double timestamp = epoch.toTimestamp();
            metaData.set("creationDate", timestamp);
            metaData.set("lastModified", timestamp);
            // checkedOut needs to be false to be send back to the author
            metaData.set("checkedOut", false);
            metaData.set("checkedOutBy", "");
            
            // Store and send back to author
            vector<char> newData;
            bool success = saveProject(projectName, metaData, data, newData);
            reply(projectName, success, newData);
            
            // Set true to finally store the project for others
            metaData.set("checkedOut", true);
            metaData.set("checkedOutBy", author);
            m_projectMetaData[projectName] = metaData;
            
            saveProject(projectName, metaData, data, newData);
        }


        /**
         This registered slot loads the requested project from the project directory
         and sends it back to the registered callback function (projectLoaded)
         in the GuiServerDevice.
         
         \param[in] userName    User who loads the project.
         \param[in] projectName Name of the project.
        */
        void ProjectManager::slotLoadProject(const std::string& userName,
                                             const std::string& projectName) {
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
                vector<char> newData;
                updateProjectFile(projectName, metaData, newData);
            }
        }

        /**
         This registered slot saves the requested project to the project directory
         and sends back to the registered callback function (projectSaved)
         in the GuiServerDevice.
         
         \param[in] userName    User who loads the project.
         \param[in] projectName Name of the project.
         \param[in] data        Binary data of the project.
        */
        void ProjectManager::slotSaveProject(const std::string& userName,
                                             const std::string& projectName,
                                             const std::vector<char>& data) {
            KARABO_LOG_DEBUG << "slotSaveProject " << userName << " " << projectName;
            
            // Update meta data and save project
            Hash &metaData = m_projectMetaData[projectName];
            
            if (userName == metaData.get<string >("checkedOutBy") && metaData.get<bool >("checkedOut")) {
                metaData.set("checkedOut", false);
            }
            
            karabo::util::Epochstamp epoch;
            double timestamp = epoch.toTimestamp();
            metaData.set("lastModified", timestamp);
            
            vector<char> newData;
            bool success = saveProject(projectName, metaData, data, newData);
            reply(projectName, success, newData);
            
            metaData.set("checkedOut", true);
            // Update project file with new meta data
            saveProject(projectName, metaData, data, newData);
        }


        /**
         This registered slot changes the meta data of the project to the project
         directory and sends the changes back to the registered callback function
         (projectClosed) in the GuiServerDevice.
         
         \param[in] userName    User who loads the project.
         \param[in] projectName Name of the project.
        */
        void ProjectManager::slotCloseProject(const std::string& userName,
                                              const std::string& projectName) {
            KARABO_LOG_DEBUG << "slotCloseProject " << userName << " " << projectName;
            
            Hash &metaData = m_projectMetaData[projectName];
            KARABO_LOG_DEBUG << metaData;
            
            if (userName != metaData.get<string >("checkedOutBy")) return;
            
            metaData.set("checkedOut", false);
            metaData.set("checkedOutBy", "");

            vector<char> newData;
            // Update project file with new meta data
            bool success = updateProjectFile(projectName, metaData, newData);
            reply(projectName, success, newData);
        }

    }
}
