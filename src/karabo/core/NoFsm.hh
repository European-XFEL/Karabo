/* 
 * File:   NoFsm.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on November 27, 2014, 1:23 PM
 */

#ifndef KARABO_CORE_NOFSM_HH
#define	KARABO_CORE_NOFSM_HH


#include <karabo/xms/SlotElement.hh>
#include "Device.hh"

namespace karabo {
    namespace core {

        class NoFsm {
        public:

            KARABO_CLASSINFO(NoFsm, "NoFsm", "1.3")

            static void expectedParameters(karabo::util::Schema& expected) {
            }

            void initFsmSlots() {
            }

            virtual void stopWorkers() {}
            
        public:

            virtual ~NoFsm() {
            }                      
            
            void startFsm() {
                if (!m_initialFunc)
                    throw KARABO_PARAMETER_EXCEPTION("No initial function defined. Please call 'initialFunc' function in the constructor");
                m_initialFunc();
            }

            void registerInitialFunction(const boost::function<void()>& func) {
                m_initialFunc = func;
            }

        private:

            boost::function<void() > m_initialFunc;

        };
    }
}

#endif	/* KARABO_CORE_NOFSM_HH */

