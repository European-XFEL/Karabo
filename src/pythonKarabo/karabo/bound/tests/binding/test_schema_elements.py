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
import copy

import numpy as np
import pytest

from karabo.bound import (
    ALARM_ELEMENT, BOOL_ELEMENT, BYTEARRAY_ELEMENT, CHOICE_ELEMENT,
    DOUBLE_ELEMENT, EVERY_1S, EVERY_100MS, EVERY_EVENT, FLOAT_ELEMENT,
    IMAGEDATA_ELEMENT, INT32_ELEMENT, INT64_ELEMENT, KARABO_CLASSINFO,
    KARABO_CONFIGURATION_BASE_CLASS, METER, MICRO, NDARRAY_ELEMENT,
    NO_ARCHIVING, NODE_ELEMENT, OVERWRITE_ELEMENT, SLOT_ELEMENT, STATE_ELEMENT,
    STRING_ELEMENT, TABLE_ELEMENT, UINT32_ELEMENT, UINT64_ELEMENT,
    VECTOR_BOOL_ELEMENT, VECTOR_DOUBLE_ELEMENT, VECTOR_INT32_ELEMENT,
    VECTOR_STRING_ELEMENT, VECTOR_UINT32_ELEMENT, AccessLevel, AccessType,
    ArchivePolicy, AssemblyRules, AssignmentType, Encoding, Hash, MetricPrefix,
    NodeType, Schema, Types, Unit, cppNDArray, cppNDArrayCopy, fullyEqual)
from karabo.common.alarm_conditions import AlarmCondition
from karabo.common.states import State


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("ShapeX", "1.0")
class ShapeX:
    @staticmethod
    def expectedParameters(expected):
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
            .defaultValue(50)
            .init()
            .commit(),

            STATE_ELEMENT(expected).key("state")
            .options(State.OFF, State.ON, State.ERROR)
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
            .setNowOperatorAccess()
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
            .defaultValue(50)
            .expertAccess()
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
            .defaultValue(50)
            .init()
            .commit(),
        )


@KARABO_CLASSINFO("EditableRectangleX", "1.0")
class EditableRectangleX(RectangleX):
    @staticmethod
    def expectedParameters(expected):
        RectangleX.expectedParameters(expected)
        (
            OVERWRITE_ELEMENT(expected).key("a")
            .setNewAssignmentInternal()
            .setNewMinExc(30)
            .setNewMaxInc(60)
            .setNowExpertAccess()
            .commit(),

            OVERWRITE_ELEMENT(expected).key("b")
            .setNowReadOnly().setNewDefaultValue(12)
            .setNowOperatorAccess()
            .commit(),

        )


