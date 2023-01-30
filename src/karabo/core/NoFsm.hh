/*
 * File:   NoFsm.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on November 27, 2014, 1:23 PM
 */

#ifndef KARABO_CORE_NOFSM_HH
#define KARABO_CORE_NOFSM_HH

#include <boost/bind/bind.hpp>
#include <boost/function.hpp>
#include <karabo/util/ClassInfo.hh>
#include <vector>

namespace karabo {

    namespace util {
        class Schema;
    }

    namespace core {

        /**
         * @class NoFsm
         * @brief Use this class if you do not use an fixed state machine but
         *        rather a simple state machine with in device state updates.
         */
        class NoFsm {
           public:
            KARABO_CLASSINFO(NoFsm, "NoFsm", "1.3")

            static void expectedParameters(karabo::util::Schema& expected) {}

            void initFsmSlots() {}

            virtual ~NoFsm() {}

            void startFsm() {
                // Call second constructors in the same order as first constructors were called
                for (size_t i = 0; i < m_initialFunc.size(); ++i) m_initialFunc[i]();
            }

            virtual void stopFsm() {}

            void registerInitialFunction(const boost::function<void()>& func) {
                m_initialFunc.push_back(func);
            }

#define KARABO_INITIAL_FUNCTION(function) this->registerInitialFunction(boost::bind(&Self::function, this));

           private:
            std::vector<boost::function<void()> > m_initialFunc;
        };
    } // namespace core
} // namespace karabo

#endif /* KARABO_CORE_NOFSM_HH */
