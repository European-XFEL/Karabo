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
            
            // {key: projectName.krb, value: metaData}
            std::map<std::string, karabo::util::Hash > m_projectMetaData;
            
        public:
            
            KARABO_CLASSINFO(ProjectManager, "ProjectManager", "1.0")
                    
            static void expectedParameters(karabo::util::Schema& expected);
                    
            ProjectManager(const karabo::util::Hash& input);
           
            virtual ~ProjectManager();
            
        private:
            
            void initialize();
            
            bool updateProjectFile(const std::string& projectName,
                                   karabo::util::Hash& metaData,
                                   std::vector<char>& newData);
            
            bool saveProject(const std::string& projectName,
                             const karabo::util::Hash& metaData,
                             const std::vector<char>& data,
                             std::vector<char>& newData);
            
            void slotGetAvailableProjects();
            
            void slotNewProject(const std::string& author,
                                const std::string& projectName,
                                const std::vector<char>& data);
            
            void slotLoadProject(const std::string& userName,
                                 const std::string& projectName);
            
            void slotSaveProject(const std::string& userName,
                                 const std::string& projectName,
                                 const std::vector<char>& data);
            
            void slotCloseProject(const std::string& userName,
                                  const std::string& projectName);

        };
    }
}
#endif	/* PROJECTMANAGER_HH */