class GraphicsRendererX1:
    @staticmethod
    def expectedParameters(expected):
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
        (
            STRING_ELEMENT(expected)
            .key("exampleKey1")
            .tags("hardware, poll")
            .displayedName("Example key 1")
            .description("Example key 1 description")
            .options("Radio,Air Condition,Navigation", ",")
            .assignmentOptional()
            .defaultValue("Navigation")
            .operatorAccess()
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
            .expertAccess()
            .minExc(-2.22)
            .maxExc(5.55)
            .assignmentInternal()
            .noDefaultValue()
            .commit(),
        )

        (
            # Always readOnly(), by default as well
            BYTEARRAY_ELEMENT(expected)
            .key("rarray")
            .alias("aliasReadArray")
            .tags("software")
            .displayedName("Example read array")
            .description("Example of ByteArray for reading")
            .operatorAccess()
            .readOnly()
            .initialValue(bytes('abcdef привет 012345', 'u8'))
            .commit(),

            BYTEARRAY_ELEMENT(expected)
            .key("rarray1")
            .displayedName("ReadArrayStr")
            .expertAccess()
            .readOnly()
            .initialValue('hello world')
            .commit(),

            BYTEARRAY_ELEMENT(expected)
            .key("rarray2")
            .displayedName("ReadByteArray")
            .observerAccess()
            .readOnly()
            .initialValue(bytearray('Tschüß!', 'u8'))
            .commit(),
        )

        (
            INT64_ELEMENT(expected)
            .key("exampleKey5")
            .alias("exampleAlias5")
            .tags("h/w; d.m.y", ";")
            .displayedName("Example key 5")
            .description("Example key 5 description")
            .setSpecialDisplayType("Int64DisplayType")
            .readOnly()
            .initialValue(1442244)
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

            STRING_ELEMENT(expected)
            .key("testPath")
            .alias(5)
            .displayedName("Filename")
            .options("file1, file2, karabo.log")
            .assignmentOptional()
            .defaultValue("karabo.log")
            .reconfigurable()
            .commit(),

            STRING_ELEMENT(expected)
            .key("testPath2")
            .readOnly()
            .archivePolicy(EVERY_1S)
            .commit(),

            STRING_ELEMENT(expected)
            .key("testPath3")
            .assignmentMandatory()
            .commit(),

            SLOT_ELEMENT(expected)
            .key("slotTest")
            .alias(0x12345678)
            .tags("hard, soft")  # [...] & (...) supported only in karabind
            .displayedName("Reset")
            .description("Test slot element")
            .allowedStates(State.STARTED, State.STOPPED, State.NORMAL)
            .operatorAccess()
            .commit(),

            SLOT_ELEMENT(expected)
            .key("slotClean")
            .alias("aliasClean")
            .displayedName("Clean")
            .allowedStates(State.STARTED, State.STOPPED, State.NORMAL)
            .observerAccess()
            .commit(),

            SLOT_ELEMENT(expected)
            .key("slotStart")
            .alias("aliasStart")
            .tags('hard; soft')
            .displayedName("Start")
            .allowedStates(State.STARTED, State.STOPPED, State.NORMAL)
            .expertAccess()
            .commit(),

            SLOT_ELEMENT(expected)
            .key("slotStop")
            .alias("aliasStop")
            .displayedName("Stop")
            .allowedStates(State.STARTED, State.STOPPED, State.NORMAL)
            .expertAccess()
            .commit(),

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
        (
            SLOT_ELEMENT(expected)
            .key("slotTest")
            .displayedName("Reset")
            .description("Test slot element")
            .allowedStates(State.STARTED, State.STOPPED, State.ERROR)
            .commit(),

            STRING_ELEMENT(expected)
            .description("File name")
            .key("filename")
            .alias(5)
            .displayedName("Filename")
            .options("file1, file2, karabo.log")
            .assignmentOptional()
            .defaultValue("karabo.log")
            .reconfigurable()
            .commit(),

            STRING_ELEMENT(expected)
            .key("testfile")
            # .isInputFile()
            .readOnly()
            .defaultValue("initFile")  # == initialValue
            .commit(),

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
            .expertAccess()
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
            .operatorAccess()
            .assignmentInternal()
            .noDefaultValue()
            .commit(),

            TABLE_ELEMENT(expected)
            .key("tableReadOnly")
            .alias("aliasForTableReadOnly")
            .tags("exp,hard,soft")
            .setColumns(rowSchema)
            .allowedStates(State.STARTED, State.STOPPED)
            .operatorAccess()
            .readOnly()
            .initialValue([Hash("a", 22, "b", "foo22", "c", 1.234),
                           Hash("a", 33, "b", "foo33", "c", 2.345),
                           Hash("a", 44, "b", "foo44", "c", 3.456)])
            .archivePolicy(EVERY_EVENT)
            .commit(),
        )


class SomeClass:
    @staticmethod
    def expectedParameters(expected):
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
            .operatorAccess()
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
            .expertAccess()
            .commit(),
        )


def test_buildUp():
    schema = ShapeX.getSchema("CircleX", AssemblyRules())
    states = [State.OFF.value, State.ON.value, State.ERROR.value]
    assert schema.getOptions("state") == states
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
    assert schema.getRequiredAccessLevel("radius") == AccessLevel.OPERATOR

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
    assert schema.getRequiredAccessLevel("alarm") == AccessLevel.OPERATOR

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
    assert schema.getRequiredAccessLevel("a") == AccessLevel.EXPERT
    assert schema.getRequiredAccessLevel("b") == AccessLevel.OPERATOR

    schema = Schema()
    GraphicsRendererX1.expectedParameters(schema)
    assert schema.isAccessInitOnly("shapes.circle.radius") is True
    assert schema.isLeaf("shapes.circle.radius") is True


def test_getRootName():
    schema = Schema("MyTest")
    TestStruct1.expectedParameters(schema)
    assert schema.getRootName() == "MyTest"


def test_getTags():
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
    assert schema.getTags("rarray")[0] == "software"


