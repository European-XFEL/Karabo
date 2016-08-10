/* 
 * File:   AlarmService.cc
 * Author: haufs
 * 
 * Created on August 5, 2016, 11:30 AM
 */

#include "AlarmService.hh"
#include "karabo/util/State.hh"

namespace karabo {
    
   
    
    namespace core {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, AlarmService)


        void AlarmService::expectedParameters(Schema& expected) {
            
            Schema tableRow;
            
            STRING_ELEMENT(tableRow).key("timeOfOccurrence")
                    .displayedName("Occurred at")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("timeOfFirstOccurrence")
                    .displayedName("First occurred at")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("deviceId")
                    .displayedName("Device")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("property")
                    .displayedName("Property")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("severity")
                    .displayedName("Severity")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("description")
                    .displayedName("Description")
                    .readOnly()
                    .commit();
            
            BOOL_ELEMENT(tableRow).key("needsAcknowledging")
                    .displayedName("ack?")
                    .readOnly()
                    .commit();
            
            BOOL_ELEMENT(tableRow).key("acknowledged")
                    .displayedName("Acknowledged")
                    .assignmentOptional().defaultValue(false)
                    .reconfigurable()
                    .commit();
            
            TABLE_ELEMENT(expected).key("currentAlarms")
                    .displayedName("Current Alarms")
                    .setNodeSchema(tableRow)
                    .assignmentOptional().noDefaultValue()
                    .reconfigurable()
                    .commit();
                  
        }

            


        AlarmService::AlarmService(const karabo::util::Hash& input) :
            karabo::core::Device<>(input){
            KARABO_INITIAL_FUNCTION(initialize);
        }
     
        AlarmService::~AlarmService() {
        }
        
        void AlarmService::initialize() {
            updateState(State::INIT);
            
        }
    }
}

