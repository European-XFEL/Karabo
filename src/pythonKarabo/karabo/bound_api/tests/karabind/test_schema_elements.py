# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import karabind
import pytest

import karathon
from karabo.bound_api.decorators import (
    KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS)
from karabo.common.alarm_conditions import AlarmCondition
from karabo.common.states import State


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("ShapeX", "1.0")
class ShapeX:
    @staticmethod
    def expectedParameters(expected):
        if isinstance(expected, karabind.Schema):
            BOOL_ELEMENT = karabind.BOOL_ELEMENT
        elif isinstance(expected, karathon.Schema):
            BOOL_ELEMENT = karathon.BOOL_ELEMENT
        else:
            raise TypeError("Unsupported argument type")
        (
            BOOL_ELEMENT(expected)
            .key("shadowEnabled")
            .description("Shadow enabled")
            .displayedName("Shadow")
            .assignmentOptional()
            .defaultValue(False)
            .init()
            .commit(),
        )


@KARABO_CLASSINFO("CircleX", "1.0")
class CircleX(ShapeX):
    @staticmethod
    def expectedParameters(expected):
        if isinstance(expected, karabind.Schema):
            FLOAT_ELEMENT = karabind.FLOAT_ELEMENT
            STATE_ELEMENT = karabind.STATE_ELEMENT
            ALARM_ELEMENT = karabind.ALARM_ELEMENT
            STRING_ELEMENT = karabind.STRING_ELEMENT
            Unit = karabind.Unit
            MetricPrefix = karabind.MetricPrefix
        elif isinstance(expected, karathon.Schema):
            FLOAT_ELEMENT = karathon.FLOAT_ELEMENT
            STATE_ELEMENT = karathon.STATE_ELEMENT
            ALARM_ELEMENT = karathon.ALARM_ELEMENT
            STRING_ELEMENT = karathon.STRING_ELEMENT
            Unit = karathon.Unit
            MetricPrefix = karathon.MetricPrefix
        else:
            raise TypeError("Unsupported argument type")
        (
            FLOAT_ELEMENT(expected)
            .key("radius")
            .alias(1)
            .description("The radius of the circle")
            .displayedName("Radius")
            .minExc(0)
            .maxExc(100)
            .unit(Unit.METER)
            .metricPrefix(MetricPrefix.MILLI)
            .assignmentOptional()
            .defaultValue(10)
            .init()
            .commit(),

            STATE_ELEMENT(expected).key("state")
            .commit(),

            STATE_ELEMENT(expected).key("stateN")
            .initialValue(State.NORMAL)
            .commit(),

            STATE_ELEMENT(expected).key("stateE")
            .initialValue(State.ERROR)
            .commit(),

            ALARM_ELEMENT(expected).key("alarm")
            .commit(),

            ALARM_ELEMENT(expected).key("alarmW")
            .initialValue(AlarmCondition.WARN)
            .commit(),

            ALARM_ELEMENT(expected).key("alarmA")
            .initialValue(AlarmCondition.ALARM)
            .commit(),

            STRING_ELEMENT(expected).key("status")
            .readOnly()
            .commit(),
        )


@KARABO_CLASSINFO("EditableCircleX", "1.0")
class EditableCircleX(CircleX):
    @staticmethod
    def expectedParameters(expected):
        CircleX.expectedParameters(expected)
        if isinstance(expected, karabind.Schema):
            OVERWRITE_ELEMENT = karabind.OVERWRITE_ELEMENT
            Unit = karabind.Unit
            MetricPrefix = karabind.MetricPrefix
        elif isinstance(expected, karathon.Schema):
            OVERWRITE_ELEMENT = karathon.OVERWRITE_ELEMENT
            Unit = karathon.Unit
            MetricPrefix = karathon.MetricPrefix
        else:
            raise TypeError("Unsupported argument type")
        (
            OVERWRITE_ELEMENT(expected).key("radius")
            .setNewDisplayedName("New Radius")
            .setNewAlias(21)
            .setNewTags("operation,control")
            .setNewAssignmentMandatory()
            .setNowReconfigurable()
            .setNewUnit(Unit.METER)
            .setNewMetricPrefix(MetricPrefix.KILO)
            .setNewMinInc(12)
            .setNewMaxExc(120)
            .setNowValidate()
            .setNewAllowedStates(State.NORMAL)
            .setNowExpertAccess()
            .commit(),

            OVERWRITE_ELEMENT(expected)
            .key("state")
            .setNewOptions(State.INIT, State.ERROR, State.NORMAL)
            .setNewDefaultValue(State.INIT)
            .commit(),

            OVERWRITE_ELEMENT(expected).key("status")
            .setNewOptions("a,b,c")
            .setNewDefaultValue("a")
            .commit(),

            OVERWRITE_ELEMENT(expected).key("alarm")
            .setNewDescription("The new alarm description")
            .setNewAssignmentOptional()
            .setNewDefaultValue(AlarmCondition.WARN_LOW)
            .setNowUserAccess()
            .commit(),

            OVERWRITE_ELEMENT(expected).key("alarmA")
            .setNowObserverAccess()
            .setNowSkipValidation()
            .commit(),

        )


@KARABO_CLASSINFO("RectangleX", "1.0")
class RectangleX(ShapeX):
    @staticmethod
    def expectedParameters(expected):
        if isinstance(expected, karabind.Schema):
            FLOAT_ELEMENT = karabind.FLOAT_ELEMENT
            Unit = karabind.Unit
            MetricPrefix = karabind.MetricPrefix
        elif isinstance(expected, karathon.Schema):
            FLOAT_ELEMENT = karathon.FLOAT_ELEMENT
            Unit = karathon.Unit
            MetricPrefix = karathon.MetricPrefix
        else:
            raise TypeError("Unsupported argument type")
        (
            FLOAT_ELEMENT(expected)
            .key("a")
            .alias(1)
            .description("Length of a")
            .displayedName("A")
            .minExc(0)
            .maxExc(100)
            .unit(Unit.METER)
            .metricPrefix(MetricPrefix.MILLI)
            .assignmentOptional()
            .defaultValue(10)
            .adminAccess()
            .init()
            .commit(),

            FLOAT_ELEMENT(expected)
            .key("b")
            .alias(1)
            .description("Length of b")
            .displayedName("B")
            .minExc(0)
            .maxExc(100)
            .unit(Unit.METER)
            .metricPrefix(MetricPrefix.MILLI)
            .assignmentOptional()
            .defaultValue(10)
            .init()
            .commit(),
        )


@KARABO_CLASSINFO("EditableRectangleX", "1.0")
class EditableRectangleX(RectangleX):
    @staticmethod
    def expectedParameters(expected):
        RectangleX.expectedParameters(expected)
        if isinstance(expected, karabind.Schema):
            OVERWRITE_ELEMENT = karabind.OVERWRITE_ELEMENT
        elif isinstance(expected, karathon.Schema):
            OVERWRITE_ELEMENT = karathon.OVERWRITE_ELEMENT
        else:
            raise TypeError("Unsupported argument type")
        (
            OVERWRITE_ELEMENT(expected).key("a")
            .setNewAssignmentInternal()
            .setNewMinExc(30)
            .setNewMaxInc(60)
            .setNowAdminAccess()
            .commit(),

            OVERWRITE_ELEMENT(expected).key("b")
            .setNowReadOnly().setNewDefaultValue(12)
            .setNowOperatorAccess()
            .commit(),

        )


class GraphicsRendererX1:
    @staticmethod
    def expectedParameters(expected):
        if isinstance(expected, karabind.Schema):
            # Schema = karabind.Schema
            BOOL_ELEMENT = karabind.BOOL_ELEMENT
            STRING_ELEMENT = karabind.STRING_ELEMENT
            CHOICE_ELEMENT = karabind.CHOICE_ELEMENT
            NODE_ELEMENT = karabind.NODE_ELEMENT
            FLOAT_ELEMENT = karabind.FLOAT_ELEMENT
        elif isinstance(expected, karathon.Schema):
            # Schema = karathon.Schema
            BOOL_ELEMENT = karathon.BOOL_ELEMENT
            STRING_ELEMENT = karathon.STRING_ELEMENT
            CHOICE_ELEMENT = karathon.CHOICE_ELEMENT
            NODE_ELEMENT = karathon.NODE_ELEMENT
            FLOAT_ELEMENT = karathon.FLOAT_ELEMENT
        else:
            raise TypeError("Unsupported argument type")

        (
            BOOL_ELEMENT(expected)
            .key("antiAlias")
            .tags("prop")
            .displayedName("Use Anti-Aliasing")
            .description("You may switch of for speed")
            .assignmentOptional()
            .defaultValue(True)
            .init()
            .expertAccess()
            .commit(),

            STRING_ELEMENT(expected)
            .key("color")
            .tags("prop")
            .displayedName("Color")
            .description("The default color for any shape")
            .assignmentOptional()
            .defaultValue("red")
            .reconfigurable()
            .allowedStates(State.OFF)
            .commit(),

            BOOL_ELEMENT(expected)
            .key("bold")
            .tags("prop")
            .displayedName("Bold")
            .description("Toggles bold painting")
            .assignmentOptional()
            .defaultValue(False)
            .reconfigurable()
            .commit(),

            CHOICE_ELEMENT(expected)
            .key("shapes")
            .assignmentOptional()
            .defaultValue("circle")
            .commit(),

            NODE_ELEMENT(expected)
            .key("shapes.circle")
            .tags("shape")
            .displayedName("Circle")
            .description("A circle")
            .appendParametersOf(CircleX)
            .commit(),

            NODE_ELEMENT(expected)
            .key("shapes.rectangle")
            .tags("shape")
            .displayedName("Rectangle")
            .description("A rectangle")
            .commit(),

            FLOAT_ELEMENT(expected)
            .key("shapes.rectangle.b")
            .description("Rectangle side - b")
            .displayedName("Side B")
            .tags("b")
            .assignmentOptional()
            .defaultValue(10)
            .init()
            .commit(),

            FLOAT_ELEMENT(expected)
            .key("shapes.rectangle.c")
            .description("Rectangle side - c")
            .displayedName("Side C")
            .assignmentOptional()
            .defaultValue(10)
            .init()
            .commit(),

            NODE_ELEMENT(expected)
            .key("triangle")
            .displayedName("triangle")
            .description("A triangle (Node element containing no other "
                         "elements)")
            .commit(),
        )


