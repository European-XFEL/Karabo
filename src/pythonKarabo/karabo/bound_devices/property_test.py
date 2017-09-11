#!/usr/bin/env python

__author__ = "gero.flucke@xfel.eu"
__date__ = "September, 2017, 13:45 PM"
__copyright__ = """Copyright (c) 2010-2017 European XFEL GmbH Hamburg.
All rights reserved."""

from karabo.bound import (
    KARABO_CLASSINFO, PythonDevice, Hash,
    Schema, State, OVERWRITE_ELEMENT,
    ADMIN, AlarmCondition,
    BOOL_ELEMENT, FLOAT_ELEMENT, DOUBLE_ELEMENT, INT32_ELEMENT, UINT32_ELEMENT,
    INT64_ELEMENT, UINT64_ELEMENT, NODE_ELEMENT, SLOT_ELEMENT, STRING_ELEMENT,
    TABLE_ELEMENT,
    VECTOR_BOOL_ELEMENT, VECTOR_CHAR_ELEMENT,
    VECTOR_FLOAT_ELEMENT, VECTOR_DOUBLE_ELEMENT,
    VECTOR_INT32_ELEMENT, VECTOR_UINT32_ELEMENT,
    VECTOR_INT64_ELEMENT, VECTOR_UINT64_ELEMENT,
    VECTOR_STRING_ELEMENT
)


