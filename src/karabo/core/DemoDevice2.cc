/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "DemoDevice2.hh"

using namespace std;
using namespace karabo::util;
using namespace log4cpp;

namespace karabo {
    namespace core {

        KARABO_REGISTER_FACTORY_2_CC(Device, StartStopFsm, DemoDevice2)


        DemoDevice2::~DemoDevice2() {
        }

        void DemoDevice2::expectedParameters(Schema& expected) {
            
            STRING_ELEMENT(expected).key("firstWord")
                    .displayedName("First Word")
                    .description("Input for first word")
                    .assignmentOptional().defaultValue("")
                    .unitName("dimensionless")
                    .unitSymbol("")
                    .allowedStates("AllOkState.StoppedState")
                    .reconfigurable()
                    .commit();
            
            DOUBLE_TARGETACTUAL_ELEMENT(expected).key("temperature")
                    .displayedName("Sensor temperature")
                    .unitName("degree celsius")
                    .unitSymbol("deg")
                    .description("Configures the temperature to which the device should be cooled")
                    .targetAssignmentOptional().targetDefaultValue(0)
                    .targetHardMin(-50)
                    .actualWarnHigh(23)
                    .actualAlarmHigh(40)
                    .commit();
        }

        void DemoDevice2::configure(const Hash& input) {
        }
        
    }
}