class TestStruct1:
    @staticmethod
    def expectedParameters(expected):
        if isinstance(expected, karabind.Schema):
            Schema = karabind.Schema
            Hash = karabind.Hash
            STRING_ELEMENT = karabind.STRING_ELEMENT
            INT32_ELEMENT = karabind.INT32_ELEMENT
            UINT32_ELEMENT = karabind.UINT32_ELEMENT
            DOUBLE_ELEMENT = karabind.DOUBLE_ELEMENT
            INT64_ELEMENT = karabind.INT64_ELEMENT
            UINT64_ELEMENT = karabind.UINT64_ELEMENT
            VECTOR_INT32_ELEMENT = karabind.VECTOR_INT32_ELEMENT
            VECTOR_UINT32_ELEMENT = karabind.VECTOR_UINT32_ELEMENT
            # VECTOR_INT64_ELEMENT = karabind.VECTOR_INT64_ELEMENT
            # VECTOR_UINT64_ELEMENT = karabind.VECTOR_UINT64_ELEMENT
            VECTOR_DOUBLE_ELEMENT = karabind.VECTOR_DOUBLE_ELEMENT
            VECTOR_STRING_ELEMENT = karabind.VECTOR_STRING_ELEMENT
            PATH_ELEMENT = karabind.PATH_ELEMENT
            NDARRAY_ELEMENT = karabind.NDARRAY_ELEMENT
            # SLOT_ELEMENT = karabind.SLOT_ELEMENT
            NODE_ELEMENT = karabind.NODE_ELEMENT
            TABLE_ELEMENT = karabind.TABLE_ELEMENT
            CHOICE_ELEMENT = karabind.CHOICE_ELEMENT
            Unit = karabind.Unit
            MetricPrefix = karabind.MetricPrefix
            EVERY_100MS = karabind.EVERY_100MS
            EVERY_1S = karabind.EVERY_1S
            NO_ARCHIVING = karabind.NO_ARCHIVING
        elif isinstance(expected, karathon.Schema):
            Schema = karathon.Schema
            Hash = karathon.Hash
            STRING_ELEMENT = karathon.STRING_ELEMENT
            INT32_ELEMENT = karathon.INT32_ELEMENT
            UINT32_ELEMENT = karathon.UINT32_ELEMENT
            DOUBLE_ELEMENT = karathon.DOUBLE_ELEMENT
            INT64_ELEMENT = karathon.INT64_ELEMENT
            UINT64_ELEMENT = karathon.UINT64_ELEMENT
            VECTOR_INT32_ELEMENT = karathon.VECTOR_INT32_ELEMENT
            VECTOR_UINT32_ELEMENT = karathon.VECTOR_UINT32_ELEMENT
            # VECTOR_INT64_ELEMENT = karathon.VECTOR_INT64_ELEMENT
            # VECTOR_UINT64_ELEMENT = karathon.VECTOR_UINT64_ELEMENT
            VECTOR_DOUBLE_ELEMENT = karathon.VECTOR_DOUBLE_ELEMENT
            VECTOR_STRING_ELEMENT = karathon.VECTOR_STRING_ELEMENT
            PATH_ELEMENT = karathon.PATH_ELEMENT
            NDARRAY_ELEMENT = karathon.NDARRAY_ELEMENT
            # SLOT_ELEMENT = karathon.SLOT_ELEMENT
            NODE_ELEMENT = karathon.NODE_ELEMENT
            TABLE_ELEMENT = karathon.TABLE_ELEMENT
            CHOICE_ELEMENT = karathon.CHOICE_ELEMENT
            Unit = karathon.Unit
            MetricPrefix = karathon.MetricPrefix
            EVERY_100MS = karathon.EVERY_100MS
            EVERY_1S = karathon.EVERY_1S
            NO_ARCHIVING = karathon.NO_ARCHIVING
        else:
            raise TypeError("Unsupported argument type")
        (
            STRING_ELEMENT(expected)
            .key("exampleKey1")
            .tags("hardware, poll")
            .displayedName("Example key 1")
            .description("Example key 1 description")
            .options("Radio,Air Condition,Navigation", ",")
            .assignmentOptional()
            .defaultValue("Navigation")
            .userAccess()
            .reconfigurable()
            .commit(),

            INT32_ELEMENT(expected)
            .key("exampleKey2")
            .alias(10)
            .tags("hardware, poll")
            .displayedName("Example key 2")
            .description("Example key 2 description")
            .options("5, 25, 10")
            .minInc(5)
            .maxInc(25)
            .unit(Unit.METER)
            .metricPrefix(MetricPrefix.MILLI)
            .assignmentOptional()
            .defaultValue(10)
            .operatorAccess()
            .init()
            .commit(),

            UINT32_ELEMENT(expected)
            .key("exampleKey3")
            .alias(5.5)
            .tags("hardware, set")
            .displayedName("Example key 3")
            .description("Example key 3 description")
            .allowedStates(State.STARTED, State.STOPPED, State.NORMAL)
            .minExc(10)
            .maxExc(20)
            .assignmentMandatory()
            .expertAccess()
            .reconfigurable()
            .commit(),

            DOUBLE_ELEMENT(expected)
            .key("exampleKey4")
            .alias("exampleAlias4")
            .tags("software")
            .displayedName("Example key 4")
            .description("Example key 4 description")
            .options("1.11     -2.22 5.55")
            .adminAccess()
            .minExc(-2.22)
            .maxExc(5.55)
            .assignmentInternal()
            .noDefaultValue()
            .commit(),

            INT64_ELEMENT(expected)
            .key("exampleKey5")
            .alias("exampleAlias5")
            .tags("h/w; d.m.y", ";")
            .displayedName("Example key 5")
            .description("Example key 5 description")
            .setSpecialDisplayType("Int64DisplayType")
            .readOnly()
            .initialValue(1442244)
            .warnLow(-10)
            .needsAcknowledging(False)
            .warnHigh(10)
            .needsAcknowledging(False)
            .alarmLow(-20)
            .needsAcknowledging(False)
            .alarmHigh(20)
            .needsAcknowledging(False)
            .commit(),

            UINT64_ELEMENT(expected)
            .key("exampleKey5b")
            .readOnly()
            .defaultValue(42)
            .commit(),

            DOUBLE_ELEMENT(expected)
            .key("exampleKey6")
            .alias([0x00123456, 0x0000aacc])
            .displayedName("Example key 6")
            .description("Example key 6 description")
            .readOnly()
            .initialValue(1.11)
            .alarmLow(-22.1)
            .needsAcknowledging(True)
            .alarmHigh(22.777)
            .needsAcknowledging(True)
            .warnLow(-5.5)
            .info("Some info")
            .needsAcknowledging(True)
            .warnHigh(5.5)
            .needsAcknowledging(True)
            .archivePolicy(EVERY_100MS)
            .commit(),

            VECTOR_INT32_ELEMENT(expected)
            .key("exampleKey7")
            .displayedName("Example key 7")
            .allowedStates(State.STARTED, State.NORMAL)
            .readOnly()
            .initialValue([1, 2, 3])
            .archivePolicy(EVERY_1S)
            .commit(),

            VECTOR_UINT32_ELEMENT(expected)
            .key("exampleKey7b")
            .readOnly()
            .initialValue([11, 22, 33])
            .archivePolicy(EVERY_1S)
            .commit(),

            VECTOR_DOUBLE_ELEMENT(expected)
            .key("exampleKey8")
            .readOnly()
            .initialValue([1.1, 2.2, 3.3])
            .archivePolicy(NO_ARCHIVING)
            .commit(),

            VECTOR_STRING_ELEMENT(expected)
            .key("exampleKey9")
            .setSpecialDisplayType("TestDisplayType")
            .readOnly()
            .initialValue(["Hallo", "World"])
            .archivePolicy(EVERY_1S)
            .commit(),

            VECTOR_INT32_ELEMENT(expected)
            .key("vectInt")
            .readOnly()
            .archivePolicy(EVERY_1S)
            .commit(),

            VECTOR_INT32_ELEMENT(expected)
            .key("exampleKey10")
            .displayedName("Example key 10")
            .minSize(2)
            .maxSize(7)
            .assignmentOptional()
            .defaultValueFromString("10, 20, 30")
            .reconfigurable()
            .commit(),

            VECTOR_INT32_ELEMENT(expected)
            .key("exampleKey11")
            .displayedName("Example key 11")
            .assignmentOptional()
            .defaultValue([10, 20, 30])
            .observerAccess()
            .reconfigurable()
            .commit(),

            VECTOR_DOUBLE_ELEMENT(expected)
            .key("exampleKey12")
            .assignmentOptional()
            .defaultValueFromString("1.1, -2.2, 3.3")
            .reconfigurable()
            .commit(),

            VECTOR_STRING_ELEMENT(expected)
            .key("exampleKey14")
            .assignmentOptional()
            .defaultValue(["Hallo", "World", "Test"])
            .reconfigurable()
            .commit(),

            VECTOR_STRING_ELEMENT(expected)
            .key("exampleKey15")
            .assignmentOptional()
            .defaultValueFromString("word1, word2, test")
            .reconfigurable()
            .commit(),

            NDARRAY_ELEMENT(expected).key("exampleKey16")
            .dtype("UINT32")
            .shape([2, 5, 0])
            .commit(),

            INT32_ELEMENT(expected)
            .key("exampleIntKey")
            .assignmentOptional()
            .defaultValueFromString("20")
            .reconfigurable()
            .commit(),

            UINT32_ELEMENT(expected)
            .key("exampleBitsKey1")
            .tags("hardware")
            .displayedName("Example bits key 1")
            .description("Example bits key 1 description")
            .reconfigurable()
            .bin()
            .assignmentOptional()
            .defaultValue(0xdeadbeef)
            .commit(),

            UINT64_ELEMENT(expected)
            .key("exampleBitsKey2")
            .tags("hardware")
            .displayedName("Example bits key 2")
            .description("Example bits key 2 description")
            .reconfigurable()
            .bin("10:In Error, 21:Busy, 35:HV On, 55:Crate On")
            .assignmentOptional()
            .defaultValue(0xdeadbeefdeadface)
            .commit(),

            UINT32_ELEMENT(expected)
            .key("exampleBitsKey3")
            .tags("hardware")
            .displayedName("Example bits key 3")
            .description("Example bits key 3 description")
            .reconfigurable()
            .oct()
            .assignmentOptional()
            .defaultValue(0xbeefface)
            .commit(),

            UINT32_ELEMENT(expected)
            .key("exampleBitsKey4")
            .tags("hardware")
            .displayedName("Example bits key 4")
            .description("Example bits key 4 description")
            .reconfigurable()
            .hex()
            .assignmentOptional()
            .defaultValue(0xbeefface)
            .commit(),

            PATH_ELEMENT(expected)
            .key("testPath")
            .alias(5)
            .displayedName("Filename")
            .isOutputFile()
            .options("file1, file2")
            .assignmentOptional()
            .defaultValue("karabo.log")
            .reconfigurable()
            .commit(),

            PATH_ELEMENT(expected)
            .key("testPath2")
            .isInputFile()
            .readOnly()
            .archivePolicy(EVERY_1S)
            .commit(),

            PATH_ELEMENT(expected)
            .key("testPath3")
            .isDirectory()
            .assignmentMandatory()
            .commit(),

            # SLOT_ELEMENT(expected)
            # .key("slotTest")
            # .displayedName("Reset")
            # .description("Test slot element")
            # .allowedStates(State.STARTED, State.STOPPED, State.NORMAL)
            # .commit(),

            CHOICE_ELEMENT(expected)
            .key("testChoice1")
            .displayedName("1. Select CircleX or RectangleX")
            .appendAsNode(CircleX, "CircleX")
            .appendAsNode(RectangleX, "RectangleX")
            .assignmentOptional().defaultValueFromString("RectangleX")
            .reconfigurable()
            .commit(),

            CHOICE_ELEMENT(expected)
            .key("testChoice2")
            .displayedName("2. Choose from menu")
            .appendNodesOfConfigurationBase(ShapeX)
            .assignmentOptional().noDefaultValue()
            .reconfigurable()
            .commit(),

            CHOICE_ELEMENT(expected)
            .key("testChoice3")
            .displayedName("3. Choose from menu")
            .appendNodesOfConfigurationBase(ShapeX)
            .assignmentMandatory()
            .init()
            .commit(),

            CHOICE_ELEMENT(expected)
            .key("testChoice4")
            .displayedName("4. 3Choose from menu")
            .appendNodesOfConfigurationBase(ShapeX)
            .assignmentOptional().defaultValue("CircleX")
            .reconfigurable()
            .commit(),
        )

        row = Schema()
        (
            INT32_ELEMENT(row)
            .key("int")
            .readOnly()
            .initialValue(0)
            .commit(),

            TABLE_ELEMENT(expected)
            .key("tableI")
            .setColumns(row)
            .readOnly()
            .initialValue([Hash("int", 2)])
            .archivePolicy(EVERY_1S)
            .commit(),

            TABLE_ELEMENT(expected)
            .key("tableD")
            .setColumns(row)
            .readOnly()
            .defaultValue([Hash("int", 3)])
            .archivePolicy(EVERY_1S)
            .commit(),
        )

        (
            NODE_ELEMENT(expected)
            .key("myNode")
            .description("WidgetNode Test")
            .setSpecialDisplayType("WidgetNode")
            .commit(),
        )


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("OtherSchemaElementsX", "1.0")
class OtherSchemaElementsX:
    @staticmethod
    def expectedParameters(expected):
        if isinstance(expected, karabind.Schema):
            Hash = karabind.Hash
            Schema = karabind.Schema
            FLOAT_ELEMENT = karabind.FLOAT_ELEMENT
            # SLOT_ELEMENT = karabind.SLOT_ELEMENT
            PATH_ELEMENT = karabind.PATH_ELEMENT
            STRING_ELEMENT = karabind.STRING_ELEMENT
            VECTOR_INT32_ELEMENT = karabind.VECTOR_INT32_ELEMENT
            VECTOR_DOUBLE_ELEMENT = karabind.VECTOR_DOUBLE_ELEMENT
            VECTOR_BOOL_ELEMENT = karabind.VECTOR_BOOL_ELEMENT
            NDARRAY_ELEMENT = karabind.NDARRAY_ELEMENT
            Types = karabind.Types
            LIST_ELEMENT = karabind.LIST_ELEMENT
            NODE_ELEMENT = karabind.NODE_ELEMENT
            Types = karabind.Types
            TABLE_ELEMENT = karabind.TABLE_ELEMENT
            INT32_ELEMENT = karabind.INT32_ELEMENT
            IMAGEDATA_ELEMENT = karabind.IMAGEDATA_ELEMENT
            # EVERY_10MIN = karabind.EVERY_10MIN
            EVERY_EVENT = karabind.EVERY_EVENT
            # NO_ARCHIVING = karabind.NO_ARCHIVING
        elif isinstance(expected, karathon.Schema):
            Hash = karathon.Hash
            Schema = karathon.Schema
            FLOAT_ELEMENT = karathon.FLOAT_ELEMENT
            # SLOT_ELEMENT = karathon.SLOT_ELEMENT
            PATH_ELEMENT = karathon.PATH_ELEMENT
            STRING_ELEMENT = karathon.STRING_ELEMENT
            VECTOR_INT32_ELEMENT = karathon.VECTOR_INT32_ELEMENT
            VECTOR_DOUBLE_ELEMENT = karathon.VECTOR_DOUBLE_ELEMENT
            VECTOR_BOOL_ELEMENT = karathon.VECTOR_BOOL_ELEMENT
            NDARRAY_ELEMENT = karathon.NDARRAY_ELEMENT
            Types = karathon.Types
            LIST_ELEMENT = karathon.LIST_ELEMENT
            NODE_ELEMENT = karathon.NODE_ELEMENT
            Types = karathon.Types
            TABLE_ELEMENT = karathon.TABLE_ELEMENT
            INT32_ELEMENT = karathon.INT32_ELEMENT
            IMAGEDATA_ELEMENT = karathon.IMAGEDATA_ELEMENT
            # EVERY_10MIN = karathon.EVERY_10MIN
            EVERY_EVENT = karathon.EVERY_EVENT
            # NO_ARCHIVING = karathon.NO_ARCHIVING
        else:
            raise TypeError("Unsupported argument type")
        (
            # SLOT_ELEMENT(expected)
            # .key("slotTest")
            # .displayedName("Reset")
            # .description("Test slot element")
            # .allowedStates(State.STARTED, State.STOPPED, State.ERROR)
            # .commit(),

            PATH_ELEMENT(expected)
            .description("File name")
            .key("filename")
            .alias(5)
            .displayedName("Filename")
            .isOutputFile()
            .options("file1, file2")
            .assignmentOptional()
            .defaultValue("karabo.log")
            .reconfigurable()
            .commit(),

            STRING_ELEMENT(expected)
            .key("testfile")
            # .isInputFile()
            .readOnly()
            .defaultValue("initFile")  # == initialValue
            .alarmHigh("a")
            .needsAcknowledging(False)
            # .alarmLow("b")
            # .needsAcknowledging(False)
            # .warnHigh("c")
            # .needsAcknowledging(False)
            # .warnLow("d")
            # .needsAcknowledging(False)
            # .archivePolicy(EVERY_10MIN)
            .commit(),

            # VECTOR_INT32_ELEMENT(expected)
            # .key("vecInt")
            # .readOnly()
            # .initialValue([10, 20, 30])
            # .warnLow([50, 50, 50])
            # .needsAcknowledging(False)
            # .warnHigh([100, 100, 100])
            # .needsAcknowledging(False)
            # .archivePolicy(EVERY_EVENT)
            # .commit(),

            # VECTOR_DOUBLE_ELEMENT(expected)
            # .key("vecDouble")
            # .readOnly()
            # .alarmLow(-5.5)
            # .needsAcknowledging(False)
            # .alarmHigh([7.7, 7.7, 7.7])
            # .needsAcknowledging(False)
            # .archivePolicy(NO_ARCHIVING)
            # .commit(),

            VECTOR_INT32_ELEMENT(expected)
            .key("vecIntReconfig")
            .assignmentOptional()
            .defaultValue([10, 20, 30])
            .reconfigurable()
            .commit(),

            VECTOR_INT32_ELEMENT(expected)
            .key("vecIntReconfigStr")
            .assignmentOptional()
            .defaultValueFromString("11, 22, 33")
            .reconfigurable()
            .commit(),

            VECTOR_DOUBLE_ELEMENT(expected)
            .key("vecDoubleReconfigStr")
            .assignmentOptional()
            .defaultValueFromString("1.1, 2.2, 3.3")
            .reconfigurable()
            .commit(),

            VECTOR_BOOL_ELEMENT(expected)
            .key("vecBool")
            .tags("h/w; d.m.y", ";")
            .allowedStates(State.STARTED, State.STOPPED)
            .minSize(2)
            .maxSize(7)
            .assignmentMandatory()
            .commit(),

            NDARRAY_ELEMENT(expected)
            .key("arrBool")
            .dtype(Types.BOOL)
            .shape([3, 2])
            .commit(),

            NDARRAY_ELEMENT(expected)
            .key("arrInt8")
            .dtype(Types.INT8)
            .shape([3, 2])
            .commit(),

            NDARRAY_ELEMENT(expected)
            .key("arrUInt16")
            .dtype(Types.UINT16)
            .shape([3, 2])
            .commit(),

            NDARRAY_ELEMENT(expected)
            .key("arrFloat")
            .dtype(Types.FLOAT)
            .shape([3, 2])
            .commit(),

            # Keep shape as string to also test that
            NDARRAY_ELEMENT(expected)
            .key("arrDouble")
            .dtype(Types.DOUBLE)
            .shape("3,2,-1")
            .commit(),

            NDARRAY_ELEMENT(expected)
            .key("arrUndefined")
            # leaf type undefined
            .shape("0,3,0")
            .commit(),

            IMAGEDATA_ELEMENT(expected)
            .key("image")
            .commit(),
        )

        rowSchema = Schema()
        (
            INT32_ELEMENT(rowSchema)
            .key("a")
            .minInc(2)
            .maxExc(10)
            .displayedName("A")
            .assignmentOptional()
            .defaultValue(2)
            .commit(),

            STRING_ELEMENT(rowSchema)
            .key("b")
            .assignmentOptional()
            .noDefaultValue()
            .commit(),

            FLOAT_ELEMENT(rowSchema)
            .key("c")
            .assignmentOptional()
            .noDefaultValue()
            .commit(),

            TABLE_ELEMENT(expected)
            .key("testTable")
            .alias([1, 2])
            .tags("hard,soft")
            .description("Test table description")
            .displayedName("Test table")
            .setColumns(rowSchema)
            .maxSize(10)
            .minSize(2)
            .assignmentOptional()
            .defaultValue([Hash("a", 3, "b", "foo", "c", 3.1415),
                           Hash("a", 3, "b", "foo", "c", 2.78)])
            .expertAccess()
            .commit(),

            TABLE_ELEMENT(expected)
            .key("testTableEmptyDefault")
            .alias(3)
            .tags("soft")
            .setColumns(rowSchema)
            .assignmentOptional()
            .defaultValue([])
            .adminAccess()
            .commit(),

            TABLE_ELEMENT(expected)
            .key("testTableNoDefault")
            .alias("aliasForTestTableNoDefault")
            .tags("hard")
            .setColumns(rowSchema)
            .assignmentOptional()
            .noDefaultValue()
            .observerAccess()
            .commit(),

            TABLE_ELEMENT(expected)
            .key("tableMandatory")
            .alias("aliasForTableMandatory")
            .tags("hard")
            .setColumns(rowSchema)
            .assignmentMandatory()
            .operatorAccess()
            .init()
            .commit(),

            TABLE_ELEMENT(expected)
            .key("tableInternal")
            .alias("aliasForTableInternal")
            .tags("hard")
            .description("Test table with assignmentInternal")
            .setColumns(rowSchema)
            .reconfigurable()
            .userAccess()
            .assignmentInternal()
            .noDefaultValue()
            .commit(),

            TABLE_ELEMENT(expected)
            .key("tableReadOnly")
            .alias("aliasForTableReadOnly")
            .tags("exp,hard,soft")
            .setColumns(rowSchema)
            .allowedStates(State.STARTED, State.STOPPED)
            .userAccess()
            .readOnly()
            .initialValue([Hash("a", 22, "b", "foo22", "c", 1.234),
                           Hash("a", 33, "b", "foo33", "c", 2.345),
                           Hash("a", 44, "b", "foo44", "c", 3.456)])
            .archivePolicy(EVERY_EVENT)
            .commit(),
        )

        (
            LIST_ELEMENT(expected)
            .key("shapeList")
            .description("A list of shapes")
            .appendNodesOfConfigurationBase(ShapeX)
            .assignmentOptional()
            .defaultValueFromString("CircleX,RectangleX")
            .commit(),

            # We can also add nodes to the list by hand:
            NODE_ELEMENT(expected)
            .key("shapeList.BizarreForm")
            .description("A funny shape added by hand")
            .commit(),

            FLOAT_ELEMENT(expected)
            .key("shapeList.BizarreForm.length")
            .description("The single length parameter characterizing "
                         "the bizarre form")
            .assignmentOptional()
            .defaultValue(10.)
            .commit(),
        )


