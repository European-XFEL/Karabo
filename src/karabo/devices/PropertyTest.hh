/* 
 * File:   PropertyTest.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on September 5, 2016, 11:08 AM
 */

#ifndef KARABO_DEVICES_PROPERTYTEST_HH
#define KARABO_DEVICES_PROPERTYTEST_HH

#include <boost/shared_ptr.hpp>
#include <karabo/karabo.hpp>

namespace karabo {
    namespace devices {
        
        
        class PropertyTest : public karabo::core::Device<> {
        public:
            
            KARABO_CLASSINFO(PropertyTest, "PropertyTest", "1.5")
            
            static void expectedParameters(karabo::util::Schema& expected);
            
            PropertyTest(const karabo::util::Hash& config);
            
            ~PropertyTest();
            
        private:
            
            void initialize();
            
        };
    } 
}

#endif	/* KARABO_DEVICES_PROPERTYTEST_HH */

