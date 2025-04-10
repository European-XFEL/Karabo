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

from karabo.bound import (
    ALARM_ELEMENT, AMPERE, BOOL_ELEMENT, CENTI, CHOICE_ELEMENT, DOUBLE_ELEMENT,
    EVERY_1S, EVERY_100MS, EVERY_EVENT, FLOAT_ELEMENT, IMAGEDATA_ELEMENT,
    INT32_ELEMENT, INT64_ELEMENT, KARABO_CLASSINFO,
    KARABO_CONFIGURATION_BASE_CLASS, METER, MILLI, NDARRAY_ELEMENT,
    NO_ARCHIVING, NODE_ELEMENT, OVERWRITE_ELEMENT, SLOT_ELEMENT, STATE_ELEMENT,
    STRING_ELEMENT, TABLE_ELEMENT, UINT32_ELEMENT, UINT64_ELEMENT,
    VECTOR_BOOL_ELEMENT, VECTOR_DOUBLE_ELEMENT, VECTOR_INT32_ELEMENT,
    VECTOR_STRING_ELEMENT, AlarmCondition, Hash, MetricPrefix, Schema, Unit)
from karabo.common.states import State


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("Shape", "1.0")
class Shape:
    def __init__(self, configuration):
        self.configuration = configuration

    @staticmethod
    def expectedParameters(expected):
        (
            BOOL_ELEMENT(expected).key("shadowEnabled")
            .description("Shadow enabled")
            .displayedName("Shadow")
            .assignmentOptional().defaultValue(False)
            .init()
            .commit(),
        )

    def getConfiguration(self):
        return self.configuration

    def draw(self):
        pass


@KARABO_CLASSINFO("Circle", "1.0")
class Circle(Shape):
    def __init__(self, configuration):
        super().__init__(configuration)

    @staticmethod
    def expectedParameters(expected):
        (
            DOUBLE_ELEMENT(expected).key("radius").alias(1)
            .description("The radius of the circle")
            .displayedName("Radius")
            .minExc(0).maxExc(100)
            .unit(METER)
            .metricPrefix(MILLI)
            .assignmentOptional().defaultValue(10)
            .init()
            .commit(),

            STATE_ELEMENT(expected).key("state")
            .commit(),

            STATE_ELEMENT(expected).key("stateN")
            .defaultValue(State.NORMAL)
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
            .defaultValue(AlarmCondition.ALARM)
            .commit(),

            STRING_ELEMENT(expected).key("status")
            .readOnly()
            .commit()
        )

    def draw(self):
        return self.__class__.__name__


'''
Editable Circle
'''


@KARABO_CLASSINFO("EditableCircle", "1.0")
class EditableCircle(Circle):
    def __init__(self, configuration):
        super().__init__(configuration)

    @staticmethod
    def expectedParameters(expected):
        (
            OVERWRITE_ELEMENT(expected)
            .key("radius")
            .setNowReconfigurable()
            .commit(),

            OVERWRITE_ELEMENT(expected)
            .key("state")
            .setNewOptions(State.INIT, State.ERROR, State.NORMAL)
            .setNewDefaultValue(State.INIT)
            .commit(),

            OVERWRITE_ELEMENT(expected)
            .key("status")
            .setNewOptions("a,b,c")
            .setNewDefaultValue("a")
            .commit()
        )

    def draw(self):
        return self.__class__.__name__


'''
Rectangle
'''