class SomeClass:
    @staticmethod
    def expectedParameters(expected):
        if isinstance(expected, karabind.Schema):
            INT32_ELEMENT = karabind.INT32_ELEMENT
            DOUBLE_ELEMENT = karabind.DOUBLE_ELEMENT
            VECTOR_INT32_ELEMENT = karabind.VECTOR_INT32_ELEMENT
            IMAGEDATA_ELEMENT = karabind.IMAGEDATA_ELEMENT
            Unit = karabind.Unit
            MetricPrefix = karabind.MetricPrefix
            EVERY_100MS = karabind.EVERY_100MS
            Encoding = karabind.Encoding
            Types = karabind.Types
        elif isinstance(expected, karathon.Schema):
            INT32_ELEMENT = karathon.INT32_ELEMENT
            DOUBLE_ELEMENT = karathon.DOUBLE_ELEMENT
            VECTOR_INT32_ELEMENT = karathon.VECTOR_INT32_ELEMENT
            IMAGEDATA_ELEMENT = karathon.IMAGEDATA_ELEMENT
            Unit = karathon.Unit
            MetricPrefix = karathon.MetricPrefix
            EVERY_100MS = karathon.EVERY_100MS
            Encoding = karathon.Encoding
            Types = karathon.Types
        else:
            raise TypeError("Unsupported argument type")
        (
            INT32_ELEMENT(expected)
            .key("x")
            .alias(10)
            .tags("IK,BH")
            .displayedName("Xkey")
            .description("Example of X key description")
            .options("5, 25, 10")
            .minInc(5)
            .maxInc(25)
            .unit(Unit.AMPERE)
            .metricPrefix(MetricPrefix.MILLI)
            .assignmentOptional()
            .defaultValue(5)
            .init()
            .expertAccess()
            .commit(),

            INT32_ELEMENT(expected)
            .key("y")
            .alias("bla")
            .tags("CY")
            .displayedName("Ykey")
            .description("Example of Y key description")
            .options("5, 25, 10")
            .minExc(0)
            .maxExc(29)
            .unit(Unit.METER)
            .metricPrefix(MetricPrefix.CENTI)
            .assignmentOptional()
            .defaultValue(10)
            .init()
            .commit(),

            DOUBLE_ELEMENT(expected)
            .key("a")
            .readOnly()
            .initialValue(1.11)
            .alarmLow(-22.1)
            .needsAcknowledging(True)
            .alarmHigh(22.777)
            .needsAcknowledging(True)
            .warnLow(-5.5)
            .needsAcknowledging(True)
            .warnHigh(5.5)
            .needsAcknowledging(True)
            .archivePolicy(EVERY_100MS)
            .commit(),

            VECTOR_INT32_ELEMENT(expected)
            .key("somelist")
            .displayedName("Ykey")
            .description("Example of Y key description")
            .reconfigurable()
            .assignmentOptional()
            .defaultValue([])
            .commit(),

            IMAGEDATA_ELEMENT(expected)
            .key("imageSetDims")
            .displayedName("Image setDimensions")
            .description("ImageData element: setDimensions")
            .setDimensions("110, 210")
            .operatorAccess()
            .commit(),

            IMAGEDATA_ELEMENT(expected)
            .key("imageSetType")
            .displayedName("Image setType")
            .description("ImageData Element: setType('UINT16')")
            .setDimensions("110, 210")
            .setType('UINT16')
            .observerAccess()
            .commit(),

            IMAGEDATA_ELEMENT(expected)
            .key("imageSetEncoding")
            .displayedName("Image setEncoding")
            .description("ImageData Element: setEncoding('RGBA')")
            .setDimensions("300, 200")
            .setType(Types.UINT32)
            .setEncoding("RGBA")
            .userAccess()
            .commit(),

            IMAGEDATA_ELEMENT(expected)
            .key("imageSetGeometry")
            .displayedName("ImageData setGeometry")
            .description("ImageData Element: setGeometry")
            .setDimensions("500, 500")
            .setEncoding(Encoding.GRAY)
            # .setGeometry()
            .expertAccess()
            .commit(),

            IMAGEDATA_ELEMENT(expected)
            .key("myImageElement")
            .setDimensions("500, 800")
            .setDimensionScales("Dimscales")
            .adminAccess()
            .commit(),
        )


