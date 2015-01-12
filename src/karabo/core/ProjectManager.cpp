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
                    .assignmentMandatory()
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

            SLOT1(slotLoadProject, Hash)
            SLOT1(slotSaveProject, Hash)
            SLOT1(slotCloseProject, Hash)

        }


        ProjectManager::~ProjectManager() {
        }


        void ProjectManager::initialize() {

            if (!boost::filesystem::exists(get<string>("directory"))) {
                boost::filesystem::create_directory(get<string>("directory"));
            }

        }


        void ProjectManager::slotLoadProject(const karabo::util::Hash& hash) {
            KARABO_LOG_DEBUG << "slotLoadProject";
            
            string projectName = hash.get<string>("projectName");
                        
            Hash answer(hash);
            std::vector<char>& buffer = answer.bindReference<std::vector<char> >("buffer");            
            karabo::io::loadFromFile(buffer, get<string>("directory") + "/" + projectName);
            reply(answer);
            
        }


        void ProjectManager::slotSaveProject(const karabo::util::Hash& hash) {
            KARABO_LOG_DEBUG << "slotSaveProject";
            
            
        }


        void ProjectManager::slotCloseProject(const karabo::util::Hash& hash) {
            KARABO_LOG_DEBUG << "slotCloseProject";
        }


    }
}
