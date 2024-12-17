/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   NoFsm.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on November 27, 2014, 1:23 PM
 */

#ifndef KARABO_CORE_NOFSM_HH
#define KARABO_CORE_NOFSM_HH

#include <functional>
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

            void registerInitialFunction(const std::function<void()>& func) {
                m_initialFunc.push_back(func);
            }

#define KARABO_INITIAL_FUNCTION(function) this->registerInitialFunction(std::bind(&Self::function, this));

           private:
            std::vector<std::function<void()> > m_initialFunc;
        };
    } // namespace core
} // namespace karabo

#endif /* KARABO_CORE_NOFSM_HH */
