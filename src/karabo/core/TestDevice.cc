/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "TestDevice.hh"

using namespace std;
using namespace exfel::util;
using namespace log4cpp;

namespace exfel {
    namespace core {

        EXFEL_REGISTER_FACTORY_2_CC(Device, ReconfigurableFsm, TestDevice)


        TestDevice::~TestDevice() {
        }

        void TestDevice::expectedParameters(Schema& expected) {

            INT32_ELEMENT(expected).key("position")
                    .displayedName("Current Position")
                    .description("The current position of the motor")
                    .readOnly()
                    .assignmentOptional().noDefaultValue()
                    .commit();

            INT32_ELEMENT(expected).key("velocity")
                    .displayedName("Velocity")
                    .description("The velocity the motor will move with")
                    .options("1,2,3,4,5,1000")
                    .assignmentOptional().defaultValue(4)
                    .reconfigurable()
                    .commit();
            
            PATH_ELEMENT(expected)
                    .key("filename")
                    .description("Name of the file to be read")
                    .displayedName("Filename")
                    .assignmentMandatory()
                    .commit();

            //      CHOICE_ELEMENT<exfel::net::Connection>(expected).key("device")
            //              .displayedName("Device")
            //              .description("The device the motor will move with")
            //              .assignmentMandatory()
            //              .init()
            //              .commit();
            //      
            //      INT32_ELEMENT(expected).key("pos")
            //              .displayedName("Position")
            //              .description("The position the motor will move with")
            //              .options("1,2,3,4")
            //              .assignmentOptional().defaultValue(4)
            //              .readOnly()
            //              .commit();
            //      
            //      FLOAT_ELEMENT(expected).key("floatElement")
            //              .displayedName("Float Element")
            //              .description("Test Float Element")
            //              .minInc(0)
            //              .maxInc(100.2)
            //              .unitName("millimeter")
            //              .unitSymbol("mm")
            //              .advanced()
            //              .assignmentOptional().defaultValue(4)
            //              .reconfigurable()
            //              .commit();
            //      
            //      VECTOR_INT32_ELEMENT(expected).key("vectorIntElement")
            //              .displayedName("V Element")
            //              .description("Vector Int Element for testing")
            //              .minSize(1)
            //              .maxSize(10)
            //              .assignmentOptional().defaultValueFromString("5,3,4")
            //              .reconfigurable()
            //              .commit();
            //      
            //      VECTOR_STRING_ELEMENT(expected).key("vectorStringElement")
            //              .displayedName("Vector String Element")
            //              .description("Vector String Element for testing")
            //              .description("")
            //              .assignmentOptional().defaultValueFromString("string1,string2,string3")
            //              .commit();
            //      
            //      BOOL_ELEMENT(expected).key("overlap")
            //              .alias(wstring(L"Overlap"))
            //              .displayedName("Overlap")
            //              .description("Enables overlap readout mode")
            //              .assignmentOptional().noDefaultValue()
            //              .reconfigurable()
            //              .commit();
            //      
            //      STRING_ELEMENT(expected).key("motorLeft")
            //              .displayedName("motorLeft")
            //              .description("The network-ID of the left motor")
            //              .assignmentOptional().defaultValue("left")
            //              .commit();
            //      
            //      STRING_ELEMENT(expected).key("pixelReadoutRate")
            //              .alias(wstring(L"PixelReadoutRate"))
            //              .displayedName("Pixel Readout Rate")
            //              .description("Configures the rate of pixel readout from the sensor")
            //              .assignmentOptional().noDefaultValue()
            //              .options("280 MHz,200 MHz,100 MHz", ",")
            //              .unitName("MegaHertz")
            //              .unitSymbol("MHz")
            //              .reconfigurable()
            //              .commit();
            //      
            //      DOUBLE_TARGETACTUAL_ELEMENT(expected).key("sensorTemperature")
            //              .displayedName("Sensor temperature")
            //              .unitName("degree celsius")
            //              .targetAlias(L"TargetSensorTemperature")
            //              .unitSymbol("deg")
            //              .description("Configures the temperature to which the sensor will be "
            //              "cooled. To be used for cameras with no correction data (-50 - "
            //              ">25). Otherwise TemperatureControl should be used")
            //              .targetAssignmentOptional().targetDefaultValue(0)
            //              .targetHardMin(-50)
            //              .actualWarnHigh(23)
            //              .actualAlarmHigh(40)
            //              .actualAlias(wstring(L"SensorTemperature"))
            //              .commit();
            //      
            //      LIST_ELEMENT<exfel::net::Connection> (expected).key("listElement")
            //              .displayedName("List Element")
            //              .description("List element for testing")
            //              .reconfigurable()
            //              .assignmentOptional().defaultValue("Jms")
            //              .commit();
            //      
            //      NON_EMPTY_LIST_ELEMENT<exfel::net::Connection> (expected).key("NonEmptyLListElement")
            //              .displayedName("Non Empty List Element")
            //              .description("Non Empty List element for testing")
            //              .reconfigurable()
            //              .assignmentOptional().defaultValue("Jms")
            //              .commit();
            //      
            //      
            //      SINGLE_ELEMENT<exfel::net::Connection>(expected).key("singleElement")
            //              .classId("Udp")
            //              .displayedName("Single Element")
            //              .description("Single element for testing")
            //              .reconfigurable()
            //              .assignmentOptional().defaultValue("Tcp")
            //              .commit();
        }

        void TestDevice::configure(const Hash& input) {
        }
    }
}
