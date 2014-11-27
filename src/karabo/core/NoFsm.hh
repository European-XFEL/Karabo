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

        class NoFsm : public BaseFsm {
        public:

            KARABO_CLASSINFO(NoFsm, "NoFsm", "1.0")


            static void expectedParameters(karabo::util::Schema& expected) {
            }

            void initFsmSlots() {
            }

        public:

            virtual ~NoFsm() {
            }

            virtual void errorFound(const std::string&, const std::string&) = 0;

            void onNoStateTransition(const std::string& typeId, int state) {
            }

            void startFsm() {
                if (!m_initialFunc)
                    throw KARABO_PARAMETER_EXCEPTION("No initial function defined. Please call 'initialFunc' function in the constructor");
                m_initialFunc();
            }

            void initialFunc(const boost::function<void()>& func) {
                m_initialFunc = func;
            }

        private:

            void exceptionFound(const std::string&, const std::string&) const {
            }

            boost::function<void() > m_initialFunc;

        };
    }
}

#endif	/* KARABO_CORE_NOFSM_HH */

