/*
 * File:   NoFsm.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on November 27, 2014, 1:23 PM
 */

#ifndef KARABO_CORE_NOFSM_HH
#define	KARABO_CORE_NOFSM_HH

#include <vector>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <karabo/xms/SlotElement.hh>
#include <karabo/util/ClassInfo.hh>
#include <karabo/util/Schema.hh>

namespace karabo {
    namespace core {

        class NoFsm {

        public:

            KARABO_CLASSINFO(NoFsm, "NoFsm", "1.3")

            static void expectedParameters(karabo::util::Schema& expected) {
            }

            void initFsmSlots() {
            }

            virtual ~NoFsm() {
            }

            void startFsm() {
                //if (m_initialFunc.empty())
                //    throw KARABO_LOGIC_EXCEPTION("No initial function defined. Please use  \"KARABO_INITIAL_FUNCTION(your_initial_function)\" macro in the device constructor");
                // Call second constructors in the same order as first constructors were called
                for (size_t i = 0; i < m_initialFunc.size(); ++i) m_initialFunc[i]();
            }

            virtual void stopFsm() {
            }

            void registerInitialFunction(const boost::function<void()>& func) {
                m_initialFunc.push_back(func);
            }

#define KARABO_INITIAL_FUNCTION(function) this->registerInitialFunction(boost::bind(&Self::function, this));

        private:

            std::vector<boost::function<void() > > m_initialFunc;

        };
    }
}

#endif	/* KARABO_CORE_NOFSM_HH */