@pytest.mark.parametrize(
    "Schema, AssemblyRules, AccessLevel",
    [(karathon.Schema, karathon.AssemblyRules, karathon.AccessLevel),
     (karabind.Schema, karabind.AssemblyRules, karabind.AccessLevel)])
def test_buildUp(Schema, AssemblyRules, AccessLevel):
    schema = ShapeX.getSchema("CircleX", AssemblyRules())
    assert schema.isAccessInitOnly("shadowEnabled") is True
    assert schema.isAccessInitOnly("radius") is True
    assert schema.isLeaf("radius") is True
    assert schema.getDisplayedName("radius") == "Radius"
    assert schema.getDescription("radius") == "The radius of the circle"
    assert schema.getAliasFromKey("radius") == 1
    assert schema.getAliasAsString("radius") == "1"
    assert schema.hasTags("radius") is False
    assert schema.isAssignmentOptional("radius") is True
    assert schema.isAssignmentMandatory("radius") is False
    assert schema.isAccessReadOnly("radius") is False
    assert schema.isAccessReconfigurable("radius") is False
    assert schema.isAccessInitOnly("radius") is True
    assert schema.getUnitName("radius") == "meter"
    assert schema.getMetricPrefixName("radius") == "milli"
    assert schema.hasMinExc("radius") is True
    assert schema.hasMinInc("radius") is False
    assert schema.getMinExc("radius") == 0
    assert schema.getMaxExc("radius") == 100
    assert schema.getRequiredAccessLevel("radius") == AccessLevel.USER

    schema = Schema()
    EditableCircleX.expectedParameters(schema)
    allowedStates = schema.getOptions("state")
    assert allowedStates == ['INIT', 'ERROR', 'NORMAL']
    assert schema.getDefaultValue("state") == 'INIT'
    assert schema.getDefaultValue("stateN") == 'NORMAL'
    assert schema.getDefaultValue("stateE") == 'ERROR'
    assert schema.getDefaultValue("alarmW") == 'warn'
    assert schema.getDefaultValue("alarmA") == 'alarm'
    assert schema.getDefaultValue("alarm") == 'warnLow'
    assert schema.getDisplayedName("radius") == "New Radius"
    assert schema.getAliasFromKey("radius") == 21
    assert schema.getAliasAsString("radius") == "21"
    assert schema.getTags("radius") == ["operation,control"]
    assert schema.isAssignmentOptional("radius") is False
    assert schema.isAssignmentMandatory("radius") is True
    assert schema.isAccessReadOnly("radius") is False
    assert schema.isAccessReconfigurable("radius") is True
    assert schema.getUnitName("radius") == "meter"
    assert schema.getMetricPrefixName("radius") == "kilo"
    assert schema.hasMinExc("radius") is True
    assert schema.hasMinInc("radius") is True
    assert schema.getMinInc("radius") == 12
    assert schema.getMaxExc("radius") == 120
    assert schema.getRequiredAccessLevel("radius") == AccessLevel.EXPERT
    assert schema.getDescription("alarm") == "The new alarm description"
    assert schema.getRequiredAccessLevel("alarm") == AccessLevel.USER

    allowedStates = schema.getOptions("status")
    assert allowedStates == ['a', 'b', 'c']
    assert schema.getDefaultValue("status") == 'a'

    schema = Schema()
    EditableRectangleX.expectedParameters(schema)
    assert schema.getMinExc("a") == 30
    assert schema.getMaxInc("a") == 60
    assert schema.isAssignmentInternal("a") is True
    assert schema.isAssignmentOptional("a") is False
    assert schema.isAssignmentMandatory("a") is False
    assert schema.isAccessReadOnly("a") is False
    assert schema.isAccessReadOnly("b") is True
    assert schema.getRequiredAccessLevel("a") == AccessLevel.ADMIN
    assert schema.getRequiredAccessLevel("b") == AccessLevel.OPERATOR

    schema = Schema()
    GraphicsRendererX1.expectedParameters(schema)
    assert schema.isAccessInitOnly("shapes.circle.radius") is True
    assert schema.isLeaf("shapes.circle.radius") is True


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_getRootName(Schema):
    schema = Schema("MyTest")
    TestStruct1.expectedParameters(schema)
    assert schema.getRootName() == "MyTest"


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_getTags(Schema):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getTags("exampleKey1")[0] == "hardware"
    assert schema.getTags("exampleKey1")[1] == "poll"
    assert schema.getTags("exampleKey2")[0] == "hardware"
    assert schema.getTags("exampleKey2")[1] == "poll"
    assert schema.getTags("exampleKey3")[0] == "hardware"
    assert schema.getTags("exampleKey3")[1] == "set"
    assert schema.getTags("exampleKey4")[0] == "software"
    assert schema.getTags("exampleKey5")[0] == "h/w"
    assert schema.getTags("exampleKey5")[1] == "d.m.y"


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_setTags(Schema):
    schema = Schema()
    SomeClass.expectedParameters(schema)
    assert schema.hasTags('x') is True
    assert schema.getTags('x') == ['IK', 'BH']
    schema.setTags('x', 'CY,SE')
    assert schema.getTags('x') == ['CY', 'SE']


