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
import unittest

from karabo.bound import (
    IMAGEDATA_ELEMENT, INT32_ELEMENT, MANDATORY, METER, MICRO, NODE_ELEMENT,
    OVERWRITE_ELEMENT, AccessLevel, AccessType, ArchivePolicy, AssignmentType,
    Configurator, DaqDataType, Hash, Logger, MetricPrefix, NodeType, Schema,
    State, Types, Unit, Validator, fullyEqual)

from .configuration_example_classes import (
    ArrayContainer, Base, GraphicsRenderer, GraphicsRenderer1, SomeClass,
    TestStruct1)


class Schema_TestCase(unittest.TestCase):

    def test_buildUp(self):
        try:
            # schema = Shape.getSchema("EditableCircle")
            schema = Configurator("Shape").getSchema("Circle")
            self.assertTrue(schema.isAccessInitOnly("shadowEnabled"))
            self.assertTrue(schema.isAccessInitOnly("radius"))
            self.assertTrue(schema.isLeaf("radius"))
            schema = Configurator("Shape").getSchema("EditableCircle")
            allowedStates = schema.getOptions("state")
            self.assertEqual(allowedStates, ['INIT', 'ERROR', 'NORMAL'])
            self.assertEqual(schema.getDefaultValue("state"), 'INIT')
            self.assertEqual(schema.getDefaultValue("stateN"), 'NORMAL')
            self.assertEqual(schema.getDefaultValue("stateE"), 'ERROR')
            self.assertEqual(schema.getDefaultValue("alarmW"), 'warn')
            self.assertEqual(schema.getDefaultValue("alarmA"), 'alarm')

            allowedStates = schema.getOptions("status")
            self.assertEqual(allowedStates, ['a', 'b', 'c'])
            self.assertEqual(schema.getDefaultValue("status"), 'a')
        except Exception as e:
            self.fail("test_buildUp exception group 1: " + str(e))

        try:
            schema = Schema("test")
            GraphicsRenderer1.expectedParameters(schema)
            self.assertTrue(schema.isAccessInitOnly("circle.radius"))
            self.assertTrue(schema.isLeaf("circle.radius"))
        except Exception as e:
            self.fail("test_buildUp exception group 2: " + str(e))

        try:
            instance = GraphicsRenderer.create("GraphicsRenderer",
                                               Hash("shapes", "Circle",
                                                    "Circle.radius", 0.5,
                                                    "color", "red",
                                                    "antiAlias", "true"))
            self.assertIsNotNone(instance)
        except Exception as e:
            self.fail("test_buildUp exception group 3: " + str(e))

    def test_getRootName(self):
        try:
            schema = Schema("MyTest")
            TestStruct1.expectedParameters(schema)
            self.assertEqual(schema.getRootName(), "MyTest")
        except Exception as e:
            self.fail("test_getRootName exception: " + str(e))

    def test_getTags(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getTags("exampleKey1")[0], "hardware")
            self.assertEqual(schema.getTags("exampleKey1")[1], "poll")
            self.assertEqual(schema.getTags("exampleKey2")[0], "hardware")
            self.assertEqual(schema.getTags("exampleKey2")[1], "poll")
            self.assertEqual(schema.getTags("exampleKey3")[0], "hardware")
            self.assertEqual(schema.getTags("exampleKey3")[1], "set")
            self.assertEqual(schema.getTags("exampleKey4")[0], "software")
            self.assertEqual(schema.getTags("exampleKey5")[0], "h/w")
            self.assertEqual(schema.getTags("exampleKey5")[1], "d.m.y")
        except Exception as e:
            self.fail("test_getTags exception: " + str(e))

    def test_setTags(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertTrue(schema.hasTags('x'))
            self.assertEqual(schema.getTags('x'), ['IK', 'BH'])
            schema.setTags('x', 'CY,SE')
            self.assertEqual(schema.getTags('x'), ['CY', 'SE'])
        except Exception as e:
            self.fail("test_setTags exception: " + str(e))

    def test_getsetExpertLevel(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(schema.getRequiredAccessLevel('x'),
                             AccessLevel.EXPERT)
            self.assertEqual(schema.getRequiredAccessLevel('y'),
                             AccessLevel.OPERATOR)
            self.assertEqual(schema.getRequiredAccessLevel('a'),
                             AccessLevel.OBSERVER)

            schema.setRequiredAccessLevel('x', AccessLevel.EXPERT)
            schema.setRequiredAccessLevel('y', AccessLevel.OPERATOR)
            self.assertEqual(schema.getRequiredAccessLevel('x'),
                             AccessLevel.EXPERT)
            self.assertEqual(schema.getRequiredAccessLevel('y'),
                             AccessLevel.OPERATOR)
        except Exception as e:
            self.fail("test_setTags exception group 1: " + str(e))

        try:
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertEqual(schema.getRequiredAccessLevel('exampleKey1'),
                             AccessLevel.OPERATOR)
            self.assertEqual(schema.getRequiredAccessLevel('exampleKey2'),
                             AccessLevel.OPERATOR)
            self.assertEqual(schema.getRequiredAccessLevel('exampleKey3'),
                             AccessLevel.EXPERT)
            self.assertEqual(schema.getRequiredAccessLevel('exampleKey4'),
                             AccessLevel.EXPERT)
            # default for readOnly
            self.assertEqual(schema.getRequiredAccessLevel('exampleKey5'),
                             AccessLevel.OBSERVER)
            # default for reconfigurable
            self.assertEqual(schema.getRequiredAccessLevel('exampleKey10'),
                             AccessLevel.OPERATOR)
            # observerAccess in reconfigurable
            self.assertEqual(schema.getRequiredAccessLevel('exampleKey11'),
                             AccessLevel.OBSERVER)
        except Exception as e:
            self.fail("test_setTags exception group 2: " + str(e))

    def test_getNodeType(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            nodeType = schema.getNodeType("exampleKey1")
            self.assertEqual(nodeType, NodeType.LEAF)
            self.assertEqual(schema.getNodeType("exampleKey5"), NodeType.LEAF)
        except Exception as e:
            self.fail("test_getNodeType exception: " + str(e))

    def test_getValueType(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getValueType("exampleKey1"), Types.STRING)
            self.assertEqual(schema.getValueType("exampleKey2"), Types.INT32)
            self.assertEqual(schema.getValueType("exampleKey3"), Types.UINT32)
            self.assertEqual(schema.getValueType("exampleKey4"), Types.DOUBLE)
            self.assertEqual(schema.getValueType("exampleKey5"), Types.INT64)
            self.assertEqual(schema.getValueType("exampleKey7"),
                             Types.VECTOR_INT32)
            self.assertEqual(schema.getValueType("exampleKey8"),
                             Types.VECTOR_DOUBLE)
            self.assertEqual(schema.getValueType("exampleKey9"),
                             Types.VECTOR_STRING)
        except Exception as e:
            self.fail("test_getValueType exception: " + str(e))

    def test_getAliasAsString(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getAliasAsString("exampleKey2"), "10")
            self.assertEqual(schema.getAliasAsString("exampleKey3"), "5.5")

            self.assertEqual(schema.getAliasAsString("exampleKey4"),
                             "exampleAlias4")
            self.assertEqual(schema.getAliasAsString("exampleKey5"),
                             "exampleAlias5")
            self.assertEqual(schema.getAliasAsString("exampleKey6"),
                             "1193046,43724")
            self.assertEqual(schema.getAliasAsString("testPath"), "5")
        except Exception as e:
            self.fail("test_getAliasAsString exception: " + str(e))

    def test_keyHasAlias(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertFalse(schema.keyHasAlias("exampleKey1"))
            self.assertTrue(schema.keyHasAlias("exampleKey2"))
            self.assertTrue(schema.keyHasAlias("exampleKey3"))
            self.assertTrue(schema.keyHasAlias("exampleKey4"))
            self.assertTrue(schema.keyHasAlias("exampleKey5"))
            self.assertTrue(schema.keyHasAlias("exampleKey6"))
            self.assertTrue(schema.keyHasAlias("testPath"))
        except Exception as e:
            self.fail("test_keyHasAlias exception: " + str(e))

    def test_aliasHasKey(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertTrue(schema.aliasHasKey(10))
            self.assertTrue(schema.aliasHasKey(5.5))
            self.assertTrue(schema.aliasHasKey("exampleAlias4"))
            self.assertTrue(schema.aliasHasKey("exampleAlias5"))
            self.assertTrue(schema.aliasHasKey([0x00123456, 0x0000aacc]))
            self.assertFalse(schema.aliasHasKey(7))
            self.assertTrue(schema.aliasHasKey(5))
        except Exception as e:
            self.fail("test_aliasHasKey exception: " + str(e))

    def test_getAliasFromKey(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getAliasFromKey("exampleKey2"), 10)
            self.assertEqual(schema.getAliasFromKey("exampleKey3"), 5.5)
            self.assertEqual(schema.getAliasFromKey("exampleKey4"),
                             "exampleAlias4")
            self.assertEqual(schema.getAliasFromKey("exampleKey5"),
                             "exampleAlias5")
            self.assertEqual(schema.getAliasFromKey("exampleKey6"),
                             [0x00123456, 0x0000aacc])
            self.assertEqual(schema.getAliasFromKey("testPath"), 5)
        except Exception as e:
            self.fail("test_getAliasFromKey exception: " + str(e))

    def test_setAlias(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(schema.getAliasFromKey("x"), 10)
            schema.setAlias('x', 'abc')
            self.assertEqual(schema.getAliasFromKey("x"), 'abc')
            schema.setAlias('x', 99)
            self.assertEqual(schema.getAliasFromKey("x"), 99)
        except Exception as e:
            self.fail("test_setAlias exception: " + str(e))

    def test_getKeyFromAlias(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getKeyFromAlias(10), "exampleKey2")
            self.assertEqual(schema.getKeyFromAlias(5.5), "exampleKey3")
            self.assertEqual(schema.getKeyFromAlias("exampleAlias4"),
                             "exampleKey4")
            self.assertEqual(schema.getKeyFromAlias("exampleAlias5"),
                             "exampleKey5")
            self.assertEqual(schema.getKeyFromAlias([0x00123456, 0x0000aacc]),
                             "exampleKey6")
            self.assertEqual(schema.getKeyFromAlias(5), "testPath")
        except Exception as e:
            self.fail("test_KeyFromAlias exception: " + str(e))

    def test_getAccessMode(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getAccessMode("exampleKey1"),
                             AccessType.WRITE)
            self.assertEqual(schema.getAccessMode("exampleKey2"),
                             AccessType.INIT)
            self.assertEqual(schema.getAccessMode("exampleKey3"),
                             AccessType.WRITE)
            self.assertEqual(schema.getAccessMode("exampleKey4"),
                             AccessType.INIT)
            self.assertEqual(schema.getAccessMode("exampleKey5"),
                             AccessType.READ)
            self.assertEqual(schema.getAccessMode("exampleKey8"),
                             AccessType.READ)
            self.assertEqual(schema.getAccessMode("exampleKey10"),
                             AccessType.WRITE)
            self.assertEqual(schema.getAccessMode("testPath"),
                             AccessType.WRITE)
            self.assertEqual(schema.getAccessMode("testPath2"),
                             AccessType.READ)
            self.assertEqual(schema.getAccessMode("testPath3"),
                             AccessType.INIT)
        except Exception as e:
            self.fail("test_getAccessMode exception: " + str(e))

    def test_getAssignment(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getAssignment("exampleKey1"),
                             AssignmentType.OPTIONAL)
            self.assertEqual(schema.getAssignment("exampleKey2"),
                             AssignmentType.OPTIONAL)
            self.assertEqual(schema.getAssignment("exampleKey3"),
                             AssignmentType.MANDATORY)
            self.assertEqual(schema.getAssignment("exampleKey4"),
                             AssignmentType.INTERNAL)
            self.assertEqual(schema.getAssignment("exampleKey5"),
                             AssignmentType.OPTIONAL)
            self.assertEqual(schema.getAssignment("exampleKey8"),
                             AssignmentType.OPTIONAL)
            self.assertEqual(schema.getAssignment("exampleKey10"),
                             AssignmentType.OPTIONAL)
            self.assertEqual(schema.getAssignment("testPath"),
                             AssignmentType.OPTIONAL)
            self.assertEqual(schema.getAssignment("testPath2"),
                             AssignmentType.OPTIONAL)
            self.assertEqual(schema.getAssignment("testPath3"),
                             AssignmentType.MANDATORY)
        except Exception as e:
            self.fail("test_getAssignment exception: " + str(e))

    def test_setAssignment(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertTrue(schema.hasAssignment('x'))
            self.assertTrue(schema.isAssignmentOptional('x'))
            self.assertFalse(schema.isAssignmentMandatory('x'))
            self.assertEqual(schema.getAssignment('x'),
                             AssignmentType.OPTIONAL)
            schema.setAssignment('x', AssignmentType.MANDATORY)
            self.assertFalse(schema.isAssignmentOptional('x'))
            self.assertTrue(schema.isAssignmentMandatory('x'))
            self.assertEqual(schema.getAssignment('x'), MANDATORY)
        except Exception as e:
            self.fail("test_setAssignment exception: " + str(e))

    def test_getOptions(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            options = schema.getOptions("exampleKey1")
            self.assertEqual(options[0], "Radio")
            self.assertEqual(options[1], "Air Condition")
            self.assertEqual(options[2], "Navigation")

            self.assertEqual(schema.getOptions("exampleKey2")[0], 5)
            self.assertEqual(schema.getOptions("exampleKey2")[1], 25)
            self.assertEqual(schema.getOptions("exampleKey2")[2], 10)

            self.assertEqual(schema.getOptions("exampleKey4")[0], 1.11)
            self.assertEqual(schema.getOptions("exampleKey4")[1], -2.22)
            self.assertEqual(schema.getOptions("exampleKey4")[2], 5.55)

            self.assertEqual(schema.getOptions("testPath")[0], "file1")
            self.assertEqual(schema.getOptions("testPath")[1], "file2")
        except Exception as e:
            self.fail("test_getOptions exception: " + str(e))

    def test_setOptions(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")

            options = schema.getOptions("x")
            self.assertEqual(options[0], 5)
            self.assertEqual(options[1], 25)
            self.assertEqual(options[2], 10)
            self.assertEqual(schema.getOptions("x"), [5, 25, 10])

            schema.setOptions('x', '20, 5, 11, 13, 25')
            options = schema.getOptions("x")
            self.assertEqual(options, [20, 5, 11, 13, 25])
        except Exception as e:
            self.fail("test_setOptions exception: " + str(e))

    def test_displayType(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            display = schema.getDisplayType("myNode")
            self.assertEqual(display, "WidgetNode")
        except Exception as e:
            self.fail("test_displayType exception: " + str(e))

    def test_getDefaultValue(self):
        try:
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertEqual(schema.getDefaultValue("exampleKey1"),
                             "Navigation")
            self.assertEqual(schema.getDefaultValue("exampleKey2"), 10)
            self.assertEqual(
                schema.getDefaultValueAs("exampleKey2", Types.STRING), "10")
            self.assertEqual(schema.getDefaultValue("exampleKey5"), 1442244)
            self.assertEqual(
                schema.getDefaultValueAs("exampleKey5", Types.STRING),
                "1442244")

            self.assertEqual(schema.getDefaultValue("exampleKey6"), 1.11)
            self.assertEqual(
                schema.getDefaultValueAs("exampleKey6", Types.DOUBLE), 1.11)

            self.assertEqual(schema.getDefaultValue("exampleKey7"), [1, 2, 3])
            self.assertEqual(schema.getDefaultValue("exampleKey8"),
                             [1.1, 2.2, 3.3])
            self.assertEqual(schema.getDefaultValue("exampleKey9"),
                             ["Hallo", "World"])

            # 'readOnly'-element (vector as well) that does not specify
            #  'initialValue' has 'defaultValue' equal to string "" :
            self.assertEqual(schema.getDefaultValue("testPath2"), "")
            self.assertEqual(schema.getDefaultValue("vectInt"), [])

            self.assertEqual(schema.getDefaultValue("exampleIntKey"), 20)

            self.assertEqual(schema.getDefaultValue("exampleKey5"), 1442244)

            self.assertEqual(schema.getDefaultValue("exampleKey7"), [1, 2, 3])

            # readOnly default specified by 'defaultValue'. not 'initialValue':
            self.assertEqual(schema.getDefaultValue("exampleKey5b"), 42)
            self.assertEqual(schema.getDefaultValue("exampleKey7b"),
                             [11, 22, 33])

        except Exception as e:
            self.fail("test_getDefaultValue exception: " + str(e))

    def test_setDefaultValue(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertTrue(schema.isAssignmentOptional('x'))
            self.assertTrue(schema.hasDefaultValue('x'))
            self.assertEqual(schema.getDefaultValue("x"), 5)
            schema.setDefaultValue("x", 10)
            self.assertEqual(schema.getDefaultValue("x"), 10)
        except Exception as e:
            self.fail("test_setDefaultValue exception: " + str(e))

    def test_getAllowedStates(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            allowedStates = schema.getAllowedStates("exampleKey3")
            self.assertEqual(allowedStates[0], State.STARTED)
            self.assertEqual(allowedStates[1], State.STOPPED)
            self.assertEqual(schema.getAllowedStates("exampleKey3")[2],
                             State.NORMAL)

            self.assertEqual(schema.getAllowedStates("exampleKey7")[0],
                             State.STARTED)
            self.assertEqual(schema.getAllowedStates("exampleKey7")[1],
                             State.NORMAL)
        except Exception as e:
            self.fail("test_getAllowedStates exception: " + str(e))

    def test_getUnit(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getUnit("exampleKey2"), Unit.METER)
            self.assertEqual(schema.getUnitName("exampleKey2"), "meter")
            self.assertEqual(schema.getUnitSymbol("exampleKey2"), "m")
        except Exception as e:
            self.fail("test_getUnit exception: " + str(e))

    def test_setUnit(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(schema.getUnit("x"), Unit.AMPERE)
            schema.setUnit('x', METER)
            self.assertEqual(schema.getUnit("x"), METER)
            self.assertEqual(schema.getUnit("x"), Unit.METER)
            self.assertEqual(schema.getUnitName("x"), "meter")
            self.assertEqual(schema.getUnitSymbol("x"), "m")
        except Exception as e:
            self.fail("test_setUnit exception: " + str(e))

    def test_getMetricPrefix(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getMetricPrefix("exampleKey2"),
                             MetricPrefix.MILLI)
            self.assertEqual(schema.getMetricPrefixName("exampleKey2"),
                             "milli")
            self.assertEqual(schema.getMetricPrefixSymbol("exampleKey2"), "m")
        except Exception as e:
            self.fail("test_getMetricPrefix exception: " + str(e))

    def test_setMetricPrefix(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(schema.getMetricPrefix("x"), MetricPrefix.MILLI)
            schema.setMetricPrefix("x", MetricPrefix.MICRO)
            self.assertEqual(schema.getMetricPrefix("x"), MICRO)
            self.assertEqual(schema.getMetricPrefixName("x"), "micro")
            self.assertEqual(schema.getMetricPrefixSymbol("x"), "u")
        except Exception as e:
            self.fail("test_setMetricPrefix exception: " + str(e))

    def test_getMinIncMaxInc(self):
        try:
            schema = TestStruct1.getSchema("TestStruct1")
            self.assertEqual(schema.getMinInc("exampleKey2"), 5)
            self.assertEqual(schema.getMinIncAs("exampleKey2", Types.STRING),
                             "5")
            self.assertEqual(schema.getMaxInc("exampleKey2"), 25)
            self.assertEqual(schema.getMaxIncAs("exampleKey2", Types.STRING),
                             "25")
        except Exception as e:
            self.fail("test_getMinIncMaxInc exception: " + str(e))

    def test_setMinIncMaxInc(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(schema.getMinInc("x"), 5)
            self.assertEqual(schema.getMinIncAs("x", Types.STRING), "5")
            self.assertEqual(schema.getMaxInc("x"), 25)
            self.assertEqual(schema.getMaxIncAs("x", Types.STRING), "25")
            schema.setMinInc('x', 3)
            schema.setMaxInc('x', 30)
            self.assertEqual(schema.getMinInc("x"), 3)
            self.assertEqual(schema.getMinIncAs("x", Types.STRING), "3")
            self.assertEqual(schema.getMaxInc("x"), 30)
            self.assertEqual(schema.getMaxIncAs("x", Types.STRING), "30")
        except Exception as e:
            self.fail("test_setMinIncMaxInc exception: " + str(e))

    def test_getMinExcMaxExc(self):
        schema = TestStruct1.getSchema("TestStruct1")
        try:
            self.assertEqual(schema.getMinExc("exampleKey3"), 10)
            self.assertEqual(schema.getMinExc("exampleKey4"), -2.22)
        except Exception as e:
            self.fail("test_getMinExcMaxExc exception in getMinExc: " + str(e))

        try:
            self.assertEqual(schema.getMaxExc("exampleKey3"), 20)
            self.assertEqual(schema.getMaxExc("exampleKey4"), 5.55)
        except Exception as e:
            self.fail("test_getMinExcMaxExc exception in getMaxExc: " + str(e))

        try:
            self.assertEqual(schema.getMinExcAs("exampleKey3", Types.STRING),
                             "10")
            self.assertEqual(schema.getMinExcAs("exampleKey4", Types.STRING),
                             "-2.22")
        except Exception as e:
            self.fail(
                "test_getMinExcMaxExc exception in getMinExcAs: " + str(e))

        try:
            self.assertEqual(schema.getMaxExcAs("exampleKey3", Types.STRING),
                             "20")
            self.assertEqual(schema.getMaxExcAs("exampleKey4", Types.STRING),
                             "5.55")
        except Exception as e:
            self.fail(
                "test_getMinExcMaxExc exception in getMaxExcAs: " + str(e))

    def test_setMinExcMaxExc(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(schema.getMinExc("y"), 0)
            self.assertEqual(schema.getMaxExc("y"), 29)
            schema.setMinExc("y", 2)
            schema.setMaxExc("y", 30)
            self.assertEqual(schema.getMinExc("y"), 2)
            self.assertEqual(schema.getMaxExc("y"), 30)
        except Exception as e:
            self.fail("test_setMinExcMaxExc exception in getMinExc: " + str(e))

    def test_vectorElement(self):
        try:
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertEqual(schema.isAccessReadOnly("exampleKey7"), True)
            self.assertEqual(schema.hasDefaultValue("exampleKey7"), True)
            self.assertEqual(schema.hasDefaultValue("exampleKey10"), True)
            self.assertEqual(schema.hasDefaultValue("exampleKey11"), True)
            self.assertEqual(schema.hasDefaultValue("exampleKey12"), True)
            self.assertEqual(schema.hasDefaultValue("exampleKey14"), True)
            self.assertEqual(schema.hasDefaultValue("exampleKey15"), True)

            self.assertEqual(schema.getDefaultValue("exampleKey10"),
                             [10, 20, 30])

            self.assertEqual(schema.getDefaultValue("exampleKey12"),
                             [1.1, -2.2, 3.3])

            self.assertEqual(schema.getDefaultValue("exampleKey11"),
                             [10, 20, 30])
            self.assertEqual(
                schema.getDefaultValueAs("exampleKey11", Types.STRING),
                "10,20,30")
            self.assertEqual(
                schema.getDefaultValueAs("exampleKey11", Types.VECTOR_INT32),
                [10, 20, 30])

            self.assertEqual(schema.getDefaultValue("exampleKey14"),
                             ["Hallo", "World", "Test"])
            self.assertEqual(
                schema.getDefaultValueAs("exampleKey14", Types.STRING),
                "Hallo,World,Test")

            self.assertEqual(schema.getDefaultValue("exampleKey15"),
                             ["word1", "word2", "test"])
            self.assertEqual(
                schema.getDefaultValueAs("exampleKey15", Types.STRING),
                "word1,word2,test")

            self.assertEqual(schema.getMinSize("exampleKey10"), 2)
            self.assertEqual(schema.getMaxSize("exampleKey10"), 7)
        except Exception as e:
            self.fail("test_vectorElement exception: " + str(e))

        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            validator = Validator()
            configuration = Hash('somelist', [])
            ok, error, validated = validator.validate(schema, configuration)
            if not ok:
                raise RuntimeError(error)
            somelist = validated['somelist']
            somelist.append(99)
            somelist.append(55)
            configuration['somelist'] = somelist
            ok, error, validated = validator.validate(schema, configuration)
            if not ok:
                raise RuntimeError(error)
            self.assertIsNotNone(validated)
        except Exception as e:
            self.fail("test_vectorElement exception 2: " + str(e))

    def test_ndarrayElement(self):
        schema = Configurator(ArrayContainer).getSchema("ArrayContainer")

        assert schema.getDefaultValue("exampleKey16.shape") == [2, 3]
        assert schema.getDefaultValue("exampleKey17.shape") == [2, 5, 0]
        assert schema.getDefaultValue("exampleKey18.shape") == [3, 2, 1]

        assert schema.getUnit("exampleKey16.data") == Unit.DEGREE_CELSIUS
        assert schema.getMetricPrefix(
            "exampleKey16.data") == MetricPrefix.CENTI

        assert schema.isAccessReadOnly("exampleKey16")
        assert schema.isAccessReadOnly("exampleKey17")
        assert schema.isAccessReadOnly("exampleKey18")

    def test_getDisplayType(self):
        schema = TestStruct1.getSchema("TestStruct1")
        self.assertEqual(schema.getDisplayType("exampleBitsKey1"), "bin")
        self.assertEqual(schema.getDisplayType("exampleBitsKey2"),
                         "bin|10:In Error, 21:Busy, 35:HV On, 55:Crate On")
        self.assertEqual(schema.getDisplayType("exampleBitsKey3"), "oct")
        self.assertEqual(schema.getDisplayType("exampleBitsKey4"), "hex")
        self.assertEqual(schema.getDisplayType("exampleKey5"),
                         "Int64DisplayType")
        self.assertEqual(schema.getDisplayType("exampleKey8"), "Curve")
        self.assertEqual(schema.getDisplayType("exampleKey9"),
                         "TestDisplayType")

    def test_setDisplayType(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertFalse(schema.hasDisplayType('y'))
            schema.setDisplayType('y', 'blabla')
            self.assertTrue(schema.hasDisplayType('y'))
            self.assertEqual(schema.getDisplayType("y"), "blabla")
        except Exception as e:
            self.fail("test_setDisplayType exception: " + str(e))

    def test_isCommand(self):
        try:
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertTrue(schema.isCommand("slotTest"))
        except Exception as e:
            self.fail("test_isCommand exception: " + str(e))

    def test_isProperty(self):
        try:
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertFalse(schema.isProperty("slotTest"))
            self.assertTrue(schema.isProperty("testPath2"))

        except Exception as e:
            self.fail("test_isProperty exception: " + str(e))

    def test_hasArchivePolicy(self):
        try:
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertTrue(schema.hasArchivePolicy("exampleKey5"))
            self.assertFalse(schema.hasArchivePolicy("exampleKey6"))
            self.assertFalse(schema.hasArchivePolicy("exampleKey7"))
            self.assertTrue(schema.hasArchivePolicy("exampleKey8"))
        except Exception as e:
            self.fail("test_hasArchivePolicy exception: " + str(e))

    def test_getArchivePolicy(self):
        try:
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertEqual(schema.getArchivePolicy("exampleKey5"),
                             ArchivePolicy.EVERY_EVENT)
            self.assertEqual(schema.getArchivePolicy("exampleKey8"),
                             ArchivePolicy.NO_ARCHIVING)
        except Exception as e:
            self.fail("test_getArchivePolicy exception: " + str(e))

    def test_setArchivePolicy(self):
        try:
            schema = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(schema.getArchivePolicy("a"),
                             ArchivePolicy.NO_ARCHIVING)
            schema.setArchivePolicy('a', ArchivePolicy.EVERY_EVENT)
            self.assertEqual(schema.getArchivePolicy("a"),
                             ArchivePolicy.EVERY_EVENT)
        except Exception as e:
            self.fail("test_setArchivePolicy exception: " + str(e))

    def test_perKeyFunctionality(self):
        try:
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            keys = schema.getKeys()
            for key in keys:
                if key == "exampleKey1":
                    self.assertTrue(schema.hasAssignment(key))
                    self.assertTrue(schema.isAssignmentOptional(key))
                    self.assertTrue(schema.hasDefaultValue(key))
                    self.assertTrue(schema.hasAccessMode(key))
                    self.assertTrue(schema.isAccessReconfigurable(key))
                    self.assertTrue(schema.hasOptions(key))
                    self.assertTrue(schema.hasTags(key))
                    self.assertFalse(schema.hasUnit(key))
                    self.assertFalse(schema.hasMetricPrefix(key))

                if key == "exampleKey2":
                    self.assertTrue(schema.hasDefaultValue(key))
                    self.assertTrue(schema.hasAccessMode(key))
                    self.assertTrue(schema.isAccessInitOnly(key))
                    self.assertTrue(schema.hasOptions(key))
                    self.assertTrue(schema.hasTags(key))
                    self.assertFalse(schema.hasAllowedStates(key))
                    self.assertTrue(schema.hasUnit(key))
                    self.assertTrue(schema.hasMetricPrefix(key))
                    self.assertTrue(schema.hasMinInc(key))
                    self.assertTrue(schema.hasMaxInc(key))

                if key == "exampleKey3":
                    self.assertTrue(schema.hasAssignment(key))
                    self.assertTrue(schema.isAssignmentMandatory(key))
                    self.assertFalse(schema.hasDefaultValue(key))
                    self.assertFalse(schema.hasOptions(key))
                    self.assertTrue(schema.hasAllowedStates(key))
                    self.assertTrue(schema.hasMinExc(key))
                    self.assertTrue(schema.hasMaxExc(key))

                if key == "exampleKey4":
                    self.assertFalse(schema.hasDefaultValue(key))
                    self.assertTrue(schema.isAssignmentInternal(key))
                    self.assertTrue(schema.hasAccessMode(key))
                    self.assertTrue(schema.isAccessInitOnly(key))

                if key == "exampleKey5":
                    self.assertTrue(schema.hasDefaultValue(key))
                    self.assertTrue(schema.hasAssignment(key))
                    self.assertTrue(schema.isAssignmentOptional(key))
                    self.assertTrue(schema.hasAccessMode(key))
                    self.assertTrue(schema.isAccessReadOnly(key))

        except Exception as e:
            self.fail("test_perKeyFunctionality exception group 1: " + str(e))

    def test_merge(self):
        try:
            schema = Configurator(Base).getSchema('P1')
            self.assertTrue("a" in schema)
            self.assertFalse("x" in schema)
            self.assertFalse("y" in schema)
            self.assertFalse("z" in schema)

            schema2 = Configurator(Base).getSchema('P2')
            self.assertTrue("x" in schema2)
            self.assertTrue("y" in schema2)
            self.assertTrue("z" in schema2)

            schema += schema2

            self.assertTrue("a" in schema)
            self.assertTrue("x" in schema)
            self.assertTrue("y" in schema)
            self.assertTrue("z" in schema)

        except Exception as e:
            self.fail("test_merge exception: " + str(e))

    def test_logger(self):
        config = Hash("level", "DEBUG")
        Logger.configure(config)
        root = Logger.getCategory()
        a1 = Logger.getCategory("a1")
        a1_a2 = Logger.getCategory("a1.a2")
        root.DEBUG("ERROR")

        Logger.useOstream()
        root.DEBUG("ok")
        a1.DEBUG("ok")
        a1_a2.DEBUG("ok")
        root.INFO("ok")
        a1.INFO("ok")
        a1_a2.INFO("ok")

        Logger.reset()
        root.DEBUG("ERROR")
        a1.DEBUG("ERROR")
        a1_a2.DEBUG("ERROR")

    def test_helpFunction(self):
        pass
        # uncomment to see help:
        # schema = TestStruct1.getSchema("TestStruct1")
        # schema.help()

    def test_schemaImageElement(self):
        try:
            schema = Configurator(TestStruct1).getSchema("TestStruct1")
            self.assertEqual(schema.getDisplayType("myImageElement"),
                             "ImageData")
            self.assertEqual(schema.isCustomNode("myImageElement"), True)
            self.assertEqual(schema.getCustomNodeClass("myImageElement"),
                             "ImageData")
            self.assertEqual(schema.getAccessMode("myImageElement"),
                             AccessType.READ)
            self.assertEqual(schema.getNodeType("myImageElement"),
                             NodeType.NODE)
            self.assertEqual(schema.getRequiredAccessLevel("myImageElement"),
                             AccessLevel.OPERATOR)  # .operatorAccess()
            self.assertEqual(schema.getDisplayedName("myImageElement"),
                             "myImage")
            self.assertEqual(schema.getDescription("myImageElement"),
                             "Image Element")
            self.assertEqual(schema.getDefaultValue("myImageElement.dims"),
                             [100, 200])
            schema2 = Configurator(SomeClass).getSchema("SomeClassId")
            self.assertEqual(schema2.getDefaultValue("myImageElement.dims"),
                             [110, 210])
            ide = IMAGEDATA_ELEMENT(schema2).key("myImageElement2")
            with self.assertRaises(RuntimeError):
                ide.setDimensions(123)  # only list ans str are allowed

            # Hijack this test to test correct false result of isCustomNode,
            # but here just one type, excessive test is done for
            # underlying C++ code
            self.assertEqual(schema.isCustomNode("exampleKey1"), False)
        except Exception as e:
            self.fail("test_schemaImageElement group 1: " + str(e))

        try:
            # self.assertEqual(schema.getDescription("myImageElement.pixels"),
            #  "Pixel array")
            # self.assertEqual(schema.getValueType("myImageElement.pixels"),
            # Types.VECTOR_CHAR)

            self.assertEqual(schema.getDisplayedName("myImageElement.dims"),
                             "Dimensions")
            self.assertEqual(schema.getValueType("myImageElement.dims"),
                             Types.VECTOR_UINT64)
            self.assertEqual(schema.getDisplayType("myImageElement.dims"),
                             "Curve")

            self.assertEqual(
                schema.getDisplayedName("myImageElement.encoding"), "Encoding")
            self.assertEqual(schema.getValueType("myImageElement.encoding"),
                             Types.INT32)

        # self.assertEqual(
        # schema.getDisplayedName("myImageElement.pixels.isBigEndian"),
        # "Is big endian")
        # self.assertEqual(
        # schema.getValueType("myImageElement.pixels.isBigEndian"), Types.BOOL)
        # self.assertEqual(
        # schema.getDefaultValue("myImageElement.pixels.isBigEndian"), False)

        except Exception as e:
            self.fail("test_schemaImageElement group 2: " + str(e))

    def test_allowed_states(self):
        schema = Configurator(TestStruct1).getSchema("TestStruct1")
        schema.setAllowedStates("exampleKey3", (State.INIT, State.NORMAL))
        allowedStates = schema.getAllowedStates("exampleKey3")
        self.assertEqual(allowedStates, [State.INIT, State.NORMAL])

    def test_daq_data_type(self):
        schema = Schema()

        NODE_ELEMENT(schema).key(
            "trainData").commit()  # Has no DAQ data type yet
        NODE_ELEMENT(schema).key("pulseData").setDaqDataType(
            DaqDataType.PULSE).commit()

        self.assertEqual(schema.hasDaqDataType("trainData"), False)
        self.assertEqual(schema.hasDaqDataType("pulseData"), True)
        self.assertEqual(schema.getDaqDataType("pulseData"), DaqDataType.PULSE)

        # Now add DAQ data type to node "trainData"
        schema.setDaqDataType("trainData", DaqDataType.TRAIN)
        self.assertEqual(schema.hasDaqDataType("trainData"), True)
        self.assertEqual(schema.getDaqDataType("trainData"), DaqDataType.TRAIN)

    def test_table_element(self):
        schema = TestStruct1.getSchema("TestStruct1")

        # Both, defaultValue and initialValue should work for read-only tables
        default = schema.getDefaultValue("tableI")
        self.assertEqual(len(default), 1)
        self.assertTrue(fullyEqual(default[0], Hash("int", 2)))
        default = schema.getDefaultValue("tableD")
        self.assertEqual(len(default), 1)
        self.assertTrue(fullyEqual(default[0], Hash("int", 3)))

        # There are more tests of tables in the integration tests...

    def test_overwrite_restrictions_for_options(self):
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
        self.assertEqual(len(vec), 2)
        self.assertEqual(vec[0], 0)
        self.assertEqual(vec[1], 1)
        (
            OVERWRITE_ELEMENT(schema).key("range")
            .setNewOptions("0,1,2")
            .commit()
        )
        vec = schema.getOptions("range")
        self.assertEqual(len(vec), 3)
        self.assertEqual(vec[0], 0)
        self.assertEqual(vec[1], 1)
        self.assertEqual(vec[2], 2)

    def test_overwrite_tags(self):
        schema = Schema()
        (
            INT32_ELEMENT(schema).key("taggedProp")
            .tags("bip, bop")
            .assignmentOptional().defaultValue(0)
            .reconfigurable()
            .commit()
        )
        vec = schema.getTags("taggedProp")
        self.assertEqual(len(vec), 2)
        self.assertEqual(vec[0], "bip")
        self.assertEqual(vec[1], "bop")
        (
            OVERWRITE_ELEMENT(schema).key("taggedProp")
            .setNewTags("doff")
            .commit()
        )
        vec = schema.getTags("taggedProp")
        self.assertEqual(len(vec), 1)
        self.assertEqual(vec[0], "doff")
        (
            OVERWRITE_ELEMENT(schema).key("taggedProp")
            .setNewTags(["doff", "deff"])
            .commit()
        )
        vec = schema.getTags("taggedProp")
        self.assertEqual(len(vec), 2)
        self.assertEqual(vec[0], "doff")
        self.assertEqual(vec[1], "deff")
        (
            OVERWRITE_ELEMENT(schema).key("taggedProp")
            .setNewTags(("chip", "chop"))
            .commit()
        )
        vec = schema.getTags("taggedProp")
        self.assertEqual(len(vec), 2)
        self.assertEqual(vec[0], "chip")
        self.assertEqual(vec[1], "chop")
        # Only iterable of str are allowed
        with self.assertRaises(RuntimeError):
            (
                OVERWRITE_ELEMENT(schema).key("taggedProp")
                .setNewTags((1, 2))
                .commit()
            )


if __name__ == '__main__':
    unittest.main()