def test_setTags():
    schema = Schema()
    SomeClass.expectedParameters(schema)
    assert schema.hasTags('x') is True
    assert schema.getTags('x') == ['IK', 'BH']
    schema.setTags('x', 'CY,SE')
    assert schema.getTags('x') == ['CY', 'SE']


def test_getsetAccessLevel():
    sch = Schema()
    SomeClass.expectedParameters(sch)
    assert sch.getRequiredAccessLevel('x') == AccessLevel.EXPERT
    assert sch.getRequiredAccessLevel('y') == AccessLevel.OPERATOR
    assert sch.getRequiredAccessLevel('a') == AccessLevel.OBSERVER

    sch.setRequiredAccessLevel('x', AccessLevel.EXPERT)
    sch.setRequiredAccessLevel('y', AccessLevel.OPERATOR)
    assert sch.getRequiredAccessLevel('x') == AccessLevel.EXPERT
    assert sch.getRequiredAccessLevel('y') == AccessLevel.OPERATOR

    sch = Schema()
    TestStruct1.expectedParameters(sch)
    assert sch.getRequiredAccessLevel('exampleKey1') == AccessLevel.OPERATOR
    assert sch.getRequiredAccessLevel('exampleKey2') == AccessLevel.OPERATOR
    assert sch.getRequiredAccessLevel('exampleKey3') == AccessLevel.EXPERT
    assert sch.getRequiredAccessLevel('exampleKey4') == AccessLevel.EXPERT
    # default for readOnly
    assert sch.getRequiredAccessLevel('exampleKey5') == AccessLevel.OBSERVER
    # default for reconfigurable
    assert sch.getRequiredAccessLevel('exampleKey10') == AccessLevel.OPERATOR
    # observerAccess in reconfigurable
    assert sch.getRequiredAccessLevel('exampleKey11') == AccessLevel.OBSERVER
    assert sch.getRequiredAccessLevel('rarray') == AccessLevel.OPERATOR


def test_isNode():
    schema = Schema("test")
    GraphicsRendererX1.expectedParameters(schema)
    assert schema.getRootName() == "test"
    assert schema.isNode("shapes.circle") is True
    assert schema.isNode("shapes.rectangle") is True
    assert schema.isNode("shapes") is False


def test_getNodeType():
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


def test_getValueType():
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
    assert schema.getValueType("rarray") == Types.BYTE_ARRAY


def test_getAliasAsString():
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getAliasAsString("exampleKey2") == "10"
    assert schema.getAliasAsString("exampleKey3") == "5.5"
    assert schema.getAliasAsString("exampleKey4") == "exampleAlias4"
    assert schema.getAliasAsString("exampleKey5") == "exampleAlias5"
    assert schema.getAliasAsString("exampleKey6") == "1193046,43724"
    assert schema.getAliasAsString("testPath") == "5"
    assert schema.getAliasAsString("rarray") == "aliasReadArray"


def test_keyHasAlias():
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.keyHasAlias("exampleKey1") is False
    assert schema.keyHasAlias("exampleKey2") is True
    assert schema.keyHasAlias("exampleKey3") is True
    assert schema.keyHasAlias("exampleKey4") is True
    assert schema.keyHasAlias("exampleKey5") is True
    assert schema.keyHasAlias("exampleKey6") is True
    assert schema.keyHasAlias("testPath") is True
    assert schema.keyHasAlias("rarray") is True


def test_aliasHasKey():
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.aliasHasKey(10) is True
    assert schema.aliasHasKey(5.5) is True
    assert schema.aliasHasKey("exampleAlias4") is True
    assert schema.aliasHasKey("exampleAlias5") is True
    assert schema.aliasHasKey([0x00123456, 0x0000aacc]) is True
    assert schema.aliasHasKey(7) is False
    assert schema.aliasHasKey(5) is True
    assert schema.aliasHasKey("aliasReadArray") is True


def test_getAliasFromKey():
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getAliasFromKey("exampleKey2") == 10
    assert schema.getAliasFromKey("exampleKey3") == 5.5
    assert schema.getAliasFromKey("exampleKey4") == "exampleAlias4"
    assert schema.getAliasFromKey("exampleKey5") == "exampleAlias5"
    assert schema.getAliasFromKey("exampleKey6") == [0x00123456, 0x0000aacc]
    assert schema.getAliasFromKey("testPath") == 5
    assert schema.getAliasFromKey("rarray") == "aliasReadArray"


