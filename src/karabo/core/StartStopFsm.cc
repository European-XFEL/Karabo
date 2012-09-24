/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "StartStopFsm.hh"
#include <karabo/xms/SlotElement.hh>

using namespace std;
using namespace exfel::util;
using namespace exfel::xms;
using namespace log4cpp;

namespace exfel {
    namespace core {
        
        void StartStopFsm::expectedParameters(exfel::util::Schema& expected) {
            
            SLOT_ELEMENT(expected).key("slotStart")
                    .displayedName("Start")
                    .description("Instructs device to go to started state")
                    .allowedStates("AllOkState.StoppedState")
                    .commit();
            
            SLOT_ELEMENT(expected).key("slotStop")
                    .displayedName("Stop")
                    .description("Instructs device to go to stopped state")
                    .allowedStates("AllOkState.StartedState")
                    .commit();
            
             
            SLOT_ELEMENT(expected).key("slotEndError")
                    .displayedName("Reset")
                    .description("Resets the device in case of an error")
                    .allowedStates("ErrorState")
                    .commit();
        }
        
        void StartStopFsm::configure(const exfel::util::Hash& input) {
            SLOT0(slotEndError)
            SLOT0(slotStart)
            SLOT0(slotStop)
        }
        
        void StartStopFsm::run() {
            startStateMachine();
            runEventLoop();
        }
        
        void StartStopFsm::initializationStateOnEntry() {
            
        }
        
        void StartStopFsm::initializationStateOnExit() {
            
        }
        
        void StartStopFsm::errorStateOnEntry() {
            
        }
        
        void StartStopFsm::errorStateOnExit() {
            
        }
        
        void StartStopFsm::startedStateOnEntry() {
            
        }
        
        void StartStopFsm::startedStateOnExit() {
            
        }
        
        void StartStopFsm::stoppedStateOnEntry() {
            
        }
        
        void StartStopFsm::stoppedStateOnExit() {
            
        }
        
        void StartStopFsm::startAction() {
            
        }
        
        void StartStopFsm::stopAction() {
            
        }
        
    }
}
