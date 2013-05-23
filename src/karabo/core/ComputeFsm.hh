/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 5, 2012, 7:20 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_COMPUTEFSM_HH
#define	KARABO_CORE_COMPUTEFSM_HH

#include "Device.hh"

namespace karabo {
    namespace core {

        class ComputeFsm : public karabo::core::Device {

        public:

            KARABO_CLASSINFO(ComputeFsm, "ComputeFsm", "1.0")

            template <class Derived>
            ComputeFsm(Derived* derived) : Device(derived), m_isAborted(false) {
            }

            virtual ~ComputeFsm() {
            }

            static void expectedParameters(karabo::util::Schema& expected);


            void configure(const karabo::util::Hash& input);


        public:

            /**************************************************************/
            /*                        Events                              */

            /**************************************************************/

            KARABO_FSM_EVENT2(m_fsm, ErrorFoundEvent, onException, std::string, std::string)

            KARABO_FSM_EVENT0(m_fsm, EndErrorEvent, slotEndError)

            KARABO_FSM_EVENT0(m_fsm, ComputeEvent, slotCompute)

            KARABO_FSM_EVENT0(m_fsm, StartRunEvent, slotStartRun)

            KARABO_FSM_EVENT0(m_fsm, EndOfStreamEvent, slotEndOfStream)

            KARABO_FSM_EVENT0(m_fsm, PauseEvent, slotPause)

            KARABO_FSM_EVENT0(m_fsm, AbortEvent, abort)

            KARABO_FSM_EVENT0(m_fsm, ComputeFinishedEvent, computeFinished)

            KARABO_FSM_EVENT0(m_fsm, ResetEvent, slotReset)

            /**************************************************************/
            /*                        States                              */
            /**************************************************************/

            KARABO_FSM_STATE(Idle)

            KARABO_FSM_STATE_V_EE(ConnectingIO, connectingIOOnEntry, connectingIOOnExit)

            KARABO_FSM_STATE_V_E(Ready, readyStateOnEntry)

            KARABO_FSM_STATE_V_EE(Computing, computingStateOnEntry, computingStateOnExit)

            KARABO_FSM_STATE_V_EE(WaitingIO, waitingIOOnEntry, waitingIOOnExit)

            KARABO_FSM_STATE(Paused)

            KARABO_FSM_STATE(Finished)

            KARABO_FSM_STATE(Aborted)

            KARABO_FSM_STATE(Ok)

            KARABO_FSM_STATE(Error)

            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/

            KARABO_FSM_V_ACTION0(StartRunAction, onStartRun)

            /**************************************************************/
            /*                           Guards                           */
            /**************************************************************/

            KARABO_FSM_V_GUARD0(CanCompute, canCompute)

            /**************************************************************/
            /*                      AllOkState Machine                    */
            /**************************************************************/

            //  Source-State      Event    Target-State    Action     Guard
            KARABO_FSM_TABLE_BEGIN(TransitionTable)
            Row< Idle, StartRunEvent, ConnectingIO, StartRunAction, none >,
            Row< ConnectingIO, none, Ready, none, none >,
            Row< Ready, ComputeEvent, Computing, none, none >,
            Row< Ready, PauseEvent, Paused, none, none >,
            Row< Ready, AbortEvent, Aborted, none, none >,
            Row< Computing, ComputeFinishedEvent, WaitingIO, none, none >,
            Row< Computing, AbortEvent, Aborted, none, none >,
            Row< WaitingIO, none, Ready, none, none >,
            Row< Aborted, ResetEvent, Idle, none, none >,
            Row< Finished, ResetEvent, Idle, none, none >,
            Row< Ok, ErrorFoundEvent, Error, ErrorFoundAction, none >,
            Row< Error, ResetEvent, Ok, none, none >
            KARABO_FSM_TABLE_END

            //                         Name       Transition-Table      Initial-State         Context
            KARABO_FSM_STATE_MACHINE(StateMachine, TransitionTable, KARABO_FSM_REGION(Ok, Idle), Self)

            void startStateMachine() {

                KARABO_FSM_CREATE_MACHINE(StateMachine, m_fsm);
                KARABO_FSM_SET_CONTEXT_TOP(this, m_fsm)
                KARABO_FSM_START_MACHINE(m_fsm)
            }


            // Override this function if you need to handle the reconfigured data (e.g. send to a hardware)

            virtual void onReconfigure(karabo::util::Hash& incomingReconfiguration) {
            }

            void run();

            virtual void compute() = 0;

            bool isAborted();

            void onInputAvailable(const karabo::xms::AbstractInput::Pointer&);

            void onOutputPossible(const karabo::xms::AbstractOutput::Pointer&);

        private: // functions


            void slotAbort();

            bool canReadFromAllInputChannels() const;

            bool canWriteToAllOutputChannels() const;

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