def test_setAlias():
    schema = Schema()
    SomeClass.expectedParameters(schema)
    assert schema.getAliasFromKey("x") == 10
    schema.setAlias('x', 'abc')
    assert schema.getAliasFromKey("x") == 'abc'
    schema.setAlias('x', 99)
    assert schema.getAliasFromKey("x") == 99


def test_getKeyFromAlias():
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getKeyFromAlias(10) == "exampleKey2"
    assert schema.getKeyFromAlias(5.5) == "exampleKey3"
    assert schema.getKeyFromAlias("exampleAlias4") == "exampleKey4"
    assert schema.getKeyFromAlias("exampleAlias5") == "exampleKey5"
    assert schema.getKeyFromAlias([0x00123456, 0x0000aacc]) == "exampleKey6"
    assert schema.getKeyFromAlias(5) == "testPath"
    assert schema.getKeyFromAlias("aliasReadArray") == "rarray"


def test_getAccessMode():
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
    assert schema.getAccessMode("rarray") == AccessType.READ


def test_getAssignment():
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


def test_setAssignment():
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


def test_getDescription():
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getDescription('exampleKey1') == "Example key 1 description"
    assert schema.getDescription('rarray') == \
        "Example of ByteArray for reading"


def test_setDescription():
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    schema.setDescription('exampleKey1', "No description")
    assert schema.getDescription('exampleKey1') == "No description"


def test_getOptions():
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


def test_setOptions():
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


def test_getDefaultValue():
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
    assert schema.getDefaultValue("rarray") == \
        b'abcdef \xd0\xbf\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82 012345'
    assert schema.getDefaultValue("rarray1") == b'hello world'
    assert schema.getDefaultValue("rarray2") == b'Tsch\xc3\xbc\xc3\x9f!'


def test_setDefaultValue():
    schema = Schema()
    SomeClass.expectedParameters(schema)
    assert schema.isAssignmentOptional('x') is True
    assert schema.hasDefaultValue('x') is True
    assert schema.getDefaultValue("x") == 5
    schema.setDefaultValue("x", 10)
    assert schema.getDefaultValue("x") == 10


def test_getAllowedStates():
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    astates = schema.getAllowedStates("exampleKey3")
    assert astates[0] == State.STARTED
    assert astates[1] == State.STOPPED
    assert astates[2] == State.NORMAL
    assert schema.getAllowedStates("exampleKey3")[2] == State.NORMAL
    assert schema.getAllowedStates("exampleKey7")[0] == State.STARTED
    assert schema.getAllowedStates("exampleKey7")[1] == State.NORMAL


def test_getUnit():
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getUnit("exampleKey2") == Unit.METER
    assert schema.getUnitName("exampleKey2") == "meter"
    assert schema.getUnitSymbol("exampleKey2") == "m"


def test_setUnit():
    schema = Schema()
    SomeClass.expectedParameters(schema)
    assert schema.getUnit("x") == Unit.AMPERE
    schema.setUnit('x', METER)
    assert schema.getUnit("x") == METER
    assert schema.getUnit("x") == Unit.METER
    assert schema.getUnitName("x") == "meter"
    assert schema.getUnitSymbol("x") == "m"


def test_getMetricPrefix():
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getMetricPrefix("exampleKey2") == MetricPrefix.MILLI
    assert schema.getMetricPrefixName("exampleKey2") == "milli"
    assert schema.getMetricPrefixSymbol("exampleKey2") == "m"


def test_setMetricPrefix():
    schema = Schema()
    SomeClass.expectedParameters(schema)
    assert schema.getMetricPrefix("x") == MetricPrefix.MILLI
    schema.setMetricPrefix("x", MetricPrefix.MICRO)
    assert schema.getMetricPrefix("x") == MICRO
    assert schema.getMetricPrefixName("x") == "micro"
    assert schema.getMetricPrefixSymbol("x") == "u"


def test_getMinIncMaxInc():
    schema = Schema()
    TestStruct1.expectedParameters(schema)
    assert schema.getMinInc("exampleKey2") == 5
    assert schema.getMinIncAs("exampleKey2", Types.STRING) == "5"
    assert schema.getMaxInc("exampleKey2") == 25
    assert schema.getMaxIncAs("exampleKey2", Types.STRING) == "25"