@KARABO_CLASSINFO("Rectangle", "1.0")
class Rectangle(Shape):
    def __init__(self, configuration):
        super().__init__(configuration)
        # print "Rectangle.__init__"

    @staticmethod
    def expectedParameters(expected):
        (
            DOUBLE_ELEMENT(expected).key("a")
            .alias(1)
            .description("Length of a")
            .displayedName("A")
            .minExc(0).maxExc(100)
            .unit(Unit.METER)
            .metricPrefix(MetricPrefix.MILLI)
            .assignmentOptional().defaultValue(10)
            .init()
            .commit(),

            DOUBLE_ELEMENT(expected).key("b").alias(1)
            .description("Length of b")
            .displayedName("B")
            .minExc(0).maxExc(100)
            .unit(Unit.METER)
            .metricPrefix(MetricPrefix.MILLI)
            .assignmentOptional().defaultValue(10)
            .init()
            .commit(),
        )

    def draw(self):
        return self.__class__.__name


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("GraphicsRenderer", "1.0")
class GraphicsRenderer:
    def __init__(self, input):
        shape = Shape.createChoice("shapes", input)
        if "shapes.Circle" in input:
            assert shape.draw() == "Circle"

    @staticmethod
    def expectedParameters(expected):
        (
            BOOL_ELEMENT(expected).key("antiAlias")
            .tags("prop")
            .displayedName("Use Anti-Aliasing")
            .description("You may switch of for speed")
            .assignmentOptional().defaultValue(True)
            .init()
            .expertAccess()
            .commit(),

            STRING_ELEMENT(expected).key("color")
            .tags("prop")
            .displayedName("Color")
            .options("red,green,blue,orange,black")
            .description("The default color for any shape")
            .assignmentOptional().defaultValue("red")
            .reconfigurable()
            .commit(),

            BOOL_ELEMENT(expected).key("bold")
            .tags("prop")
            .displayedName("Bold")
            .description("Toggles bold painting")
            .assignmentOptional().defaultValue(False)
            .reconfigurable()
            .commit(),

            CHOICE_ELEMENT(expected).key("shapes")
            .description("Some shapes")
            .displayedName("Shapes")
            .appendNodesOfConfigurationBase(Shape)
            .assignmentOptional().defaultValue("Rectangle")
            .commit(),
        )