@pytest.mark.parametrize(
    "Schema, AccessLevel",
    [(karathon.Schema, karathon.AccessLevel),
     (karabind.Schema, karabind.AccessLevel)])
def test_getsetAccessLevel(Schema, AccessLevel):
    sch = Schema()
    SomeClass.expectedParameters(sch)
    assert sch.getRequiredAccessLevel('x') == AccessLevel.EXPERT
    assert sch.getRequiredAccessLevel('y') == AccessLevel.USER
    assert sch.getRequiredAccessLevel('a') == AccessLevel.OBSERVER

    sch.setRequiredAccessLevel('x', AccessLevel.ADMIN)
    sch.setRequiredAccessLevel('y', AccessLevel.OPERATOR)
    assert sch.getRequiredAccessLevel('x') == AccessLevel.ADMIN
    assert sch.getRequiredAccessLevel('y') == AccessLevel.OPERATOR

    sch = Schema()
    TestStruct1.expectedParameters(sch)
    assert sch.getRequiredAccessLevel('exampleKey1') == AccessLevel.USER
    assert sch.getRequiredAccessLevel('exampleKey2') == AccessLevel.OPERATOR
    assert sch.getRequiredAccessLevel('exampleKey3') == AccessLevel.EXPERT
    assert sch.getRequiredAccessLevel('exampleKey4') == AccessLevel.ADMIN
    # default for readOnly
    assert sch.getRequiredAccessLevel('exampleKey5') == AccessLevel.OBSERVER
    # default for reconfigurable
    assert sch.getRequiredAccessLevel('exampleKey10') == AccessLevel.USER
    # observerAccess in reconfigurable
    assert sch.getRequiredAccessLevel('exampleKey11') == AccessLevel.OBSERVER


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_isNode(Schema):
    schema = Schema("test")
    GraphicsRendererX1.expectedParameters(schema)
    assert schema.getRootName() == "test"
    assert schema.isNode("shapes.circle") is True
    assert schema.isNode("shapes.rectangle") is True
    assert schema.isNode("shapes") is False


@pytest.mark.parametrize(
    "Schema, NodeType",
    [(karathon.Schema, karathon.NodeType),
     (karabind.Schema, karabind.NodeType)])
def test_getNodeType(Schema, NodeType):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    nodeType = schema.getNodeType("exampleKey1")
    assert nodeType == NodeType.LEAF
    assert schema.getNodeType("exampleKey5") == NodeType.LEAF
    schema = Schema()
    GraphicsRendererX1.expectedParameters(schema)
    assert schema.getNodeType("shapes") == NodeType.CHOICE_OF_NODES
    assert schema.getNodeType("shapes.circle") == NodeType.NODE
    assert schema.getNodeType("shapes.rectangle") == NodeType.NODE


@pytest.mark.parametrize(
    "Schema, Types",
    [(karathon.Schema, karathon.Types),
     (karabind.Schema, karabind.Types)])
def test_getValueType(Schema, Types):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getValueType("exampleKey1") == Types.STRING
    assert schema.getValueType("exampleKey2") == Types.INT32
    assert schema.getValueType("exampleKey3") == Types.UINT32
    assert schema.getValueType("exampleKey4") == Types.DOUBLE
    assert schema.getValueType("exampleKey5") == Types.INT64
    assert schema.getValueType("exampleKey5b") == Types.UINT64
    assert schema.getValueType("exampleKey7") == Types.VECTOR_INT32
    assert schema.getValueType("exampleKey7b") == Types.VECTOR_UINT32
    assert schema.getValueType("exampleKey8") == Types.VECTOR_DOUBLE
    assert schema.getValueType("exampleKey9") == Types.VECTOR_STRING


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_getAliasAsString(Schema):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getAliasAsString("exampleKey2") == "10"
    assert schema.getAliasAsString("exampleKey3") == "5.5"
    assert schema.getAliasAsString("exampleKey4") == "exampleAlias4"
    assert schema.getAliasAsString("exampleKey5") == "exampleAlias5"
    assert schema.getAliasAsString("exampleKey6") == "1193046,43724"
    assert schema.getAliasAsString("testPath") == "5"


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_keyHasAlias(Schema):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.keyHasAlias("exampleKey1") is False
    assert schema.keyHasAlias("exampleKey2") is True
    assert schema.keyHasAlias("exampleKey3") is True
    assert schema.keyHasAlias("exampleKey4") is True
    assert schema.keyHasAlias("exampleKey5") is True
    assert schema.keyHasAlias("exampleKey6") is True
    assert schema.keyHasAlias("testPath") is True


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_aliasHasKey(Schema):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.aliasHasKey(10) is True
    assert schema.aliasHasKey(5.5) is True
    assert schema.aliasHasKey("exampleAlias4") is True
    assert schema.aliasHasKey("exampleAlias5") is True
    assert schema.aliasHasKey([0x00123456, 0x0000aacc]) is True
    assert schema.aliasHasKey(7) is False
    assert schema.aliasHasKey(5) is True


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_getAliasFromKey(Schema):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getAliasFromKey("exampleKey2") == 10
    assert schema.getAliasFromKey("exampleKey3") == 5.5
    assert schema.getAliasFromKey("exampleKey4") == "exampleAlias4"
    assert schema.getAliasFromKey("exampleKey5") == "exampleAlias5"
    assert schema.getAliasFromKey("exampleKey6") == [0x00123456, 0x0000aacc]
    assert schema.getAliasFromKey("testPath") == 5


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_setAlias(Schema):
    schema = Schema()
    SomeClass.expectedParameters(schema)
    assert schema.getAliasFromKey("x") == 10
    schema.setAlias('x', 'abc')
    assert schema.getAliasFromKey("x") == 'abc'
    schema.setAlias('x', 99)
    assert schema.getAliasFromKey("x") == 99


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_getKeyFromAlias(Schema):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getKeyFromAlias(10) == "exampleKey2"
    assert schema.getKeyFromAlias(5.5) == "exampleKey3"
    assert schema.getKeyFromAlias("exampleAlias4") == "exampleKey4"
    assert schema.getKeyFromAlias("exampleAlias5") == "exampleKey5"
    assert schema.getKeyFromAlias([0x00123456, 0x0000aacc]) == "exampleKey6"
    assert schema.getKeyFromAlias(5) == "testPath"


@pytest.mark.parametrize(
    "Schema, AccessType",
    [(karathon.Schema, karathon.AccessType),
     (karabind.Schema, karabind.AccessType)])
