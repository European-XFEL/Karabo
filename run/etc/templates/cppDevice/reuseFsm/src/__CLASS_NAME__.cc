/*
 * $Id$
 *
 * Author: <__EMAIL__>
 * 
 * Created on __DATE__
 *
 * Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved.
 */

#include "__CLASS_NAME__.hh"

using namespace std;

USING_KARABO_NAMESPACES

namespace karabo {

    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<StartStopFsm>, __CLASS_NAME__)
    
    void __CLASS_NAME__::expectedParameters(Schema& expected) {

        FLOAT_ELEMENT(expected).key("targetSpeed")
                .displayedName("Target Conveyor Speed")
                .description("Configures the speed of the conveyor belt")
                .unit(Unit::METER_PER_SECOND)
                .assignmentOptional().defaultValue(1.0)
                .reconfigurable()
                .commit();

        FLOAT_ELEMENT(expected).key("currentSpeed")
                .displayedName("Current Conveyor Speed")
                .description("Shows the current speed of the conveyor")
                .readOnly()
                .commit();

        BOOL_ELEMENT(expected).key("reverseDirection")
                .displayedName("Reverse Direction")
                .description("Reverses the direction of the conveyor band")
                .assignmentOptional().defaultValue(false)
                .allowedStates("Ok.Stopped")
                .reconfigurable()
                .commit();
    }
    
    __CLASS_NAME__::__CLASS_NAME__(const karabo::util::Hash& config) : Device<StartStopFsm>(config) {
    }

    void __CLASS_NAME__::initializationStateOnEntry() {

        KARABO_LOG_INFO << "Connecting to conveyer hardware, setting up motors...";

        // Initialize read only variable
        set<float>("currentSpeed", 0.0);
    }
    
    void __CLASS_NAME__::startAction() {
        
        // Retrieve current values from our own device-state
        float tgtSpeed = get<float>("targetSpeed");
        float currentSpeed = get<float>("currentSpeed");
        
        // If we do not stand still here that is an error
        if (currentSpeed > 0.0) throw KARABO_LOGIC_EXCEPTION("Conveyer does not stand still at start-up");
        
        // Separate ramping into 50 steps
        float increase = tgtSpeed / 50.0;
        
        // Simulate a slow ramping up of the conveyor
        for (int i = 0; i < 50; ++i) {
            currentSpeed += increase;
            set("currentSpeed", currentSpeed);
            boost::this_thread::sleep(boost::posix_time::millisec(50));
        }
        // Be sure to finally run with targetSpeed
        set<float>("currentSpeed", tgtSpeed); 
    }

    void __CLASS_NAME__::stopAction() {
        
        // Retrieve current value from our own device-state
        float currentSpeed = get<float>("currentSpeed");
        
        // Separate ramping into 50 steps
        float decrease = currentSpeed / 50.0;
        
        // Simulate a slow ramping down of the conveyor
        for (int i = 0; i < 50; ++i) {
            currentSpeed -= decrease;
            set("currentSpeed", currentSpeed);
            boost::this_thread::sleep(boost::posix_time::millisec(50));
        }
        // Be sure to finally stand still
        set<float>("currentSpeed", 0);
    }
}