def test_setMinIncMaxInc():
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


def test_getMinExcMaxExc():
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


def test_setMinExcMaxExc():
    schema = Schema()
    SomeClass.expectedParameters(schema)
    assert schema.getMinExc("y") == 0
    assert schema.getMaxExc("y") == 29
    schema.setMinExc("y", 2)
    schema.setMaxExc("y", 30)
    assert schema.getMinExc("y") == 2
    assert schema.getMaxExc("y") == 30


def test_hasgetsetMinMaxSize():
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


def test_vectorElement():
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


def test_table_element():
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


def test_overwrite_restrictions_for_options():
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


def test_overwrite_tags():
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


def test_choice_element():
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


def test_table_elements():
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
            AccessLevel.EXPERT)

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
    assert sch.getRequiredAccessLevel("tableInternal") == AccessLevel.OPERATOR

    assert sch.has("tableReadOnly") is True
    assert sch.getAliasFromKey("tableReadOnly") == "aliasForTableReadOnly"
    assert sch.getTags("tableReadOnly") == ['exp', 'hard', 'soft']
    assert sch.isAssignmentOptional("tableReadOnly") is True
    assert sch.isAccessReadOnly("tableReadOnly") is True
    assert sch.hasDefaultValue("tableReadOnly") is True
    assert sch.hasAllowedStates("tableReadOnly") is True
    assert (sch.getAllowedStates("tableReadOnly") ==
            [State.STARTED, State.STOPPED])
    assert sch.getRequiredAccessLevel("tableReadOnly") == AccessLevel.OPERATOR
    assert sch.hasArchivePolicy("tableReadOnly") is True
    assert sch.getArchivePolicy("tableReadOnly") == ArchivePolicy.EVERY_EVENT


def test_imagedata_element():
    sch = Schema()
    SomeClass.expectedParameters(sch)
    h = sch.getParameterHash()
    assert sch.has("imageSetDims") is True
    assert sch.getDisplayedName("imageSetDims") == "Image setDimensions"
    assert (sch.getDescription("imageSetDims") ==
            "ImageData element: setDimensions")
    assert h.getAttribute('imageSetDims.dims', 'defaultValue') == [110, 210]
    assert h.getAttribute('imageSetDims.encoding',
                          'defaultValue') == Encoding.GRAY.value
    assert sch.getRequiredAccessLevel("imageSetDims") == AccessLevel.OPERATOR

    assert sch.has("imageSetType") is True
    assert sch.getRequiredAccessLevel("imageSetType") == AccessLevel.OBSERVER
    assert h.getAttribute('imageSetType.dims', 'defaultValue') == [110, 210]
    assert h.getAttribute('imageSetType.encoding',
                          'defaultValue') == Encoding.GRAY.value

    assert sch.has("imageSetEncoding") is True
    assert sch.getRequiredAccessLevel("imageSetEncoding") == \
        AccessLevel.OPERATOR
    assert h.getAttribute('imageSetEncoding.dims',
                          'defaultValue') == [300, 200]
    assert h.getAttribute('imageSetEncoding.encoding',
                          'defaultValue') == Encoding.RGBA.value

    assert sch.has("imageSetGeometry") is True
    assert sch.getRequiredAccessLevel("imageSetGeometry") == \
        AccessLevel.EXPERT

    assert sch.has("myImageElement") is True
    assert h.getAttribute("myImageElement.dims",
                          'defaultValue') == [500, 800]
    assert h.getAttribute("myImageElement.dimScales",
                          'defaultValue') == "Dimscales"
    assert sch.getRequiredAccessLevel("myImageElement") == AccessLevel.EXPERT


