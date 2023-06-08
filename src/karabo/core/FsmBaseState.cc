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
#include "FsmBaseState.hh"

namespace karabo {
    namespace core {

        void StateVisitor::visitState(const FsmBaseState* state, bool stopWorker) {
            if (stopWorker) {
                Worker* worker = state->getWorker();
                if (worker && worker->is_running()) worker->abort().join();
            } else {
                std::string stateName(state->getStateName());
                std::string fsmName(state->getFsmName());
                // Technical correction:
                // if state-machine and state are the same name, the state is subcomposed into the former machine
                if (stateName == fsmName) fsmName = m_currentFsm;

                m_state = state;
                m_stateName = stateName;
                m_currentFsm = fsmName;
            }
        }

    } // namespace core
} // namespace karabo