def test_getAccessMode(Schema, AccessType):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getAccessMode("exampleKey1") == AccessType.WRITE
    assert schema.getAccessMode("exampleKey2") == AccessType.INIT
    assert schema.getAccessMode("exampleKey3") == AccessType.WRITE
    assert schema.getAccessMode("exampleKey4") == AccessType.INIT
    assert schema.getAccessMode("exampleKey5") == AccessType.READ
    assert schema.getAccessMode("exampleKey8") == AccessType.READ
    assert schema.getAccessMode("exampleKey10") == AccessType.WRITE
    assert schema.getAccessMode("testPath") == AccessType.WRITE
    assert schema.getAccessMode("testPath2") == AccessType.READ
    assert schema.getAccessMode("testPath3") == AccessType.INIT


@pytest.mark.parametrize(
    "Schema, AssignmentType",
    [(karathon.Schema, karathon.AssignmentType),
     (karabind.Schema, karabind.AssignmentType)])
def test_getAssignment(Schema, AssignmentType):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getAssignment("exampleKey1") == AssignmentType.OPTIONAL
    assert schema.getAssignment("exampleKey2") == AssignmentType.OPTIONAL
    assert schema.getAssignment("exampleKey3") == AssignmentType.MANDATORY
    assert schema.getAssignment("exampleKey4") == AssignmentType.INTERNAL
    assert schema.getAssignment("exampleKey5") == AssignmentType.OPTIONAL
    assert schema.getAssignment("exampleKey8") == AssignmentType.OPTIONAL
    assert schema.getAssignment("exampleKey10") == AssignmentType.OPTIONAL
    assert schema.getAssignment("testPath") == AssignmentType.OPTIONAL
    assert schema.getAssignment("testPath2") == AssignmentType.OPTIONAL
    assert schema.getAssignment("testPath3") == AssignmentType.MANDATORY


@pytest.mark.parametrize(
    "Schema, AssignmentType",
    [(karathon.Schema, karathon.AssignmentType),
     (karabind.Schema, karabind.AssignmentType)])
def test_setAssignment(Schema, AssignmentType):
    schema = Schema()
    SomeClass.expectedParameters(schema)
    assert schema.hasAssignment('x') is True
    assert schema.isAssignmentOptional('x') is True
    assert schema.isAssignmentMandatory('x') is False
    assert schema.getAssignment('x') == AssignmentType.OPTIONAL
    schema.setAssignment('x', AssignmentType.MANDATORY)
    assert schema.isAssignmentOptional('x') is False
    assert schema.isAssignmentMandatory('x') is True
    assert schema.getAssignment('x') == AssignmentType.MANDATORY


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_getDescription(Schema):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getDescription('exampleKey1') == "Example key 1 description"


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_setDescription(Schema):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    schema.setDescription('exampleKey1', "No description")
    assert schema.getDescription('exampleKey1') == "No description"


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_getOptions(Schema):
    schema = Schema()
    TestStruct1.expectedParameters(schema)

    options = schema.getOptions("exampleKey1")
    assert options[0] == "Radio"
    assert options[1] == "Air Condition"
    assert options[2] == "Navigation"

    assert schema.getOptions("exampleKey2")[0] == 5
    assert schema.getOptions("exampleKey2")[1] == 25
    assert schema.getOptions("exampleKey2")[2] == 10
    assert schema.getOptions("exampleKey4")[0] == 1.11
    assert schema.getOptions("exampleKey4")[1] == -2.22
    assert schema.getOptions("exampleKey4")[2] == 5.55

    assert schema.getOptions("testPath")[0] == "file1"
    assert schema.getOptions("testPath")[1] == "file2"


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_setOptions(Schema):
    schema = Schema()
    SomeClass.expectedParameters(schema)

    options = schema.getOptions("x")
    assert options[0] == 5
    assert options[1] == 25
    assert options[2] == 10
    assert schema.getOptions("x") == [5, 25, 10]

    schema.setOptions('x', '20, 5, 11, 13, 25')
    options = schema.getOptions("x")
    assert options == [20, 5, 11, 13, 25]


# @pytest.mark.parametrize(
#     "Schema", [(karathon.Schema), (karabind.Schema)])
# def test_displayType(Schema):
#     schema = Schema()
#     TestStruct1.expectedParameters(schema)
#     display = schema.getDisplayType("myNode")
#     assert display == "WidgetNode"


@pytest.mark.parametrize(
    "Schema, Types",
    [(karathon.Schema, karathon.Types),
     (karabind.Schema, karabind.Types)])
def test_getDefaultValue(Schema, Types):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getDefaultValue("exampleKey1") == "Navigation"
    assert schema.getDefaultValue("exampleKey2") == 10
    assert schema.getDefaultValueAs("exampleKey2", Types.STRING) == "10"
    assert schema.getDefaultValue("exampleKey5") == 1442244
    assert schema.getDefaultValueAs("exampleKey5", Types.STRING) == "1442244"
    assert schema.getDefaultValue("exampleKey6") == 1.11
    assert schema.getDefaultValueAs("exampleKey6", Types.DOUBLE) == 1.11
    assert schema.getDefaultValue("exampleKey7") == [1, 2, 3]
    assert schema.getDefaultValue("exampleKey8") == [1.1, 2.2, 3.3]
    assert schema.getDefaultValue("exampleKey9") == ["Hallo", "World"]

    # 'readOnly'-element that does not specify
    #  'initialValue' has 'defaultValue' equal to string "" :
    assert schema.getDefaultValue("testPath2") == ""
    # ... and for vectors equal to empty list:
    assert schema.getDefaultValue("vectInt") == []

    assert schema.getDefaultValue("exampleIntKey") == 20

    assert schema.getDefaultValue("exampleKey5") == 1442244

    assert schema.getDefaultValue("exampleKey7") == [1, 2, 3]

    # readOnly default specified by 'defaultValue'. not 'initialValue':
    assert schema.getDefaultValue("exampleKey5b") == 42
    assert schema.getDefaultValue("exampleKey7b") == [11, 22, 33]


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_setDefaultValue(Schema):
    schema = Schema()
    SomeClass.expectedParameters(schema)
    assert schema.isAssignmentOptional('x') is True
    assert schema.hasDefaultValue('x') is True
    assert schema.getDefaultValue("x") == 5
    schema.setDefaultValue("x", 10)
    assert schema.getDefaultValue("x") == 10


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_getAllowedStates(Schema):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    astates = schema.getAllowedStates("exampleKey3")
    assert astates[0] == State.STARTED
    assert astates[1] == State.STOPPED
    assert astates[2] == State.NORMAL
    assert schema.getAllowedStates("exampleKey3")[2] == State.NORMAL
    assert schema.getAllowedStates("exampleKey7")[0] == State.STARTED
    assert schema.getAllowedStates("exampleKey7")[1] == State.NORMAL


@pytest.mark.parametrize(
    "Schema, Unit",
    [(karathon.Schema, karathon.Unit),
     (karabind.Schema, karabind.Unit)])
def test_getUnit(Schema, Unit):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getUnit("exampleKey2") == Unit.METER
    assert schema.getUnitName("exampleKey2") == "meter"
    assert schema.getUnitSymbol("exampleKey2") == "m"


@pytest.mark.parametrize(
    "Schema, Unit, METER",
    [(karathon.Schema, karathon.Unit, karathon.METER),
     (karabind.Schema, karabind.Unit, karabind.METER)])
def test_setUnit(Schema, Unit, METER):
    schema = Schema()
    SomeClass.expectedParameters(schema)
    assert schema.getUnit("x") == Unit.AMPERE
    schema.setUnit('x', METER)
    assert schema.getUnit("x") == METER
    assert schema.getUnit("x") == Unit.METER
    assert schema.getUnitName("x") == "meter"
    assert schema.getUnitSymbol("x") == "m"


@pytest.mark.parametrize(
    "Schema, MetricPrefix",
    [(karathon.Schema, karathon.MetricPrefix),
     (karabind.Schema, karabind.MetricPrefix)])
def test_getMetricPrefix(Schema, MetricPrefix):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getMetricPrefix("exampleKey2") == MetricPrefix.MILLI
    assert schema.getMetricPrefixName("exampleKey2") == "milli"
    assert schema.getMetricPrefixSymbol("exampleKey2") == "m"


@pytest.mark.parametrize(
    "Schema, MetricPrefix, MICRO",
    [(karathon.Schema, karathon.MetricPrefix, karathon.MICRO),
     (karabind.Schema, karabind.MetricPrefix, karabind.MICRO)])
def test_setMetricPrefix(Schema, MetricPrefix, MICRO):
    schema = Schema()
    SomeClass.expectedParameters(schema)
    assert schema.getMetricPrefix("x") == MetricPrefix.MILLI
    schema.setMetricPrefix("x", MetricPrefix.MICRO)
    assert schema.getMetricPrefix("x") == MICRO
    assert schema.getMetricPrefixName("x") == "micro"
    assert schema.getMetricPrefixSymbol("x") == "u"


@pytest.mark.parametrize(
    "Schema, Types",
    [(karathon.Schema, karathon.Types),
     (karabind.Schema, karabind.Types)])
def test_getMinIncMaxInc(Schema, Types):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getMinInc("exampleKey2") == 5
    assert schema.getMinIncAs("exampleKey2", Types.STRING) == "5"
    assert schema.getMaxInc("exampleKey2") == 25
    assert schema.getMaxIncAs("exampleKey2", Types.STRING) == "25"


@pytest.mark.parametrize(
    "Schema, Types",
    [(karathon.Schema, karathon.Types),
     (karabind.Schema, karabind.Types)])
