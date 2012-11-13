/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on October 4, 2011, 7:20 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_CAMERAFSM_HH
#define	KARABO_CORE_CAMERAFSM_HH

#include "Device.hh"

namespace karabo {
    namespace core {

        class CameraFsm : public karabo::core::Device {
        public:

            KARABO_CLASSINFO(CameraFsm, "CameraFsm", "1.0")

            template <class Derived>
            CameraFsm(Derived* derived) : Device(derived) {
            }

            virtual ~CameraFsm() {
            }

            static void expectedParameters(karabo::util::Schema& expected);

            void configure(const karabo::util::Hash& input);
            
            virtual void run();

        public:

            /**************************************************************/
            /*                        Events                              */
            /**************************************************************/

            // Standard events

            KARABO_FSM_EVENT2(m_fsm, ErrorFoundEvent, onException, std::string, std::string)

            KARABO_FSM_EVENT0(m_fsm, EndErrorEvent, slotEndError)

            KARABO_FSM_EVENT0(m_fsm, AcquireEvent, slotAcquire)

            KARABO_FSM_EVENT0(m_fsm, StopEvent, slotStop)

            KARABO_FSM_EVENT0(m_fsm, TriggerEvent, slotTrigger)

            /**************************************************************/
            /*                        States                              */
            /**************************************************************/

            KARABO_FSM_STATE_V_EE(Error, errorStateOnEntry, errorStateOnExit)

            KARABO_FSM_STATE_V_EE(HardwareSetup, hardwareSetupStateOnEntry, hardwareSetupStateOnExit)

            KARABO_FSM_STATE_V_EE(Acquisition, acquisitionStateOnEntry, acquisitionStateOnExit)

            KARABO_FSM_STATE_V_EE(Ready, readyStateOnEntry, readyStateOnExit)

            /**************************************************************/
            /*                    Transition Actions                      */
            /**************************************************************/

            KARABO_FSM_V_ACTION0(AcquireAction, acquireAction)

            KARABO_FSM_V_ACTION0(StopAction, stopAction)

            KARABO_FSM_V_ACTION0(TriggerAction, triggerAction)

            /**************************************************************/
            /*                      AllOkState Machine                    */
            /**************************************************************/

            KARABO_FSM_TABLE_BEGIN(AllOkStateTransitionTable)
            //  Source-State      Event    Target-State    Action     Guard
            Row< Ready, AcquireEvent, Acquisition, AcquireAction, none >,
            Row< Acquisition, StopEvent, Ready, StopAction, none >,
            Row< Acquisition, TriggerEvent, none, TriggerAction, none >
            KARABO_FSM_TABLE_END

            //                       Name      Transition-Table           Initial-State  Context
            KARABO_FSM_STATE_MACHINE(AllOk, AllOkStateTransitionTable, Ready, Self)

            /**************************************************************/
            /*                      Top Machine                         */
            /**************************************************************/

            //  Source-State    Event        Target-State    Action         Guard
            KARABO_FSM_TABLE_BEGIN(CameraMachineTransitionTable)
            Row< HardwareSetup, none, AllOk, none, none >,
            Row< AllOk, ErrorFoundEvent, Error, ErrorFoundAction, none >,
            Row< Error, EndErrorEvent, AllOk, none, none >
            KARABO_FSM_TABLE_END


            //                                 Name                   Transition-Table       Initial-State Context
            KARABO_FSM_STATE_MACHINE(CameraMachine, CameraMachineTransitionTable, HardwareSetup, Self)


            void startStateMachine() {

                KARABO_FSM_CREATE_MACHINE(CameraMachine, m_fsm);
                KARABO_FSM_SET_CONTEXT_TOP(this, m_fsm)
                KARABO_FSM_SET_CONTEXT_SUB(this, m_fsm, AllOk)
                KARABO_FSM_START_MACHINE(m_fsm)
            }


            // Override this function if you need to handle the reconfigured data (e.g. send to a hardware)

            virtual void onReconfigure(karabo::util::Hash& incomingReconfiguration) {
            }

        protected: // Functions and Classes
            
            template <class T>
            bool ensureSoftwareHardwareConsistency(const std::string key, const T& targetValue, const T& actualValue, karabo::util::Hash& configuration) {
                // TODO One should maybe think of a more sophisticated method than converting to string and compare those...
                if (karabo::util::String::toString(targetValue) != karabo::util::String::toString(actualValue)) {
                    
                    std::ostringstream msg;
                    msg << "Hardware rejected to accept (re-)configuration for key \"" << key << "\" to target \""
                            << karabo::util::String::toString(targetValue) << "\". Actual value is \"" << karabo::util::String::toString(actualValue) << "\"";
                    log() << log4cpp::Priority::WARN << msg.str();
                    emit("signalBadReconfiguration", msg.str(), getInstanceId());
                    configuration.set(key, actualValue);
                    return false;
                } else return true;
            }
            
        protected: // Members
            
            
        private: // functions
            
            
        private: // members
            
            KARABO_FSM_DECLARE_MACHINE(CameraMachine, m_fsm);
            
        };

    }
}

#endif

