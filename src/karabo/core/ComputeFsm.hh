/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 5, 2012, 7:20 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_COMPUTE_FSM_HH
#define	KARABO_CORE_COMPUTE_FSM_HH

#include "Device.hh"

namespace karabo {
    namespace core {

        class ComputeFsm : public BaseFsm {

            bool m_isAborted;

        public:

            KARABO_CLASSINFO(ComputeFsm, "ComputeFsm", "1.0")


            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::xms;

                SLOT_ELEMENT(expected).key("start")
                        .displayedName("Start Computing")
                        .description("Starts computing if data is available")
                        .allowedStates("Ok.Ready")
                        .commit();

                SLOT_ELEMENT(expected).key("pause")
                        .displayedName("Pause")
                        .description("Will finish current computation and pause")
                        .commit();

                SLOT_ELEMENT(expected).key("abort")
                        .displayedName("Abort")
                        .description("Abort contribution to this run, fully disconnect")
                        .commit();

                SLOT_ELEMENT(expected).key("reset")
                        .displayedName("Reset")
                        .description("Completely reset this device")
                        .allowedStates("Error.WaitingIO")
                        .commit();

                BOOL_ELEMENT(expected).key("autoCompute")
                        .displayedName("Auto Compute")
                        .description("Trigger computation automatically once data is available")
                        .reconfigurable()
                        .assignmentOptional().defaultValue(true)
                        .commit();
            }

            ComputeFsm() : m_isAborted(false) {
            }

            void initFsmSlots() {

                SLOT0(start);
                SLOT0(pause);
                SLOT0(abort);
                SLOT0(reset);
            }

        public:

            /**************************************************************/
            /*                        Events                              */
            /**************************************************************/

            KARABO_FSM_EVENT2(m_fsm, ErrorFoundEvent, errorFound, std::string, std::string)

            KARABO_FSM_EVENT0(m_fsm, ResetEvent, reset)

            KARABO_FSM_EVENT0(m_fsm, StartEvent, start)

            KARABO_FSM_EVENT0(m_fsm, EndOfStreamEvent, endOfStream)

            KARABO_FSM_EVENT0(m_fsm, PauseEvent, pause)

            KARABO_FSM_EVENT0(m_fsm, AbortEvent, abort)

            KARABO_FSM_EVENT0(m_fsm, ComputeFinishedEvent, computeFinished)

            /**************************************************************/
            /*                        States                              */
            /**************************************************************/

            KARABO_FSM_STATE(ConnectingIO)

            KARABO_FSM_STATE_VE_E(Ready, readyStateOnEntry)

            KARABO_FSM_STATE_VE_EE(Computing, computingStateOnEntry, computingStateOnExit)

            KARABO_FSM_STATE_VE_EE(WaitingIO, waitingIOOnEntry, waitingIOOnExit)

            KARABO_FSM_STATE(Paused)

            KARABO_FSM_STATE(Finished)

            KARABO_FSM_STATE(Aborted)

            KARABO_FSM_STATE(Ok)

            KARABO_FSM_INTERRUPT_STATE(Error, ResetEvent)

            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/

            KARABO_FSM_VE_ACTION2(ErrorFoundAction, errorFoundAction, std::string, std::string);

            KARABO_FSM_VE_ACTION0(StartRunAction, onStartRun)

            KARABO_FSM_VE_ACTION0(ConnectAction, connectAction)

            /**************************************************************/
            /*                           Guards                           */
            /**************************************************************/

            KARABO_FSM_V_GUARD0(CanCompute, canCompute)

            /**************************************************************/
            /*                      AllOkState Machine                    */
            /**************************************************************/

            //  Source-State      Event    Target-State    Action     Guard
            KARABO_FSM_TABLE_BEGIN(TransitionTable)
            Row< ConnectingIO, none, Ready, ConnectAction, none >,
            Row< Ready, StartEvent, Computing, none, none >,
            Row< Ready, PauseEvent, Paused, none, none >,
            Row< Ready, AbortEvent, Aborted, none, none >,
            Row< Computing, ComputeFinishedEvent, WaitingIO, none, none >,
            Row< Computing, AbortEvent, Aborted, none, none >,
            Row< WaitingIO, none, Ready, none, none >,
            Row< Aborted, ResetEvent, Ready, none, none >,
            Row< Finished, ResetEvent, Ready, none, none >,
            Row< Ok, ErrorFoundEvent, Error, ErrorFoundAction, none >,
            Row< Error, ResetEvent, Ok, none, none >
            KARABO_FSM_TABLE_END

            //                         Name       Transition-Table      Initial-State         Context
            KARABO_FSM_STATE_MACHINE(StateMachine, TransitionTable, KARABO_FSM_REGION(Ok, ConnectingIO), Self)

            void startStateFsm() {

                KARABO_FSM_CREATE_MACHINE(StateMachine, m_fsm);
                KARABO_FSM_SET_CONTEXT_TOP(this, m_fsm)
                KARABO_FSM_START_MACHINE(m_fsm)
            }

            // Main function to implement
            virtual void compute() = 0;

            bool isAborted();

            void onInputAvailable(const karabo::xms::AbstractInput::Pointer&) {
                m_nInputs++;
            }

        private: // functions

            void slotAbort() {
                m_isAborted = true;
            }

            bool isAborted() {
                return m_isAborted;
            }

            bool canCompute() const {
                return (this->canReadFromAllInputChannels() && this->canWriteToAllOutputChannels());
            }

            bool canReadFromAllInputChannels() const {
                using namespace karabo::xms;
                const InputChannels& inputChannels = this->getInputChannels();
                for (InputChannels::const_iterator it = inputChannels.begin(); it != inputChannels.end(); ++it) {
                    if (!it->second->canCompute()) return false;
                }
                return true;
            }
            
            void connectAction() {
                const InputChannels& inputChannels = this->getInputChannels();
                for (InputChannels::const_iterator it = inputChannels.begin(); it != inputChannels.end(); ++it) {
                    const Hash& tmp = this->get<Hash > (it->first);
                    const Hash& config = tmp.get<Hash > (tmp.begin());
                    std::cout << "Starting run and trying to reconfigure: " << config << std::endl;
                    it->second->reconfigure(config);
                }
                this->connectInputChannels();
            }

            void readyStateOnEntry() {
                if (this->get<bool>("autoCompute")) this->start();
            }

            void computingStateOnEntry() {
                this->compute();
                if (!isAborted()) computeFinished();
            }

            void waitingIOOnEntry() {
                this->updateChannels();
            }

            void updateChannels() {

                const InputChannels& inputChannels = this->getInputChannels();
                const OutputChannels& outputChannels = this->getOutputChannels();

                for (OutputChannels::const_iterator it = outputChannels.begin(); it != outputChannels.end(); ++it) {
                    it->second->update();
                }
                for (InputChannels::const_iterator it = inputChannels.begin(); it != inputChannels.end(); ++it) {
                    it->second->update();
                }
            }

            void doCompute();

            void updateChannels();

        private: // members

            KARABO_FSM_DECLARE_MACHINE(StateMachine, m_fsm);

            bool m_isAborted;

            boost::thread m_computeThread;

            karabo::util::Hash m_channels;

        };

    }
}

#endif

