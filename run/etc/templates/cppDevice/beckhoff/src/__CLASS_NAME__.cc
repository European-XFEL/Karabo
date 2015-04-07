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


    KARABO_REGISTER_BECKHOFF_DEVICE(__CLASS_NAME__)

    void __CLASS_NAME__::expectedParameters(Schema& expected) {
                
        SLOT_ELEMENT(expected).key("start")
                .alias<AliasType > (0x31) // The plcKey of this command
                .displayedName("Start") // The diplayed label in GUI
                .description("Instructs device to start") // Tooltip information
                .allowedStates("Override Stopped") // States in which it is allowed to send this command
                .commit();

        SLOT_ELEMENT(expected).key("stop")
                .alias<AliasType > (0x32)
                .displayedName("Stop")
                .description("Instructs device to stop")
                .allowedStates("Override Started")
                .commit();

        FLOAT_ELEMENT(expected).key("dummyWriteProperty")
                .alias<AliasType > (0x121)
                .tags("plc") // Important to tag, else this property won't be send down to PLC
                .description("This will set a dummy property")
                .displayedName("Dummy write property")
                .unit(Unit::METER)
                .metricPrefix(MetricPrefix::MILLI)
                .assignmentOptional().noDefaultValue() // This is good practice for Beckhoff
                .reconfigurable()
                .allowedStates("Override Stopped")
                .commit();
              
        INT32_ELEMENT(expected).key("dummyReadProperty") 
                .alias<AliasType >(0x128)
                .displayedName("Dummy read property")
                .description("The will read out the value of the dummy read property")
                .expertAccess()
                .readOnly()
                .commit();                
    }


    __CLASS_NAME__::__CLASS_NAME__(const karabo::util::Hash& config) : BeckhoffDevice(config) {
        
        // start and stop are regular c++ functions, this macro makes the callable remotely
        SLOT0(start)
        SLOT0(stop)
        
    }


    __CLASS_NAME__::~__CLASS_NAME__() {
    }
    
    
    void __CLASS_NAME__::start() {
        // The command string "start" must be the stringified version of the function name
        // The same string must be used correctly in the expected parameters as well as in the call below.
        sendCommandToPlc("start");
    }
    
    
    void __CLASS_NAME__::stop() {
        sendCommandToPlc("stop");
    }
    
    
    std::string __CLASS_NAME__::decodeHardwareState(const std::bitset<32>& hardwareStatusBitField) {
        
        // Decode the bits into string, e.g.
        // NOTE: Error handling is done in the base class
        if (bits.test(12)) return "Started";
        return "Stopped";
    }
    
    
    void __CLASS_NAME__::onHardwareStatusUpdate(const std::string& hwState) {
        
        std::string swState = get<string > ("state");
                
        KARABO_LOG_DEBUG << "onHardwareStatusUpdate hwState: " << hwState << " swState: " << swState;
        
        // White-list possible transitions
        if (swState == "Started" && hwState == "Stopped") {
            updateState(hwState); // Important to update the state
        }
        
        else if (swState == "Stopped" && hwState == "Started") {
            updateState(hwState);
        }
        
        else if (swState != hwState) {
            handleSoftwareHardwareInconsistency(swState, hwState);
        }            
    }
}