@KARABO_CLASSINFO("GraphicsRenderer1", "1.0")
class GraphicsRenderer1:
    @staticmethod
    def expectedParameters(expected):
        (
            BOOL_ELEMENT(expected).key("antiAlias")
            .tags("prop")
            .displayedName("Use Anti-Aliasing")
            .description("You may switch of for speed")
            .assignmentOptional().defaultValue(True)
            .init()
            .expertAccess()
            .commit(),

            STRING_ELEMENT(expected).key("color")
            .tags("prop")
            .displayedName("Color")
            .description("The default color for any shape")
            .assignmentOptional().defaultValue("red")
            .reconfigurable()
            .commit(),

            BOOL_ELEMENT(expected).key("bold")
            .tags("prop")
            .displayedName("Bold")
            .description("Toggles bold painting")
            .assignmentOptional()
            .defaultValue(False)
            .reconfigurable()
            .commit(),

            CHOICE_ELEMENT(expected).key("shapes")
            .assignmentOptional().defaultValue("circle")
            .commit(),

            NODE_ELEMENT(expected).key("shapes.circle")
            .tags("shape")
            .displayedName("Circle")
            .description("A circle")
            .appendParametersOf(Circle)
            .commit(),

            NODE_ELEMENT(expected).key("shapes.rectangle")
            .tags("shape")
            .displayedName("Rectangle")
            .description("A rectangle")
            .commit(),

            DOUBLE_ELEMENT(expected).key("shapes.rectangle.b")
            .description("Rectangle side - b")
            .displayedName("Side B")
            .assignmentOptional().defaultValue(10)
            .init()
            .commit(),

            DOUBLE_ELEMENT(expected).key("shapes.rectangle.c")
            .description("Rectangle side - c")
            .displayedName("Side C")
            .assignmentOptional().defaultValue(10)
            .init()
            .commit(),

            NODE_ELEMENT(expected).key("triangle")
            .displayedName("triangle")
            .description("A triangle (Node element containing no "
                         "other elements)")
            .commit(),
        )


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("TestStruct1", "1.0")
class TestStruct1:
    @staticmethod
    def expectedParameters(expected):
        (
            STRING_ELEMENT(expected).key("exampleKey1")
            .tags("hardware, poll")
            .displayedName("Example key 1")
            .description("Example key 1 description")
            .options("Radio,Air Condition,Navigation", ",")
            .assignmentOptional().defaultValue("Navigation")
            .reconfigurable()
            .operatorAccess()
            .commit(),

            INT32_ELEMENT(expected).key("exampleKey2")
            .alias(10)
            .tags("hardware, poll")
            .displayedName("Example key 2")
            .description("Example key 2 description")
            .options("5, 25, 10")
            .minInc(5).maxInc(25)
            .unit(METER)
            .metricPrefix(MILLI)
            .assignmentOptional().defaultValue(10)
            .init()
            .operatorAccess()
            .commit(),

            UINT32_ELEMENT(expected).key("exampleKey3")
            .alias(5.5)
            .tags("hardware, set")
            .displayedName("Example key 3")
            .description("Example key 3 description")
            .allowedStates(State.STARTED, State.STOPPED, State.NORMAL)
            .minExc(10).maxExc(20)
            .assignmentMandatory()
            .expertAccess()
            .reconfigurable()
            .commit(),

            DOUBLE_ELEMENT(expected).key("exampleKey4")
            .alias("exampleAlias4")
            .tags("software")
            .displayedName("Example key 4")
            .description("Example key 4 description")
            .options("1.11     -2.22 5.55")
            .minExc(-2.22).maxExc(5.55)
            .assignmentInternal().noDefaultValue()
            .expertAccess()
            .commit(),

            INT64_ELEMENT(expected).key("exampleKey5")
            .alias("exampleAlias5")
            .tags("h/w; d.m.y", ";")
            .displayedName("Example key 5")
            .description("Example key 5 description")
            .setSpecialDisplayType("Int64DisplayType")
            .readOnly().initialValue(1442244)
            .archivePolicy(EVERY_EVENT)
            .commit(),

            INT64_ELEMENT(expected).key("exampleKey5b")
            .readOnly().defaultValue(42)
            .commit(),

            DOUBLE_ELEMENT(expected).key("exampleKey6")
            .alias([0x00123456, 0x0000aacc])
            .displayedName("Example key 6")
            .description("Example key 6 description")
            .readOnly().initialValue(1.11)
            .archivePolicy(EVERY_100MS)
            .commit(),

            VECTOR_INT32_ELEMENT(expected).key("exampleKey7")
            .displayedName("Example key 7")
            .allowedStates(State.STARTED, State.NORMAL)
            .readOnly().initialValue([1, 2, 3])
            .archivePolicy(EVERY_1S)
            .commit(),

            VECTOR_INT32_ELEMENT(expected).key("exampleKey7b")
            .readOnly().initialValue([11, 22, 33])
            .commit(),

            VECTOR_DOUBLE_ELEMENT(expected).key("exampleKey8")
            .readOnly().initialValue([1.1, 2.2, 3.3])
            .archivePolicy(NO_ARCHIVING)
            .commit(),

            VECTOR_STRING_ELEMENT(expected).key("exampleKey9")
            .setSpecialDisplayType("TestDisplayType")
            .readOnly().initialValue(["Hallo", "World"])
            .commit(),

            VECTOR_INT32_ELEMENT(expected).key("vectInt")
            .readOnly()
            .commit(),

            VECTOR_INT32_ELEMENT(expected).key("exampleKey10")
            .displayedName("Example key 10")
            .minSize(2)
            .maxSize(7)
            .assignmentOptional().defaultValueFromString("10, 20, 30")
            .reconfigurable()
            .commit(),

            VECTOR_INT32_ELEMENT(expected).key("exampleKey11")
            .displayedName("Example key 11")
            .assignmentOptional().defaultValue([10, 20, 30])
            .observerAccess()
            .reconfigurable()
            .commit(),

            VECTOR_DOUBLE_ELEMENT(expected).key("exampleKey12")
            .assignmentOptional().defaultValueFromString("1.1, -2.2, 3.3")
            .reconfigurable()
            .commit(),

            VECTOR_STRING_ELEMENT(expected).key("exampleKey14")
            .assignmentOptional().defaultValue(["Hallo", "World", "Test"])
            .reconfigurable()
            .commit(),

            VECTOR_STRING_ELEMENT(expected).key("exampleKey15")
            .assignmentOptional()
            .defaultValueFromString("word1, word2, test")
            .reconfigurable()
            .commit(),

            NDARRAY_ELEMENT(expected).key("exampleKey16")
            .dtype("UINT32")
            .shape([2, 5, 0])
            .commit(),

            INT32_ELEMENT(expected).key("exampleIntKey")
            .assignmentOptional().defaultValueFromString("20")
            .reconfigurable()
            .commit(),

            UINT32_ELEMENT(expected).key("exampleBitsKey1")
            .tags("hardware")
            .displayedName("Example bits key 1")
            .description("Example bits key 1 description")
            .reconfigurable()
            .bin()
            .assignmentOptional().defaultValue(0xdeadbeef)
            .commit(),

            UINT64_ELEMENT(expected).key("exampleBitsKey2")
            .tags("hardware")
            .displayedName("Example bits key 2")
            .description("Example bits key 2 description")
            .reconfigurable()
            .bin("10:In Error, 21:Busy, 35:HV On, 55:Crate On")
            .assignmentOptional().defaultValue(0xdeadbeefdeadface)
            .commit(),

            UINT32_ELEMENT(expected).key("exampleBitsKey3")
            .tags("hardware")
            .displayedName("Example bits key 3")
            .description("Example bits key 3 description")
            .reconfigurable()
            .oct()
            .assignmentOptional().defaultValue(0xbeefface)
            .commit(),

            UINT32_ELEMENT(expected).key("exampleBitsKey4")
            .tags("hardware")
            .displayedName("Example bits key 4")
            .description("Example bits key 4 description")
            .reconfigurable()
            .hex()
            .assignmentOptional().defaultValue(0xbeefface)
            .commit(),

            STRING_ELEMENT(expected).key("testPath")
            .alias(5)
            .displayedName("Filename")
            .options("file1, file2, karabo.log")
            .assignmentOptional().defaultValue("karabo.log")
            .reconfigurable()
            .commit(),

            STRING_ELEMENT(expected).key("testPath2")
            .readOnly()
            .commit(),

            STRING_ELEMENT(expected).key("testPath3")
            .assignmentMandatory()
            .commit(),

            SLOT_ELEMENT(expected).key("slotTest")
            .displayedName("Reset")
            .description("Test slot element")
            .allowedStates(State.STARTED, State.STOPPED, State.NORMAL)
            .commit(),

            IMAGEDATA_ELEMENT(expected).key("myImageElement")
            .displayedName("myImage")
            .description("Image Element")
            .setDimensions("100, 200")
            .operatorAccess()
            .commit(),
        )
        row = Schema()
        INT32_ELEMENT(row).key("int").readOnly().initialValue(0).commit()
        (
            TABLE_ELEMENT(expected).key("tableI")
            .setColumns(row)
            .readOnly()
            .initialValue([Hash("int", 2)])
            .commit(),

            TABLE_ELEMENT(expected).key("tableD")
            .setColumns(row)
            .readOnly()
            .defaultValue([Hash("int", 3)])
            .commit(),

            NODE_ELEMENT(expected).key("myNode")
            .displayedName("myNode")
            .description("WidgetNode Test")
            .setSpecialDisplayType("WidgetNode")
            .commit(),
        )


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("SomeClassId", "1.0")
class SomeClass:
    def __init__(self, configuration):
        super().__init__()

    @staticmethod
    def expectedParameters(expected):
        (
            INT32_ELEMENT(expected).key("x").alias(10)
            .tags("IK,BH")
            .displayedName("Xkey")
            .description("Example of X key description")
            .options("5, 25, 10")
            .minInc(5).maxInc(25).unit(AMPERE).metricPrefix(MILLI)
            .assignmentOptional().defaultValue(5)
            .init()
            .expertAccess()
            .commit(),

            INT32_ELEMENT(expected).key("y").alias('bla')
            .tags("CY")
            .displayedName("Ykey")
            .description("Example of Y key description")
            .options("5, 25, 10")
            .minExc(0).maxExc(29).unit(METER).metricPrefix(CENTI)
            .assignmentOptional().defaultValue(10)
            .init()
            .commit(),

            DOUBLE_ELEMENT(expected).key("a")
            .readOnly().initialValue(1.11)
            .archivePolicy(EVERY_100MS)
            .commit(),

            VECTOR_INT32_ELEMENT(expected).key("somelist")
            .displayedName("Ykey")
            .description("Example of Y key description")
            .reconfigurable()
            .assignmentOptional().defaultValue([])
            .commit(),

            IMAGEDATA_ELEMENT(expected).key("myImageElement")
            .displayedName("myImage")
            .description("Image Element")
            .setDimensions([110, 210])
            .operatorAccess()
            .commit(),
        )


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("Base", "1.0")
class Base:
    def __init__(self, configuration):
        super().__init__()


