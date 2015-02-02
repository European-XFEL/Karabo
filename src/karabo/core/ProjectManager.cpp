/* 
 * File:   ProjectManager.cpp
 * Author: heisenb
 * 
 * Created on January 9, 2015, 1:17 PM
 */

#include <karabo/karabo.hpp>
#include "ProjectManager.hh"

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

            SLOT0(slotGetAvailableProjects)
            SLOT2(slotLoadProject, string /*username*/, string /*projectName*/)
            SLOT3(slotSaveProject, string /*username*/, string /*projectName*/, vector<char> /*data*/)
            SLOT2(slotCloseProject, string /*username*/, string /*projectName*/)
        }


        ProjectManager::~ProjectManager() {
        }


        void ProjectManager::initialize() {

            if (!boost::filesystem::exists(get<string>("directory"))) {
                boost::filesystem::create_directory(get<string>("directory"));
            }

        }

        
        void ProjectManager::slotGetAvailableProjects() {
            KARABO_LOG_DEBUG << "slotGetAvailableProjects";
            
            std::vector<std::string> projects;
            // Check project directory for all projects
            boost::filesystem::path directory(get<string> ("directory"));
            boost::filesystem::directory_iterator end_iter;
            for (boost::filesystem::directory_iterator iter(directory); iter != end_iter; ++iter) {
                projects.push_back(iter->path().filename().string());
            }
            
            reply(projects);
        }

        
        void ProjectManager::slotLoadProject(const std::string& userName, const std::string& projectName) {
            KARABO_LOG_DEBUG << "slotLoadProject";

            //Hash answer("name", projectName);
            //std::vector<char>& buffer = answer.bindReference<std::vector<char> >("buffer");
            
            std::vector<char> data;
            karabo::io::loadFromFile(data, get<string>("directory") + "/" + projectName);
            
            reply(projectName, data);
        }


        void ProjectManager::slotSaveProject(const std::string& userName, const std::string& projectName, const vector<char>& data) {
            KARABO_LOG_DEBUG << "slotSaveProject " << userName << " " << projectName;
            
            bool success = karabo::io::saveToFile(data, get<string>("directory") + "/" + projectName);
            KARABO_LOG_DEBUG << "success " << success;
            
            reply(projectName, success);
        }


        void ProjectManager::slotCloseProject(const std::string& userName, const std::string& projectName) {
            KARABO_LOG_DEBUG << "slotCloseProject";
            
            bool success = True;
            reply(projectName, success);
        }

    }
}