def test_slot_element():
    sch = Schema()
    TestStruct1.expectedParameters(sch)
    assert sch.has("slotTest") is True
    assert sch.getAliasFromKey("slotTest") == 0x12345678
    assert sch.isCommand("slotTest") is True
    assert sch.getDisplayedName("slotTest") == "Reset"
    assert sch.getTags("slotTest") == ['hard', 'soft']
    assert sch.getDescription("slotTest") == "Test slot element"
    assert sch.getAllowedStates("slotTest") == [
        State.STARTED, State.STOPPED, State.NORMAL]
    assert sch.getRequiredAccessLevel("slotTest") == AccessLevel.OPERATOR
    assert sch.has("slotClean") is True
    assert sch.getAliasFromKey("slotClean") == "aliasClean"
    assert sch.isCommand("slotClean") is True
    assert sch.getDisplayedName("slotClean") == "Clean"
    assert sch.getRequiredAccessLevel("slotClean") == AccessLevel.OBSERVER
    assert sch.has("slotStart") is True
    assert sch.getAliasFromKey("slotStart") == "aliasStart"
    assert sch.getTags("slotStart") == ['hard', 'soft']
    assert sch.isCommand("slotStart") is True
    assert sch.getDisplayedName("slotStart") == "Start"
    assert sch.getRequiredAccessLevel("slotStart") == AccessLevel.EXPERT
    assert sch.has("slotStop") is True
    assert sch.getAliasFromKey("slotStop") == "aliasStop"
    assert sch.isCommand("slotStop") is True
    assert sch.getDisplayedName("slotStop") == "Stop"
    assert sch.getRequiredAccessLevel("slotStop") == AccessLevel.EXPERT


def test_allowed_actions():
    s = Schema()
    (
        NODE_ELEMENT(s).key("node")
        .setAllowedActions(("action1", "action2"))  # tuple
        .commit(),

        INT32_ELEMENT(s).key("node.int")
        .assignmentMandatory()
        .commit(),

        NDARRAY_ELEMENT(s).key("node.arr")
        .setAllowedActions(["otherAction"])  # list
        .commit(),

        IMAGEDATA_ELEMENT(s).key("image")
        .setAllowedActions("")  # str - each char is taken, here none
        .commit(),
    )

    assert s.hasAllowedActions("node") is True
    assert s.hasAllowedActions("node.int") is False
    actions = s.getAllowedActions("node")
    assert len(actions) == 2
    assert actions[0] == "action1"
    assert actions[1] == "action2"

    assert s.hasAllowedActions("node.arr") is True
    actions = s.getAllowedActions("node.arr")
    assert len(actions) == 1
    assert actions[0] == "otherAction"

    assert s.hasAllowedActions("image") is True
    actions = s.getAllowedActions("image")
    assert len(actions) == 0

    # Check setAllowedActions from Schema
    # and validate that also keys of a dict are taken:
    s.setAllowedActions("node",
                        {"actA": 1, "actB": 2, "actC": "who care"})
    actions = s.getAllowedActions("node")
    assert len(actions) == 3
    assert actions[0] == "actA"
    assert actions[1] == "actB"
    assert actions[2] == "actC"

    # Only (custom) nodes can have allowed actions:
    with pytest.raises(RuntimeError):
        s.setAllowedActions("node.int", ["bla", "blue"])


def test_cpp_ndarray():
    # Build numpy arrays on C++ side and convert them to the python objects
    # using commented binding code below  (see "karabind_test.cc")
    # Conversions happen at the last step...

    # m.def("cppNDArray", []() {
    #     const Dims shape(3, 4);
    #     std::vector<int> someData(3 * 4, 7);
    #     for (int i = 0; i < 3; ++i) someData[i] = 100 + i;
    #     NDArray nda(someData.begin(), someData.end(), shape);
    #     return karabind::wrapper::castNDArrayToPy(nda);
    # });
    # m.def("cppNDArrayCopy", []() {
    #     const Dims shape(3, 4);
    #    std::vector<int> someData(3 * 4, 7);
    #    for (int i = 0; i < 3; ++i) someData[i] = 100 + i;
    #    NDArray nda(someData.begin(), someData.end(), shape);
    #    return karabind::wrapper::copyNDArrayToPy(nda);
    # });

    # create py::array (numpy.array) objects
    a = cppNDArray()
    b = cppNDArrayCopy()
    # These binding return the same data
    assert np.all(a == b)
    # The only difference between them is data ownership ...
    # C++ is an owner of data array
    assert a.flags.owndata is False
    assert a.base is not None
    # Python is an owner of data array
    assert b.flags.owndata is True
    assert b.base is None
    assert np.all(a == b)
    # this works like 'deepcopy'
    c = a.copy()
    assert c.flags.owndata is True
    assert c.base is None
    # or use deepcopy directly ..
    d = copy.deepcopy(a)
    assert d.flags.owndata is True
    assert d.base is None
    # and copy works like deepcopy
    e = copy.copy(a)
    assert e.flags.owndata is True
    assert e.base is None
