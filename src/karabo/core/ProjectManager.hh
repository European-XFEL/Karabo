/* 
 * File:   ProjectManager.hh
 * Author: heisenb
 *
 * Created on January 9, 2015, 1:17 PM
 */

#ifndef KARABO_CORE_PROJECTMANAGER_HH
#define	KARABO_CORE_PROJECTMANAGER_HH

#include "Device.hh"

namespace karabo {


    namespace core {

        class ProjectManager : public karabo::core::Device<> {
            
        public:
            
            KARABO_CLASSINFO(ProjectManager, "ProjectManager", "1.0")
                    
            static void expectedParameters(karabo::util::Schema& expected);
                    
            ProjectManager(const karabo::util::Hash& input);
           
            virtual ~ProjectManager();
            
        private:
            
            void initialize();
            
            void slotGetAvailableProjects();
            
            void slotLoadProject(const std::string& userName, const std::string& projectName);
            
            void slotSaveProject(const std::string& userName, const std::string& projectName);
            
            void slotCloseProject(const std::string& userName, const std::string& projectName);            
            

        };
    }
}
#endif	/* PROJECTMANAGER_HH */

