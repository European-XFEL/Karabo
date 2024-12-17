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
 * File:   FsmBaseState.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on June 8, 2016, 3:45 PM
 */

#ifndef FSMBASESTATE_HH
#define FSMBASESTATE_HH

#include <memory>
#include <string>
// back-end
#include <boost/msm/back/state_machine.hpp>
// front-end
#include <boost/msm/front/state_machine_def.hpp>
// functors
#include <boost/msm/front/euml/common.hpp>
#include <boost/msm/front/functor_row.hpp>
// for And_ operator
#include <boost/msm/front/euml/operator.hpp>
// for func_state and func_state_machine
#include <boost/msm/front/euml/state_grammar.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/cat.hpp>

#include "Worker.hh"
#include "karabo/util/State.hh"

// Allow boost msm names appear globally in karabo namespace
namespace karabo {
    using boost::msm::front::none;
    using boost::msm::front::Row;
} // namespace karabo

namespace karabo {
    namespace core {

        class FsmBaseState;

        struct StateVisitor {
            void visitState(const FsmBaseState* state, bool stopWorker = false);

            const FsmBaseState* getState() {
                return m_state;
            }

           private:
            const FsmBaseState* m_state;
            std::string m_stateName;
            std::string m_currentFsm;
        };

        struct FsmBaseState {
            FsmBaseState()
                : m_state(karabo::util::State::UNKNOWN),
                  m_fsmName(),
                  m_isContained(false),
                  m_timeout(-1),
                  m_repetition(-1) {}

            const karabo::util::State& getState() const {
                return m_state;
            }

            void setStateMachineName(const std::string& name) {
                m_stateMachineName = name;
            }

            const karabo::util::State* parent() const {
                return m_state.parent();
            }

           public:
            bool isDerivedFrom(const karabo::util::State& s) const {
                return m_state.isDerivedFrom(s);
            }

            const std::string& getFsmName() const {
                return m_fsmName;
            }

            virtual void setFsmName(const std::string& fsmName) {
                m_fsmName = fsmName;
            }

            const bool& isContained() const {
                return m_isContained;
            }

            void setContained(bool isContained) {
                m_isContained = isContained;
            }

            // Signature of the accept function
            typedef boost::msm::back::args<void, std::shared_ptr<StateVisitor>, bool> accept_sig;

            // This makes states polymorphic

            virtual ~FsmBaseState() {}

            // Default implementation for states who need to be visited
            void accept(std::shared_ptr<StateVisitor> visitor, bool stopWorker) const {
                visitor->visitState(this, stopWorker);
            }

            void setTimeout(int timeout) {
                m_timeout = timeout;
            }

            int getTimeout() const {
                return m_timeout;
            }

            void setRepetition(int cycles) {
                m_repetition = cycles;
            }

            int getRepetition() const {
                return m_repetition;
            }

            virtual Worker* getWorker() const {
                return NULL;
            }

            const std::string& getStateName() const {
                if (m_state.name().empty()) return m_stateMachineName;
                return m_state.name();
            }

            const std::string& name() const {
                return this->getStateName();
            }

           protected:
            void setState(const karabo::util::State& state) {
                m_state = state;
            }

            karabo::util::State m_state;
            std::string m_stateMachineName;
            std::string m_fsmName;
            bool m_isContained;
            int m_timeout;
            int m_repetition;
        };

    } // namespace core
} // namespace karabo


#endif /* FSMBASESTATE_HH */
