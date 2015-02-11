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

        struct ProjectMetaData {
            std::string m_version;
            std::string m_author;
            karabo::util::Epochstamp m_creationEpoch;
            karabo::util::Epochstamp m_lastModifiedEpoch;
            bool m_checkedOut;
            std::string m_checkedOutBy;
        };

        class ProjectManager : public karabo::core::Device<> {
            
        public:
            
            KARABO_CLASSINFO(ProjectManager, "ProjectManager", "1.0")
                    
            static void expectedParameters(karabo::util::Schema& expected);
                    
            ProjectManager(const karabo::util::Hash& input);
           
            virtual ~ProjectManager();
            
        private:
            
            void initialize();
            
            void slotGetAvailableProjects();
            
            void slotNewProject(const karabo::util::Hash& info);
            
            void slotLoadProject(const std::string& userName, const std::string& projectName);
            
            void slotSaveProject(const std::string& userName, const std::string& projectName, const std::vector<char>& data);
            
            void slotCloseProject(const std::string& userName, const std::string& projectName);            
            

        };
    }
}
#endif	/* PROJECTMANAGER_HH */