@KARABO_CLASSINFO("PropertyTest", "2.1")
class PropertyTest(PythonDevice):

    def __init__(self, configuration):
        # always call PythonDevice constructor first!
        super(PropertyTest, self).__init__(configuration)
        # Define first function to be called after the constructor has finished
        self.registerInitialFunction(self.initialization)

    @staticmethod
    def expectedParameters(expected):
        '''Description of device parameters statically known'''
        (
            OVERWRITE_ELEMENT(expected).key("state")
            .setNewOptions(State.INIT, State.NORMAL, State.ERROR)
            .setNewDefaultValue(State.INIT)
            .commit(),

            OVERWRITE_ELEMENT(expected).key("visibility")
            .setNewDefaultValue(ADMIN)
            .commit(),

            SLOT_ELEMENT(expected).key("setReadonly")
            .displayedName("Set read only values")
            .description("Set all readonly properties to the corresponding "
                         "reconfigurable ones")
            .commit(),

            BOOL_ELEMENT(expected).key("boolProperty")
            .displayedName("Bool")
            .description("A bool property")
            .reconfigurable()
            .assignmentOptional().defaultValue(False)
            .commit(),

            BOOL_ELEMENT(expected).key("boolPropertyReadonly")
            .displayedName("Readonly Bool")
            .description("A bool property for testing alarms")
            .readOnly().initialValue(True)
            .warnLow(True).info("Rather low").needsAcknowledging(False)
            .commit(),

            # CHAR_ELEMENT, (U)INT8_ELEMENT, (U)INT16_ELEMENT do not exists

            INT32_ELEMENT(expected).key("int32Property")
            .displayedName("Int32")
            .description("An int32 property")
            .reconfigurable()
            .assignmentOptional().defaultValue(32000000)
            .commit(),

            INT32_ELEMENT(expected).key("int32PropertyReadonly")
            .displayedName("Readonly Int32")
            .description("An int32 property for testing alarms")
            .readOnly().initialValue(32000000)
            .warnLow(0).info("Rather low").needsAcknowledging(False)
            .alarmLow(-32000000).info("Too low").needsAcknowledging(True)
            .commit(),

            UINT32_ELEMENT(expected).key("uint32Property")
            .displayedName("UInt32")
            .description("A uint32 property")
            .reconfigurable()
            .assignmentOptional().defaultValue(32000000)
            .commit(),

            UINT32_ELEMENT(expected).key("uint32PropertyReadonly")
            .displayedName("Readonly UInt32")
            .description("A uint32 property for testing alarms")
            .readOnly().initialValue(32000000)
            .warnHigh(32000001).info("Rather high").needsAcknowledging(False)
            .alarmHigh(64000000).info("Too high").needsAcknowledging(True)
            .commit(),

            INT64_ELEMENT(expected).key("int64Property")
            .displayedName("Int64")
            .description("An int64 property")
            .reconfigurable()
            .assignmentOptional().defaultValue(3200000000)
            .commit(),

            INT64_ELEMENT(expected).key("int64PropertyReadonly")
            .displayedName("Readonly Int64")
            .description("An int64 property for testing alarms")
            .readOnly().initialValue(3200000000)
            .warnLow(0).info("Rather high").needsAcknowledging(False)
            .alarmLow(-3200000000).info("Too high").needsAcknowledging(True)
            .commit(),

            UINT64_ELEMENT(expected).key("uint64Property")
            .displayedName("UInt64 property")
            .description("A uint64 property")
            .reconfigurable()
            .assignmentOptional().defaultValue(3200000000)
            .commit(),

            UINT64_ELEMENT(expected).key("uint64PropertyReadonly")
            .displayedName("Readonly UInt64")
            .description("A uint64 property for testing alarms")
            .readOnly().initialValue(3200000000)
            .warnHigh(3200000001).info("Rather high").needsAcknowledging(False)
            .alarmHigh(6400000000).info("Too high").needsAcknowledging(True)
            .commit(),

            FLOAT_ELEMENT(expected).key("floatProperty")
            .displayedName("Float property")
            .description("A float property")
            .reconfigurable()
            .assignmentOptional().defaultValue(3.141596)
            .commit(),

            FLOAT_ELEMENT(expected).key("floatPropertyReadonly")
            .displayedName("Readonly Float")
            .description("A float property for testing alarms")
            .readOnly().initialValue(3.141596)
            .alarmLow(-(3.141596**2)).info("Too high").needsAcknowledging(True)
            .warnLow(-2*3.141596).info("Rather high").needsAcknowledging(False)
            .warnHigh(2*3.141596).info("Rather high").needsAcknowledging(False)
            .alarmHigh(3.141596**2).info("Too high").needsAcknowledging(True)
            .commit(),

            DOUBLE_ELEMENT(expected).key("doubleProperty")
            .displayedName("Double")
            .description("A double property")
            .reconfigurable()
            .assignmentOptional().defaultValue(0.)
            .commit(),

            DOUBLE_ELEMENT(expected).key("doublePropertyReadonly")
            .displayedName("Readonly Double")
            .description("A double property for testing alarms")
            .readOnly().initialValue(0.)
            .alarmLow(-100.).info("Too high").needsAcknowledging(True)
            .warnLow(-10.).info("Rather high").needsAcknowledging(False)
            .warnHigh(10.).info("Rather high").needsAcknowledging(False)
            .alarmHigh(100.).info("Too high").needsAcknowledging(True)
            .commit(),

            STRING_ELEMENT(expected).key("stringProperty")
            .displayedName("String")
            .description("A string property")
            .reconfigurable()
            .assignmentOptional().defaultValue("Some arbitrary text.")
            .commit(),

            SLOT_ELEMENT(expected).key("setAlarm")
            .displayedName("Set Alarm")
            .description("Set alarm to value of String - if convertable")
            .commit(),

            NODE_ELEMENT(expected).key("vectors")
            .displayedName("Vectors")
            .description("A node containing vector properties")
            .commit(),

            VECTOR_BOOL_ELEMENT(expected).key("vectors.boolProperty")
            .displayedName("Bool property")
            .description("A vector boolean property")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue([True, False, True,
                                                False, True, False])
            .commit(),

            VECTOR_CHAR_ELEMENT(expected).key("vectors.charProperty")
            .displayedName("Char property")
            .description("A vector character property")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue(['A', 'B', 'C', 'D', 'E', 'F'])
            .commit(),

            VECTOR_INT32_ELEMENT(expected).key("vectors.int32Property")
            .displayedName("Int32 property")
            .description("A vector int32 property")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue([20000041, 20000042, 20000043,
                                                20000044, 20000045, 20000046])
            .commit(),

            VECTOR_UINT32_ELEMENT(expected).key("vectors.uint32Property")
            .displayedName("UInt32 property")
            .description("A vector uint32 property")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue([90000041, 90000042, 90000043,
                                                90000044, 90000045, 90000046])
            .commit(),

            VECTOR_INT64_ELEMENT(expected).key("vectors.int64Property")
            .displayedName("Int64 property")
            .description("A vector int64 property")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue([20000000041, 20000000042,
                                                20000000043, 20000000044,
                                                20000000045, 20000000046])
            .commit(),

            VECTOR_UINT64_ELEMENT(expected).key("vectors.uint64Property")
            .displayedName("UInt64 property")
            .description("A vector uint64 property")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue([90000000041, 90000000042,
                                                90000000043, 90000000044,
                                                90000000045, 90000000046])
            .commit(),

            VECTOR_FLOAT_ELEMENT(expected).key("vectors.floatProperty")
            .displayedName("Float property")
            .description("A vector float property")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue([1.23456, 2.34567, 3.45678,
                                                4.56789, 5.67891, 6.78912])
            .commit(),

            VECTOR_DOUBLE_ELEMENT(expected).key("vectors.doubleProperty")
            .displayedName("Double property")
            .description("A vector double property")
            .reconfigurable()
            .minSize(1)
            .maxSize(10)
            .assignmentOptional().defaultValue([1.234567891, 2.345678912,
                                                3.456789123, 4.567891234,
                                                5.678901234, 6.123456789])
            .commit(),

            VECTOR_STRING_ELEMENT(expected).key("vectors.stringProperty")
            .displayedName("String property")
            .description("A vector string property")
            .reconfigurable()
            .minSize(1).maxSize(10)
            .assignmentOptional().defaultValue(["AAAAA", "BBBBB", "CCCCC",
                                                "XXXXX", "YYYYY", "ZZZZZ"])
            .commit(),
        )

        # Now prepare table
        columns = Schema()
        (
            STRING_ELEMENT(columns).key("e1")
            .displayedName("E1")
            .description("E1 property")
            .assignmentOptional().defaultValue("E1")
            .reconfigurable()
            .commit(),

            BOOL_ELEMENT(columns).key("e2")
            .displayedName("E2")
            .description("E2 property")
            .reconfigurable()
            .assignmentOptional().defaultValue(False)
            .commit(),

            INT32_ELEMENT(columns).key("e3")
            .displayedName("E3")
            .description("E3 property")
            .reconfigurable()
            .assignmentOptional().defaultValue(42)
            .commit(),

            FLOAT_ELEMENT(columns).key("e4")
            .displayedName("E4")
            .description("E4 property")
            .assignmentOptional().defaultValue(-3.14)
            .reconfigurable()
            .commit(),

            DOUBLE_ELEMENT(columns).key("e5")
            .displayedName("E5")
            .description("E5 property")
            .assignmentOptional().defaultValue(3.14)
            .reconfigurable()
            .commit(),

            TABLE_ELEMENT(expected).key("table")
            .displayedName("Table property")
            .description("Table containing one node.")
            .setColumns(columns)
            .assignmentOptional().defaultValue([Hash("e1", "abc", "e2", True,
                                                     "e3", 12, "e4", 0.9837,
                                                     "e5", 1.2345),
                                                Hash("e1", "xyz", "e2", False,
                                                     "e3", 42, "e4", 2.33333,
                                                     "e5", 7.77777)])
            .reconfigurable()
            .commit(),
        )

    def initialization(self):
        # Define the slots
        self.KARABO_SLOT(self.setReadonly)
        self.KARABO_SLOT(self.setAlarm)

        self.updateState(State.NORMAL)

    def setReadonly(self):
        props = ["int32Property", "uint32Property",
                 "int64Property", "uint64Property",
                 "floatProperty", "doubleProperty"]

        bulkSets = Hash()
        for prop in props:
            readOnlyProp = prop + "Readonly"
            if self[readOnlyProp] != self[prop]:
                bulkSets[readOnlyProp] = self[prop]
        if bulkSets:
            self.set(bulkSets)

    def setAlarm(self):
        try:
            alarm = AlarmCondition.fromString(self["stringProperty"])
        except ValueError as e:
            self.log.WARN("'{}' is not a valid alarm condition - please adjust"
                          " 'stringProperty'".format(self["stringProperty"]))
        else:
            self.setAlarmCondition(alarm,
                                   description="Converted from stringProperty")
