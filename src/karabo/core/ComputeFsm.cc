/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "ComputeFsm.hh"
#include <karabo/xms/SlotElement.hh>

using namespace std;
using namespace karabo::util;
using namespace karabo::xms;
using namespace log4cpp;

namespace karabo {
    namespace core {


        void ComputeFsm::expectedParameters(karabo::util::Schema& expected) {

            SLOT_ELEMENT(expected).key("slotStartRun")
                    .displayedName("StartRun")
                    .description("Starts a new pipeline run")
                    .allowedStates("Ok.Idle")
                    .commit();

            SLOT_ELEMENT(expected).key("slotCompute")
                    .displayedName("Compute")
                    .description("Do a single computation")
                    .allowedStates("Ok.Ready")
                    .commit();

            SLOT_ELEMENT(expected).key("slotAbort")
                    .displayedName("Abort")
                    .description("Abort contribution to this run, fully disconnect")
                    .commit();

            SLOT_ELEMENT(expected).key("slotReset")
                    .displayedName("Reset")
                    .description("Completely reset this device")
                    .allowedStates("Error.WaitingIO")
                    .commit();

            BOOL_ELEMENT(expected).key("autoCompute")
                    .displayedName("Auto Compute")
                    .description("Trigger computation automatically once data is available")
                    .reconfigurable()
                    .allowedStates("Ok.Ready,Ok.WaitingIO,Ok.Idle")
                    .assignmentOptional().defaultValue(true)
                    .commit();
        }


        void ComputeFsm::slotAbort() {
            m_isAborted = true;
        }


        bool ComputeFsm::isAborted() {
            return m_isAborted;
        }


        void ComputeFsm::onInputAvailable(const AbstractInput::Pointer&) {

        }


        void ComputeFsm::onOutputPossible(const AbstractOutput::Pointer&) {

        }


        bool ComputeFsm::canCompute() {

            if (this->canReadFromAllInputChannels() && this->canWriteToAllOutputChannels()) {
                return true;
            } else {
                return false;
            }
        }


        bool ComputeFsm::canReadFromAllInputChannels() const {
            const InputChannels& inputChannels = this->getInputChannels();
            for (InputChannels::const_iterator it = inputChannels.begin(); it != inputChannels.end(); ++it) {
                if (!it->second->canCompute()) return false;
            }
            return true;
        }


        bool ComputeFsm::canWriteToAllOutputChannels() const {
            const OutputChannels& outputChannels = this->getOutputChannels();
            for (OutputChannels::const_iterator it = outputChannels.begin(); it != outputChannels.end(); ++it) {
                if (!it->second->canCompute()) return false;
            }
            return true;
        }


        void ComputeFsm::onStartRun() {
        }


        void ComputeFsm::connectingIOOnEntry() {
            const InputChannels& inputChannels = this->getInputChannels();
            for (InputChannels::const_iterator it = inputChannels.begin(); it != inputChannels.end(); ++it) {
                const Hash& tmp = this->get<Hash > (it->first);
                const Hash& config = tmp.get<Hash > (tmp.begin());
                std::cout << "Starting run and trying to reconfigure: " << config << std::endl;
                it->second->reconfigure(config);
            }
            this->connectInputChannels();
        }


        void ComputeFsm::connectingIOOnExit() {
        }


        void ComputeFsm::readyStateOnEntry() {
            if (this->get<bool>("autoCompute")) this->slotCompute();
        }


        void ComputeFsm::computingStateOnEntry() {
            this->compute();
            if (!isAborted()) computeFinished();
        }


        void ComputeFsm::computingStateOnExit() {
        }


        void ComputeFsm::waitingIOOnEntry() {
            this->updateChannels();
        }


        void ComputeFsm::updateChannels() {

            const InputChannels& inputChannels = this->getInputChannels();
            const OutputChannels& outputChannels = this->getOutputChannels();

            for (OutputChannels::const_iterator it = outputChannels.begin(); it != outputChannels.end(); ++it) {
                it->second->update();
            }
            for (InputChannels::const_iterator it = inputChannels.begin(); it != inputChannels.end(); ++it) {
                it->second->update();
            }
        }


        void ComputeFsm::waitingIOOnExit() {
        }
    }
}
