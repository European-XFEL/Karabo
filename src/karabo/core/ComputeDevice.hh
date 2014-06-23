/* 
 * $Id$
 * 
 * Author: <burkhard.heisen@xfel.eu>
 * 
 * Created on July 22, 2013, 9:10 AM
 * 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_CORE_COMPUTEDEVICE_HH
#define	KARABO_CORE_COMPUTEDEVICE_HH


#include "Device.hh"

namespace karabo {
    namespace core {

        class ComputeDevice : public Device<> {

            bool m_isAborted;
            bool m_isEndOfStream;
            bool m_deviceIsDead;
            unsigned int m_nEndOfStreams;
            unsigned int m_iterationCount;
            
            boost::thread m_computeThread;
            boost::thread m_waitingIOThread;
            boost::mutex m_computeMutex;
            boost::mutex m_waitingIOMutex;
            boost::condition_variable m_computeCond;
            boost::condition_variable m_waitingIOCond;
           

        public:

            KARABO_CLASSINFO(ComputeDevice, "ComputeDevice", "1.0")
            
            #define KARABO_INPUT_CHANNEL(type, name, configuration) this->createInputChannel<type>(name, configuration, boost::bind(&karabo::core::ComputeDevice::_onInputAvailable, this, _1), boost::bind(&karabo::core::ComputeDevice::_onEndOfStream, this) );
            #define KARABO_OUTPUT_CHANNEL(type, name, configuration) this->createOutputChannel<type>(name, configuration);
            
            static void expectedParameters(karabo::util::Schema& expected);

            ComputeDevice(const karabo::util::Hash& input);

            virtual ~ComputeDevice();
            
            /**
             * Put your specific algorithms here
             */
            virtual void compute() = 0;
            
            /**
             * Override this function for specializing the endOfStream behavior
             */
            virtual void onEndOfStream();
            
            /**
             * Retrieves the current iteration count
             */
            int getCurrentIteration() const;
            
            /**
             * Override this function for specializing the update behaviors of your IO channels
             * Please now what are you doing!
             */            
            virtual void update();
           
            
            void _onInputAvailable(const karabo::io::AbstractInput::Pointer&);
            
            void _onEndOfStream();
                
            bool isAborted() const;

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
            
            KARABO_FSM_EVENT0(m_fsm, UpdatedIOEvent, updatedIO)
            
            /**************************************************************/
            /*                        States                              */
            /**************************************************************/
            
            KARABO_FSM_STATE(Ok)

            KARABO_FSM_INTERRUPT_STATE(Error, ResetEvent)
            
            KARABO_FSM_STATE(ConnectingIO)

            KARABO_FSM_STATE_V_E(Ready, readyStateOnEntry)

            KARABO_FSM_STATE_V_EE(Computing, computingStateOnEntry, computingStateOnExit)

            KARABO_FSM_STATE_V_EE(WaitingIO, waitingIOOnEntry, waitingIOOnExit)

            KARABO_FSM_STATE(Paused)

            KARABO_FSM_STATE_V_EE(Finished, finishedOnEntry, finishedOnExit)

            KARABO_FSM_STATE_V_E(Aborted, abortedOnEntry)
            
            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/

            KARABO_FSM_VE_ACTION2(ErrorFoundAction, errorFoundAction, std::string, std::string);

            KARABO_FSM_VE_ACTION0(ResetAction, resetAction)

            KARABO_FSM_V_ACTION0(ConnectAction, connectAction)
            
            KARABO_FSM_V_ACTION0(EndOfStreamAction, endOfStreamAction)
            
            KARABO_FSM_VE_ACTION0(NextIterationAction, onNextIteration)
            
            /**************************************************************/
            /*                           Guards                           */
            /**************************************************************/

            KARABO_FSM_V_GUARD0(CanCompute, canCompute)
            
            KARABO_FSM_V_GUARD0(AbortGuard, registerAbort)
            
            KARABO_FSM_V_GUARD0(PauseGuard, registerPause)
            
            /**************************************************************/
            /*                      AllOkState Machine                    */
            /**************************************************************/

            //  Source-State      Event    Target-State    Action     Guard
            KARABO_FSM_TABLE_BEGIN(TransitionTable)
            Row< ConnectingIO, none, Ready, ConnectAction, none >,
            Row< Ready, StartEvent, Computing, none, CanCompute >,
            Row< Ready, PauseEvent, Paused, none, none >,
            Row< Ready, AbortEvent, Aborted, none, none >,
            Row< Ready, EndOfStreamEvent, Finished, EndOfStreamAction, none >,
            Row< Paused, ResetEvent, Ready, none, CanCompute >,
            Row< Computing, ComputeFinishedEvent, WaitingIO, none, none >,
            Row< Computing, AbortEvent, Aborted, none, AbortGuard >,
            Row< Computing, PauseEvent, Paused, none, PauseGuard >,
            Row< WaitingIO, UpdatedIOEvent, Ready, none, none >,
            Row< Aborted, ResetEvent, Ready, none, none >,
            Row< Finished, ResetEvent, Ready, none, none >,
            Row< Finished, StartEvent, Computing, NextIterationAction, none>,
            Row< Ok, ErrorFoundEvent, Error, ErrorFoundAction, none >,
            Row< Error, ResetEvent, Ok, ResetAction, none >
            KARABO_FSM_TABLE_END

            //                         Name       Transition-Table      Initial-State         Context
            KARABO_FSM_STATE_MACHINE(StateMachine, TransitionTable, KARABO_FSM_REGION(Ok, ConnectingIO), Self)

            void startFsm() {

                KARABO_FSM_CREATE_MACHINE(StateMachine, m_fsm);
                KARABO_FSM_SET_CONTEXT_TOP(this, m_fsm)
                KARABO_FSM_START_MACHINE(m_fsm)
            }
            
        private:
            
            KARABO_FSM_DECLARE_MACHINE(StateMachine, m_fsm);
            
            void doCompute();
            void doWait();
        };
    }
}

#endif