def test_setMinIncMaxInc(Schema, Types):
    schema = Schema()
    SomeClass.expectedParameters(schema)
    assert schema.getMinInc("x") == 5
    assert schema.getMinIncAs("x", Types.STRING) == "5"
    assert schema.getMaxInc("x") == 25
    assert schema.getMaxIncAs("x", Types.STRING) == "25"
    schema.setMinInc('x', 3)
    schema.setMaxInc('x', 30)
    assert schema.getMinInc("x") == 3
    assert schema.getMinIncAs("x", Types.STRING) == "3"
    assert schema.getMaxInc("x") == 30
    assert schema.getMaxIncAs("x", Types.STRING) == "30"


@pytest.mark.parametrize(
    "Schema, Types",
    [(karathon.Schema, karathon.Types),
     (karabind.Schema, karabind.Types)])
def test_getMinExcMaxExc(Schema, Types):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getMinExc("exampleKey3") == 10
    assert schema.getMinExc("exampleKey4") == -2.22
    assert schema.getMaxExc("exampleKey3") == 20
    assert schema.getMaxExc("exampleKey4") == 5.55
    assert schema.getMinExcAs("exampleKey3", Types.STRING) == "10"
    assert schema.getMinExcAs("exampleKey4", Types.STRING) == "-2.22"
    assert schema.getMaxExcAs("exampleKey3", Types.STRING) == "20"
    assert schema.getMaxExcAs("exampleKey4", Types.STRING) == "5.55"


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_setMinExcMaxExc(Schema):
    schema = Schema()
    SomeClass.expectedParameters(schema)
    assert schema.getMinExc("y") == 0
    assert schema.getMaxExc("y") == 29
    schema.setMinExc("y", 2)
    schema.setMaxExc("y", 30)
    assert schema.getMinExc("y") == 2
    assert schema.getMaxExc("y") == 30


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_hasgetsetMinMaxSize(Schema):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.hasMinSize('exampleKey10') is True
    assert schema.hasMaxSize('exampleKey10') is True
    assert schema.getMinSize('exampleKey10') == 2
    assert schema.getMaxSize('exampleKey10') == 7
    schema.setMinSize('exampleKey10', 5)
    schema.setMaxSize('exampleKey10', 12)
    assert schema.getMinSize('exampleKey10') == 5
    assert schema.getMaxSize('exampleKey10') == 12
    # ------
    assert schema.hasMinSize('exampleKey11') is False
    assert schema.hasMaxSize('exampleKey11') is False
    schema.setMinSize('exampleKey11', 1)
    schema.setMaxSize('exampleKey11', 42)
    assert schema.hasMinSize('exampleKey11') is True
    assert schema.hasMaxSize('exampleKey11') is True
    assert schema.getMinSize('exampleKey11') == 1
    assert schema.getMaxSize('exampleKey11') == 42


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_hasgetsetMinMax(Schema):
    schema = Schema()
    OtherSchemaElementsX.expectedParameters(schema)
    assert schema.hasMin('shapeList') is False
    assert schema.hasMax('shapeList') is False
    schema.setMin('shapeList', 1)
    schema.setMax('shapeList', 5)
    assert schema.hasMin('shapeList') is True
    assert schema.hasMax('shapeList') is True
    assert schema.getMin('shapeList') == 1
    assert schema.getMax('shapeList') == 5


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_listelem(Schema):
    # sch = Configurator(OtherSchemaElementsX).getSchema(
    #     "OtherSchemaElementsX", AssemblyRules())
    sch = Schema()
    OtherSchemaElementsX.expectedParameters(sch)
    assert sch.has("shapeList") is True
    assert sch.isListOfNodes("shapeList") is True
    assert sch.isNode("shapeList") is False
    assert sch.getDefaultValue("shapeList") == ['CircleX', 'RectangleX']
    assert sch.has("shapeList.CircleX") is True
    assert sch.isNode("shapeList.CircleX") is True
    assert sch.has("shapeList.BizarreForm") is True
    assert sch.isNode("shapeList.BizarreForm") is True
    assert sch.has("shapeList.BizarreForm.length") is True
    assert sch.isLeaf("shapeList.BizarreForm.length") is True


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_getWarnAlarmLowHigh(Schema):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getWarnLow("exampleKey5") == -10
    assert schema.getWarnLow("exampleKey6") == -5.5
    assert schema.getWarnHigh("exampleKey5") == 10
    assert schema.getWarnHigh("exampleKey6") == 5.5
    assert schema.getAlarmLow("exampleKey5") == -20
    assert schema.getAlarmLow("exampleKey6") == -22.1
    assert schema.getAlarmHigh("exampleKey5") == 20
    assert schema.getAlarmHigh("exampleKey6") == 22.777


@pytest.mark.parametrize(
    "Schema, Types",
    [(karathon.Schema, karathon.Types),
     (karabind.Schema, karabind.Types)])
def test_getWarnAlarmLowHighAs(Schema, Types):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getWarnLowAs("exampleKey5", Types.STRING) == "-10"
    assert schema.getWarnLowAs("exampleKey6", Types.STRING) == "-5.5"
    assert schema.getWarnHighAs("exampleKey5", Types.STRING) == "10"
    assert schema.getWarnHighAs("exampleKey6", Types.STRING) == "5.5"
    assert schema.getAlarmLowAs("exampleKey5", Types.STRING) == "-20"
    assert schema.getAlarmLowAs("exampleKey6", Types.STRING) == "-22.1"
    assert schema.getAlarmHighAs("exampleKey5", Types.STRING) == "20"
    assert schema.getAlarmHighAs("exampleKey6", Types.STRING) == "22.777"


@pytest.mark.parametrize(
    "Schema", [(karathon.Schema), (karabind.Schema)])
def test_hasWarnAlarm(Schema):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.hasWarnLow("exampleKey5") is True
    assert schema.hasWarnHigh("exampleKey5") is True
    assert schema.hasWarnLow("exampleKey6") is True
    assert schema.hasWarnHigh("exampleKey6") is True
    assert schema.hasAlarmLow("exampleKey6") is True
    assert schema.hasAlarmHigh("exampleKey6") is True
    assert schema.hasAlarmHigh("exampleKey1") is False


@pytest.mark.parametrize(
    "Schema, Types",
    [(karathon.Schema, karathon.Types),
     (karabind.Schema, karabind.Types)])
def test_vectorElement(Schema, Types):
    s = Schema()
    TestStruct1.expectedParameters(s)
    assert s.isAccessReadOnly("exampleKey7") is True
    assert s.hasDefaultValue("exampleKey7") is True
    assert s.hasDefaultValue("exampleKey10") is True
    assert s.hasDefaultValue("exampleKey11") is True
    assert s.hasDefaultValue("exampleKey12") is True
    assert s.hasDefaultValue("exampleKey14") is True
    assert s.hasDefaultValue("exampleKey15") is True
    assert s.getDefaultValue("exampleKey10") == [10, 20, 30]
    assert s.getDefaultValue("exampleKey12") == [1.1, -2.2, 3.3]
    assert s.getDefaultValue("exampleKey11") == [10, 20, 30]
    assert s.getDefaultValueAs("exampleKey11", Types.STRING) == "10,20,30"
    assert s.getDefaultValueAs("exampleKey11",
                               Types.VECTOR_INT32) == [10, 20, 30]
    assert s.getDefaultValue("exampleKey14") == ["Hallo", "World", "Test"]
    assert s.getDefaultValueAs("exampleKey14",
                               Types.STRING) == "Hallo,World,Test"
    assert s.getDefaultValue("exampleKey15") == ["word1", "word2", "test"]
    assert s.getDefaultValueAs("exampleKey15",
                               Types.STRING) == "word1,word2,test"
    assert s.getMinSize("exampleKey10") == 2
    assert s.getMaxSize("exampleKey10") == 7


@pytest.mark.parametrize(
    "Schema, Hash, fullyEqual",
    [(karathon.Schema, karathon.Hash, karathon.fullyEqual),
     (karabind.Schema, karabind.Hash, karabind.fullyEqual)])
def test_table_element(Schema, Hash, fullyEqual):
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    # Both, defaultValue and initialValue should work for read-only tables
    default = schema.getDefaultValue("tableI")
    assert len(default) == 1
    assert fullyEqual(default[0], Hash("int", 2)) is True
    default = schema.getDefaultValue("tableD")
    assert len(default) == 1
    assert fullyEqual(default[0], Hash("int", 3)) is True

    # There are more tests of tables in the integration tests...


@pytest.mark.parametrize(
    "Schema, INT32_ELEMENT, OVERWRITE_ELEMENT",
    [(karathon.Schema, karathon.INT32_ELEMENT, karathon.OVERWRITE_ELEMENT),
     (karabind.Schema, karabind.INT32_ELEMENT, karabind.OVERWRITE_ELEMENT)])
def test_overwrite_restrictions_for_options(Schema, INT32_ELEMENT,
                                            OVERWRITE_ELEMENT):
    schema = Schema()
    (
        INT32_ELEMENT(schema).key("range")
        .displayedName("Range")
        .options("0,1")
        .assignmentOptional().defaultValue(0)
        .reconfigurable()
        .commit()
    )
    vec = schema.getOptions("range")
    assert len(vec) == 2
    assert vec[0] == 0
    assert vec[1] == 1
    (
        OVERWRITE_ELEMENT(schema).key("range")
        .setNewOptions("0,1,2")
        .commit()
    )
    vec = schema.getOptions("range")
    assert len(vec) == 3
    assert vec[0] == 0
    assert vec[1] == 1
    assert vec[2] == 2


@pytest.mark.parametrize(
    "Schema, INT32_ELEMENT, OVERWRITE_ELEMENT",
    [(karathon.Schema, karathon.INT32_ELEMENT, karathon.OVERWRITE_ELEMENT),
     (karabind.Schema, karabind.INT32_ELEMENT, karabind.OVERWRITE_ELEMENT)])