@KARABO_CLASSINFO("P1", "1.0")
class P1(Base):
    def __init__(self, configuration):
        super().__init__(configuration)

    @staticmethod
    def expectedParameters(expected):
        (
            STRING_ELEMENT(expected).key("a")
            .description("a").displayedName("a")
            .assignmentOptional().defaultValue("a value")
            .tags("CY,CY,NC,JS,KW,NC")
            .commit(),

            STRING_ELEMENT(expected).key("b")
            .tags("BH,CY")
            .displayedName("Example key 1")
            .description("Example key 1 description")
            .options("Radio,Air Condition,Navigation", ",")
            .assignmentOptional().defaultValue("Air Condition")
            .reconfigurable()
            .commit(),

            INT32_ELEMENT(expected).key("c").alias(10)
            .tags("BH")
            .displayedName("Example key 2")
            .description("Example key 2 description")
            .options("5, 25, 10")
            .minInc(5).maxInc(25).unit(METER).metricPrefix(MILLI)
            .assignmentOptional().defaultValue(5)
            .init()
            .commit(),

            UINT32_ELEMENT(expected).key("d").alias(5.5)
            .tags("CY,JS")
            .displayedName("Example key 3")
            .description("Example key 3 description")
            .allowedStates(State.STARTED, State.STOPPED,
                           State.NORMAL)  # TODO check
            .minExc(10).maxExc(20).assignmentOptional().defaultValue(11)
            .reconfigurable()
            .commit(),

            FLOAT_ELEMENT(expected).key("e").alias("exampleAlias4")
            .tags("DB,NC,CY")
            .displayedName("Example key 4")
            .description("Example key 4 description")
            .options("1.1100000, -2.22, 5.55")
            .assignmentOptional()
            .defaultValue(1.11)
            .commit(),

            INT64_ELEMENT(expected).key("f").alias("exampleAlias5")
            .tags("LM,DB")
            .displayedName("Example key 5")
            .description("Example key 5 description")
            .assignmentOptional()
            .defaultValue(5)
            .commit(),
        )


