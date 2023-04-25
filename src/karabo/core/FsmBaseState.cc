/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
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
