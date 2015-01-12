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
            
            void slotLoadProject(const karabo::util::Hash& hash);
            
            void slotSaveProject(const karabo::util::Hash& hash);
            
            void slotCloseProject(const karabo::util::Hash& hash);            
            

        };
    }
}
#endif	/* PROJECTMANAGER_HH */