@KARABO_CLASSINFO("P2", "1.0")
class P2(Base):
    def __init__(self, configuration):
        super().__init__(configuration)

    @staticmethod
    def expectedParameters(expected):
        (
            STRING_ELEMENT(expected).key("x")
            .description("x")
            .displayedName("x")
            .assignmentOptional().defaultValue("a value")
            .tags("LM,BH")
            .commit(),

            STRING_ELEMENT(expected).key("y")
            .tags("CY")
            .displayedName("Example key 1")
            .description("Example key 1 description")
            .options("Radio,Air Condition,Navigation", ",")
            .assignmentOptional().defaultValue("Radio")
            .reconfigurable()
            .commit(),

            INT32_ELEMENT(expected).key("z").alias(10)
            .tags("CY,LM,KW")
            .displayedName("Example key 2")
            .description("Example key 2 description")
            .options("5, 25, 10")
            .minInc(5).maxInc(25).unit(AMPERE).metricPrefix(MILLI)
            .assignmentOptional().defaultValue(10)
            .init()
            .commit(),
        )


@KARABO_CLASSINFO("P3", "1.0")
class P3(Base):
    def __init__(self, configuration):
        super().__init__(configuration)

    @staticmethod
    def expectedParameters(expected):
        (
            STRING_ELEMENT(expected).key("k")
            .description("k").displayedName("k")
            .assignmentOptional().defaultValue("k value")
            .tags("LM")
            .commit(),

            STRING_ELEMENT(expected).key("l")
            .tags("CY")
            .displayedName("l").description("l")
            .options("Radio,Air Condition,Navigation", ",")
            .assignmentOptional().defaultValue("Navigation")
            .reconfigurable()
            .commit(),

            INT32_ELEMENT(expected).key("m").alias(10)
            .tags("CY,DB,JE,BP,MK,PG,BF")
            .displayedName("Example key 2")
            .description("Example key 2 description")
            .options("5, 25, 10")
            .minInc(5).maxInc(25).unit(METER).metricPrefix(MILLI)
            .assignmentOptional().defaultValue(25)
            .init()
            .commit(),
        )


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("ArrayContainer", "1.0")
class ArrayContainer(Base):
    def __init__(self, configuration):
        super().__init__(configuration)

    @staticmethod
    def expectedParameters(expected):
        (
            NDARRAY_ELEMENT(expected).key("exampleKey16")
            .dtype("BOOL")
            .shape([2, 3])
            .unit(Unit.DEGREE_CELSIUS)
            .metricPrefix(MetricPrefix.CENTI)
            .skipValidation()
            .commit(),

            NDARRAY_ELEMENT(expected).key("exampleKey17")
            .dtype("UINT32")
            .shape([2, 5, 0])
            .skipValidation()
            .commit(),
            # NOTE: validation is skipped for NDArray even when not explicitly
            # asked for. However, this will change in the future.
            NDARRAY_ELEMENT(expected).key("exampleKey18")
            .dtype("FLOAT")
            .shape("3,2,1")
            .commit()
        )


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("GraphicsRenderer2", "1.0")
class GraphicsRenderer2:
    def __init__(self, configuration):
        super().__init__()

    @staticmethod
    def expectedParameters(expected):
        (
            BOOL_ELEMENT(expected).key("antiAlias")
            .tags("NC")
            .displayedName("Use Anti-Aliasing")
            .description("You may switch of for speed")
            .assignmentOptional().defaultValue(True)
            .init().expertAccess()
            .commit(),

            STRING_ELEMENT(expected).key("color")
            .tags("KW")
            .displayedName("Color")
            .description("The default color for any shape")
            .assignmentOptional().defaultValue("red")
            .reconfigurable()
            .commit(),

            BOOL_ELEMENT(expected).key("bold")
            .tags("LM")
            .displayedName("Bold").description("Toggles bold painting")
            .assignmentOptional().defaultValue(False)
            .reconfigurable()
            .commit(),

            CHOICE_ELEMENT(expected).key("shapes")
            .tags("DB")
            .assignmentOptional().defaultValue("rectangle")
            .commit(),

            NODE_ELEMENT(expected).key("shapes.circle")
            .tags("JS")
            .displayedName("Circle").description("A circle")
            .commit(),

            FLOAT_ELEMENT(expected).key("shapes.circle.radius")
            .description("The radius of the circle")
            .displayedName("Radius")
            .tags("NC,KW")
            .minExc(0).maxExc(100).unit(METER).metricPrefix(MILLI)
            .assignmentOptional().defaultValue(10)
            .init().commit(),

            NODE_ELEMENT(expected).key("shapes.rectangle")
            .tags("BH, KW , CY")
            .displayedName("Rectangle").description("A rectangle")
            .commit(),

            FLOAT_ELEMENT(expected).key("shapes.rectangle.b")
            .tags("JS")
            .description("Rectangle side - b").displayedName("Side B")
            .assignmentOptional().defaultValue(10)
            .init().commit(),

            FLOAT_ELEMENT(expected).key("shapes.rectangle.c")
            .tags("LM,JS")
            .description("Rectangle side - c").displayedName("Side C")
            .assignmentOptional().defaultValue(10)
            .init()
            .commit(),

            NODE_ELEMENT(expected).key("shapes.triangle")
            .displayedName("triangle")
            .description("A triangle (Node element containing no "
                         "other elements)")
            .commit(),

            NODE_ELEMENT(expected).key("letter")
            .displayedName("Letter").description("Letter")
            .appendParametersOf(P1)
            .commit(),

        )


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("OtherSchemaElements", "1.0")
class OtherSchemaElements:
    def __init__(self, configuration):
        super().__init__()

    @staticmethod
    def expectedParameters(expected):
        (
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
        )
