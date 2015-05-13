/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on April 14, 2011, 7:53 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_MOTORINTERFACE_HH
#define	KARABO_CORE_MOTORINTERFACE_HH

#include <karabo/xms/SlotElement.hh>
#include "Device.hh"

namespace karabo {
    namespace core {

        class MotorInterface : public virtual karabo::xms::SignalSlotable {
        public:

            KARABO_CLASSINFO(MotorInterface, "MotorInterface", "1.0")


            static void expectedParameters(karabo::util::Schema& expected) {

                using namespace karabo::xms;
                using namespace karabo::util;
                
                OVERWRITE_ELEMENT(expected).key("state")
                        .setNewOptions("Initializing,HardwareError,Safe,Override,Off,Stopped,Idle,Homing,Moving")
                        .setNewDefaultValue("Initializing")
                        .commit();
                
                SLOT_ELEMENT(expected).key("resetHardware")                      
                        .description("Resets the hardware")
                        .displayedName("Reset hardware")
                        .allowedStates("HardwareError")
                        .commit();

                SLOT_ELEMENT(expected).key("safe")                        
                        .description("Brings device into a safe operation mode (as defined on h/w)")
                        .displayedName("Safe")
                        .commit();

                SLOT_ELEMENT(expected).key("normal")                       
                        .displayedName("Normal")
                        .description("Brings device into normal operation mode")
                        .expertAccess()
                        .commit();

                SLOT_ELEMENT(expected).key("override")                        
                        .displayedName("Override")
                        .description("Brings device into override operation mode (be careful, hardware may be broken)")
                        .adminAccess()
                        .commit();

                SLOT_ELEMENT(expected).key("off")
                        .displayedName("Off")
                        .description("Instructs device to switch off")
                        .allowedStates("Override Stopped Idle Homing")
                        .commit();

                SLOT_ELEMENT(expected).key("on")                        
                        .displayedName("On")
                        .description("Instructs device to switch on")
                        .allowedStates("Override Off")
                        .commit();

                SLOT_ELEMENT(expected).key("stop")
                        .displayedName("Stop")
                        .description("Instructs the device to switch on and stopped")
                        .allowedStates("Override Idle Moving Homing")
                        .commit();

                SLOT_ELEMENT(expected).key("home")
                        .displayedName("Home")
                        .description("Find home position")
                        .allowedStates("Override Stopped")
                        .commit();

                SLOT_ELEMENT(expected).key("move")
                        .displayedName("Move")
                        .description("Move position")
                        .allowedStates("Override Stopped")
                        .commit();

                SLOT_ELEMENT(expected).key("stepUp")
                        .displayedName("Step up")
                        .description("Step up")
                        .allowedStates("Override Idle Stopped")
                        .commit();

                SLOT_ELEMENT(expected).key("stepDown")
                        .displayedName("Step down")
                        .description("Step down")
                        .allowedStates("Override Idle Stopped")
                        .commit();

                FLOAT_ELEMENT(expected).key("encoderPosition")
                        .description("Encoder position")
                        .displayedName("Encoder position")
                        .unit(Unit::METER)
                        .metricPrefix(MetricPrefix::MILLI)
                        .readOnly()
                        .commit();

                FLOAT_ELEMENT(expected).key("stepCounterPosition")
                        .displayedName("Stepcounter position")
                        .description("The step counter position describes the motor position calculated from counter steps (instead of encoder values), and is only valid if connected to external encoder")
                        .expertAccess()
                        .readOnly()
                        .commit();
                
                FLOAT_ELEMENT(expected).key("targetPosition")
                        .description("Target position in position mode")
                        .displayedName("Target position")
                        .unit(Unit::METER)
                        .metricPrefix(MetricPrefix::MILLI)
                        .assignmentOptional().noDefaultValue()
                        .reconfigurable()
                        .allowedStates("Override Stopped Off Idle Moving")
                        .commit();

                INT16_ELEMENT(expected).key("targetVelocity")                        
                        .description("Target velocity in velocity mode")
                        .displayedName("Target velocity")
                        .assignmentOptional().noDefaultValue()
                        .reconfigurable()
                        .allowedStates("Override Stopped Off Idle Moving")
                        .expertAccess()
                        .commit();
            }
            
            virtual ~MotorInterface() {
            }

            void initFsmSlots() {
                KARABO_SLOT(resetHardware)
                KARABO_SLOT(safe)
                KARABO_SLOT(normal)
                KARABO_SLOT(override)
                KARABO_SLOT(off)
                KARABO_SLOT(on)
                KARABO_SLOT(stop)
                KARABO_SLOT(home)
                KARABO_SLOT(move)
                KARABO_SLOT(stepUp)
                KARABO_SLOT(stepDown)
            }                        
            
            virtual void resetHardware() = 0;
            
            virtual void safe() = 0;
            
            virtual void normal() = 0;
            
            virtual void override() = 0;
            
            virtual void off() = 0;
            
            virtual void on() = 0;
            
            virtual void stop() = 0;
            
            virtual void home() = 0;
            
            virtual void move() = 0;
            
            virtual void stepUp() = 0;
            
            virtual void stepDown() = 0;
            
            virtual void initialize() = 0;
            
            void startFsm() {
                this->initialize();
            }
            
            void stopFsm() {};
           
        };
    }
}

#endif

