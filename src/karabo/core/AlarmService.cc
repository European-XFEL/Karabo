/* 
 * File:   AlarmService.cc
 * Author: haufs
 * 
 * Created on August 5, 2016, 11:30 AM
 */

#include "AlarmService.hh"

namespace karabo {
    namespace core {

        using namespace krb_log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<karabo::core::OkErrorFsm>, AlarmService)


        void AlarmService::expectedParameters(Schema& expected) {
            
            Schema tableRow;
            
            STRING_ELEMENT(tableRow).key("timeOfOccurance")
                    .displayedName("Occured at")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("timeOfFirstOccurance")
                    .displayedName("First occured at")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(tableRow).key("deviceId")
                    .displayedName("Device")
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
                    .assignmentOptional().noDefaultValue()
                    .reconfigurable()
                    .commit();
                  
        }

            


        AlarmService::AlarmService(const karabo::util::Hash& input) :
            karabo::core::Device<karabo::core::OkErrorFsm>(input){
        }
     
        AlarmService::~AlarmService() {
        }
    }
}