def test_overwrite_tags(Schema, INT32_ELEMENT, OVERWRITE_ELEMENT):
    schema = Schema()
    (
        INT32_ELEMENT(schema).key("taggedProp")
        .tags("bip, bop")
        .assignmentOptional().defaultValue(0)
        .reconfigurable()
        .commit()
    )
    vec = schema.getTags("taggedProp")
    assert len(vec) == 2
    assert vec[0] == "bip"
    assert vec[1] == "bop"
    (
        OVERWRITE_ELEMENT(schema).key("taggedProp")
        .setNewTags("doff")
        .commit()
    )
    vec = schema.getTags("taggedProp")
    assert len(vec) == 1
    assert vec[0] == "doff"
    (
        OVERWRITE_ELEMENT(schema).key("taggedProp")
        .setNewTags(["doff", "deff"])
        .commit()
    )
    vec = schema.getTags("taggedProp")
    assert len(vec) == 2
    assert vec[0] == "doff"
    assert vec[1] == "deff"
    (
        OVERWRITE_ELEMENT(schema).key("taggedProp")
        .setNewTags(("chip", "chop"))
        .commit()
    )
    vec = schema.getTags("taggedProp")
    assert len(vec) == 2
    assert vec[0] == "chip"
    assert vec[1] == "chop"
    # Only iterable of str are allowed
    with pytest.raises(RuntimeError):
        (
            OVERWRITE_ELEMENT(schema).key("taggedProp")
            .setNewTags((1, 2))
            .commit()
        )


@pytest.mark.parametrize(
    "Schema",
    [(karathon.Schema), (karabind.Schema)])
def test_choice_element(Schema):
    s = Schema()
    TestStruct1.expectedParameters(s)
    # ------ Optional, defaultValueFromString
    assert s.has("testChoice1") is True
    assert s.isAssignmentOptional("testChoice1") is True
    assert s.hasDefaultValue("testChoice1") is True
    assert s.getDefaultValue("testChoice1") == "RectangleX"
    assert s.isAccessReconfigurable("testChoice1") is True
    assert s.has("testChoice1.CircleX") is True
    assert s.has("testChoice1.CircleX.radius") is True
    assert s.has("testChoice1.RectangleX") is True
    assert s.has("testChoice1.RectangleX.a") is True
    assert s.has("testChoice1.RectangleX.b") is True
    # ------ Optional, noDefaultValue
    assert s.has("testChoice2") is True
    assert s.isAssignmentOptional("testChoice2") is True
    assert s.hasDefaultValue("testChoice2") is False
    assert s.isAccessReconfigurable("testChoice2") is True
    assert s.has("testChoice2.CircleX") is True
    assert s.has("testChoice2.RectangleX") is True
    assert s.has("testChoice2.RectangleX.a") is True
    assert s.has("testChoice2.RectangleX.b") is True
    # ------ Mandatory
    assert s.has("testChoice3") is True
    assert s.isAssignmentOptional("testChoice3") is False
    assert s.isAssignmentMandatory("testChoice3") is True
    assert s.hasDefaultValue("testChoice3") is False
    assert s.isAccessReconfigurable("testChoice3") is False
    assert s.isAccessInitOnly("testChoice3") is True
    assert s.has("testChoice3.CircleX") is True
    assert s.has("testChoice3.RectangleX") is True
    assert s.has("testChoice3.RectangleX.a") is True
    assert s.has("testChoice3.RectangleX.b") is True
    # ------ Optional, defaultValue
    assert s.has("testChoice4") is True
    assert s.isAssignmentOptional("testChoice4") is True
    assert s.hasDefaultValue("testChoice4") is True
    assert s.getDefaultValue("testChoice4") == "CircleX"
    assert s.isAccessReconfigurable("testChoice4") is True
    assert s.has("testChoice4.CircleX") is True
    assert s.has("testChoice1.CircleX.radius") is True
    assert s.has("testChoice4.RectangleX") is True
    assert s.has("testChoice4.RectangleX.a") is True
    assert s.has("testChoice4.RectangleX.b") is True


@pytest.mark.parametrize(
    "Schema, AccessLevel, ArchivePolicy",
    [(karathon.Schema, karathon.AccessLevel, karathon.ArchivePolicy),
     (karabind.Schema, karabind.AccessLevel, karabind.ArchivePolicy)])
def test_table_elements(Schema, AccessLevel, ArchivePolicy):
    sch = Schema()
    OtherSchemaElementsX.expectedParameters(sch)
    assert sch.has("testTable") is True
    assert sch.getMinSize("testTable") == 2
    assert sch.getMaxSize("testTable") == 10
    assert sch.getAliasFromKey("testTable") == [1, 2]
    assert sch.getTags("testTable") == ['hard', 'soft']
    assert sch.getDescription("testTable") == "Test table description"
    assert sch.getDisplayedName("testTable") == 'Test table'
    assert sch.isAssignmentOptional("testTable") is True
    assert sch.isAssignmentMandatory("testTable") is False
    assert sch.hasDefaultValue("testTable") is True
    assert sch.isAccessInitOnly("testTable") is True
    assert sch.getRequiredAccessLevel("testTable") == AccessLevel.EXPERT

    assert sch.has("testTableEmptyDefault") is True
    assert sch.getAliasFromKey("testTableEmptyDefault") == 3
    assert sch.getTags("testTableEmptyDefault") == ['soft']
    assert sch.isAssignmentOptional("testTableEmptyDefault") is True
    assert sch.hasDefaultValue("testTableEmptyDefault") is True
    assert sch.isAccessInitOnly("testTableEmptyDefault") is True
    assert (sch.getRequiredAccessLevel("testTableEmptyDefault") ==
            AccessLevel.ADMIN)

    assert sch.has("testTableNoDefault") is True
    assert (sch.getAliasFromKey("testTableNoDefault") ==
            "aliasForTestTableNoDefault")
    assert sch.getTags("testTableNoDefault") == ['hard']
    assert sch.isAccessReconfigurable("testTableNoDefault") is False
    assert sch.isAccessInitOnly("testTableNoDefault") is True
    assert sch.isAssignmentOptional("testTableNoDefault") is True
    assert sch.hasDefaultValue("testTableNoDefault") is False
    assert (sch.getRequiredAccessLevel("testTableNoDefault") ==
            AccessLevel.OBSERVER)

    assert sch.has("tableMandatory") is True
    assert sch.getAliasFromKey("tableMandatory") == "aliasForTableMandatory"
    assert sch.getTags("tableMandatory") == ['hard']
    assert sch.isAssignmentInternal("tableMandatory") is False
    assert sch.isAssignmentMandatory("tableMandatory") is True
    assert sch.isAccessInitOnly("tableMandatory") is True
    assert (sch.getRequiredAccessLevel("tableMandatory") ==
            AccessLevel.OPERATOR)

    assert sch.has("tableInternal") is True
    assert sch.getAliasFromKey("tableInternal") == "aliasForTableInternal"
    assert sch.getTags("tableInternal") == ['hard']
    assert sch.isAssignmentInternal("tableInternal") is True
    assert sch.isAssignmentMandatory("tableInternal") is False
    assert sch.isAccessReconfigurable("tableInternal") is True
    assert sch.hasDefaultValue("tableInternal") is False
    assert sch.getRequiredAccessLevel("tableInternal") == AccessLevel.USER

    assert sch.has("tableReadOnly") is True
    assert sch.getAliasFromKey("tableReadOnly") == "aliasForTableReadOnly"
    assert sch.getTags("tableReadOnly") == ['exp', 'hard', 'soft']
    assert sch.isAssignmentOptional("tableReadOnly") is True
    assert sch.isAccessReadOnly("tableReadOnly") is True
    assert sch.hasDefaultValue("tableReadOnly") is True
    assert sch.hasAllowedStates("tableReadOnly") is True
    assert (sch.getAllowedStates("tableReadOnly") ==
            [State.STARTED, State.STOPPED])
    assert sch.getRequiredAccessLevel("tableReadOnly") == AccessLevel.USER
    assert sch.hasArchivePolicy("tableReadOnly") is True
    assert sch.getArchivePolicy("tableReadOnly") == ArchivePolicy.EVERY_EVENT


@pytest.mark.parametrize(
    "Schema, Encoding, AccessLevel",
    [(karathon.Schema, karathon.Encoding, karathon.AccessLevel),
     (karabind.Schema, karabind.Encoding, karabind.AccessLevel)])
def test_imagedata_element(Schema, Encoding, AccessLevel):
    sch = Schema()
    SomeClass.expectedParameters(sch)
    h = sch.getParameterHash()
    assert sch.has("imageSetDims") is True
    assert sch.getDisplayedName("imageSetDims") == "Image setDimensions"
    assert (sch.getDescription("imageSetDims") ==
            "ImageData element: setDimensions")
    assert h.getAttribute('imageSetDims.dims', 'defaultValue') == [110, 210]
    assert h.getAttribute('imageSetDims.encoding',
                          'defaultValue') == Encoding.GRAY
    assert sch.getRequiredAccessLevel("imageSetDims") == AccessLevel.OPERATOR

    assert sch.has("imageSetType") is True
    assert sch.getRequiredAccessLevel("imageSetType") == AccessLevel.OBSERVER
    assert h.getAttribute('imageSetType.dims', 'defaultValue') == [110, 210]
    assert h.getAttribute('imageSetType.encoding',
                          'defaultValue') == Encoding.GRAY

    assert sch.has("imageSetEncoding") is True
    assert sch.getRequiredAccessLevel("imageSetEncoding") == AccessLevel.USER
    assert h.getAttribute('imageSetEncoding.dims',
                          'defaultValue') == [300, 200]
    assert h.getAttribute('imageSetEncoding.encoding',
                          'defaultValue') == Encoding.RGBA

    assert sch.has("imageSetGeometry") is True
    assert sch.getRequiredAccessLevel("imageSetGeometry") == \
        AccessLevel.EXPERT

    assert sch.has("myImageElement") is True
    assert h.getAttribute("myImageElement.dims",
                          'defaultValue') == [500, 800]
    assert h.getAttribute("myImageElement.dimScales",
                          'defaultValue') == "Dimscales"
    assert sch.getRequiredAccessLevel("myImageElement") == AccessLevel.ADMIN
