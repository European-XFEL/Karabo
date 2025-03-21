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

import pytest

from karabind import (
    AMPERE, IMAGEDATA_ELEMENT, INT32_ELEMENT, MANDATORY, METER, MICRO, MILLI,
    NDARRAY_ELEMENT, NODE_ELEMENT, OVERWRITE_ELEMENT, STATE_ELEMENT,
    STRING_ELEMENT, AccessLevel, AccessType, ArchivePolicy, AssignmentType,
    DaqDataType, DAQPolicy, Hash, Logger, MetricPrefix, NodeType, Schema,
    Types, Unit, Validator, cppGraphicsRenderer1SchemaTest,
    cppOtherSchemaElementsSchemaOtherSchemaElements, cppShapeSchemaCircle,
    cppShapeSchemaEditableCircle, cppSomeClassSchemaSomeClassId,
    cppTestStruct1SchemaMyTest, cppTestStruct1SchemaTestStruct1, fullyEqual)
from karabo.common.alarm_conditions import AlarmCondition
from karabo.common.states import State


def test_buildUp():
    try:
        schema = cppShapeSchemaCircle()
        assert schema.isAccessInitOnly("shadowEnabled")
        assert schema.isAccessInitOnly("radius")
        assert schema.isLeaf("radius")
        schema = cppShapeSchemaEditableCircle()
        allowedStates = schema.getOptions("state")
        assert allowedStates == ['INIT', 'ERROR', 'NORMAL']
        assert schema.getDefaultValue("state") == 'INIT'
        assert schema.getDefaultValue("stateN") == 'NORMAL'
        assert schema.getDefaultValue("stateE") == 'ERROR'
        assert schema.getDefaultValue("alarmW") == 'warn'
        assert schema.getDefaultValue("alarmA") == 'alarm'

        allowedStates = schema.getOptions("status")
        assert allowedStates == ['a', 'b', 'c']
        assert schema.getDefaultValue("status") == 'a'
    except Exception as e:
        pytest.fail(e, pytrace=True)

    try:
        schema = cppGraphicsRenderer1SchemaTest()
        assert schema.isAccessInitOnly("shapes.circle.radius")
        assert schema.isLeaf("shapes.circle.radius")
    except Exception as e:
        pytest.fail(e, pytrace=True)

    # try:
    #     instance = GraphicsRenderer.create("GraphicsRenderer",
    #                                        Hash("shapes.Circle.radius",
    #                                             0.5, "color", "red",
    #                                             "antiAlias", "true"))
    #     assert instance is not None
    # except Exception as e:
    #     pytest.fail(e, pytrace=True)


def test_getRootName():
    try:
        schema = cppTestStruct1SchemaMyTest()
        assert schema.getRootName() == "MyTest"
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_getTags():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        assert schema.getTags("exampleKey1")[0] == "hardware"
        assert schema.getTags("exampleKey1")[1] == "poll"
        assert schema.getTags("exampleKey2")[0] == "hardware"
        assert schema.getTags("exampleKey2")[1] == "poll"
        assert schema.getTags("exampleKey3")[0] == "hardware"
        assert schema.getTags("exampleKey3")[1] == "set"
        assert schema.getTags("exampleKey4")[0] == "software"
        assert schema.getTags("exampleKey5")[0] == "h/w"
        assert schema.getTags("exampleKey5")[1] == "d.m.y"
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_setTags():
    try:
        schema = cppSomeClassSchemaSomeClassId()
        assert schema.hasTags('x')
        assert schema.getTags('x') == ['IK', 'BH']
        schema.setTags('x', 'CY,SE')
        assert schema.getTags('x') == ['CY', 'SE']
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_getsetExpertLevel():
    try:
        schema = cppSomeClassSchemaSomeClassId()
        assert schema.getRequiredAccessLevel('x') == AccessLevel.EXPERT
        assert schema.getRequiredAccessLevel('y') == AccessLevel.USER
        assert schema.getRequiredAccessLevel('a') == AccessLevel.OBSERVER

        schema.setRequiredAccessLevel('x', AccessLevel.ADMIN)
        schema.setRequiredAccessLevel('y', AccessLevel.OPERATOR)
        assert schema.getRequiredAccessLevel('x') == AccessLevel.ADMIN
        assert schema.getRequiredAccessLevel('y') == AccessLevel.OPERATOR
    except Exception as e:
        pytest.fail(e, pytrace=True)

    try:
        schema = cppTestStruct1SchemaTestStruct1()
        assert schema.getRequiredAccessLevel('exampleKey1') == \
            AccessLevel.USER
        assert schema.getRequiredAccessLevel('exampleKey2') == \
            AccessLevel.OPERATOR
        assert schema.getRequiredAccessLevel('exampleKey3') == \
            AccessLevel.EXPERT
        assert schema.getRequiredAccessLevel('exampleKey4') == \
            AccessLevel.ADMIN
        # default for readOnly
        assert schema.getRequiredAccessLevel('exampleKey5') == \
            AccessLevel.OBSERVER
        # default for reconfigurable
        assert schema.getRequiredAccessLevel('exampleKey10') == \
            AccessLevel.USER
        # observerAccess in reconfigurable
        assert schema.getRequiredAccessLevel('exampleKey11') == \
            AccessLevel.OBSERVER
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_isNode():
    schema = cppGraphicsRenderer1SchemaTest()
    assert schema.getRootName() == "test"
    assert schema.isNode("shapes.circle")
    assert schema.isNode("shapes.rectangle")
    assert not schema.isNode("shapes")


def test_getNodeType():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        nodeType = schema.getNodeType("exampleKey1")
        assert nodeType == NodeType.LEAF
        assert schema.getNodeType("exampleKey5") == NodeType.LEAF
        schema = cppGraphicsRenderer1SchemaTest()
        assert schema.getNodeType("shapes") == NodeType.CHOICE_OF_NODES
        assert schema.getNodeType("shapes.circle") == NodeType.NODE
        assert schema.getNodeType("shapes.rectangle") == NodeType.NODE
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_getValueType():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        assert schema.getValueType("exampleKey1") == Types.STRING
        assert schema.getValueType("exampleKey2") == Types.INT32
        assert schema.getValueType("exampleKey3") == Types.UINT32
        assert schema.getValueType("exampleKey4") == Types.DOUBLE
        assert schema.getValueType("exampleKey5") == Types.INT64
        assert schema.getValueType("exampleKey7") == Types.VECTOR_INT32
        assert schema.getValueType("exampleKey8") == Types.VECTOR_DOUBLE
        assert schema.getValueType("exampleKey9") == Types.VECTOR_STRING
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_getAliasAsString():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        assert schema.getAliasAsString("exampleKey2") == "10"
        assert schema.getAliasAsString("exampleKey3") == "5.5"

        assert schema.getAliasAsString("exampleKey4") == "exampleAlias4"
        assert schema.getAliasAsString("exampleKey5") == "exampleAlias5"
        assert schema.getAliasAsString("exampleKey6") == "1193046,43724"
        assert schema.getAliasAsString("testPath") == "5"
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_keyHasAlias():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        assert schema.keyHasAlias("exampleKey1") is False
        assert schema.keyHasAlias("exampleKey2")
        assert schema.keyHasAlias("exampleKey3")
        assert schema.keyHasAlias("exampleKey4")
        assert schema.keyHasAlias("exampleKey5")
        assert schema.keyHasAlias("exampleKey6")
        assert schema.keyHasAlias("testPath")
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_aliasHasKey():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        assert schema.aliasHasKey(10)
        assert schema.aliasHasKey(5.5)
        assert schema.aliasHasKey("exampleAlias4")
        assert schema.aliasHasKey("exampleAlias5")
        assert schema.aliasHasKey([0x00123456, 0x0000aacc])
        assert schema.aliasHasKey(7) is False
        assert schema.aliasHasKey(5)
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_getAliasFromKey():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        assert schema.getAliasFromKey("exampleKey2") == 10
        assert schema.getAliasFromKey("exampleKey3") == 5.5
        assert schema.getAliasFromKey("exampleKey4") == "exampleAlias4"
        assert schema.getAliasFromKey("exampleKey5") == "exampleAlias5"
        assert schema.getAliasFromKey("exampleKey6") == \
            [0x00123456, 0x0000aacc]
        assert schema.getAliasFromKey("testPath") == 5
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_setAlias():
    try:
        schema = cppSomeClassSchemaSomeClassId()
        assert schema.getAliasFromKey("x") == 10
        schema.setAlias('x', 'abc')
        assert schema.getAliasFromKey("x") == 'abc'
        schema.setAlias('x', 99)
        assert schema.getAliasFromKey("x") == 99
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_getKeyFromAlias():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        assert schema.getKeyFromAlias(10) == "exampleKey2"
        assert schema.getKeyFromAlias(5.5) == "exampleKey3"
        assert schema.getKeyFromAlias("exampleAlias4") == "exampleKey4"
        assert schema.getKeyFromAlias("exampleAlias5") == "exampleKey5"
        assert schema.getKeyFromAlias([0x00123456, 0x0000aacc]) == \
            "exampleKey6"
        assert schema.getKeyFromAlias(5) == "testPath"
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_getAccessMode():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
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
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_getAssignment():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
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
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_setAssignment():
    try:
        schema = cppSomeClassSchemaSomeClassId()
        assert schema.hasAssignment('x')
        assert schema.isAssignmentOptional('x')
        assert schema.isAssignmentMandatory('x') is False
        assert schema.getAssignment('x') == AssignmentType.OPTIONAL
        schema.setAssignment('x', AssignmentType.MANDATORY)
        assert schema.isAssignmentOptional('x') is False
        assert schema.isAssignmentMandatory('x')
        assert schema.getAssignment('x') == MANDATORY
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_getDescription():
    schema = cppTestStruct1SchemaTestStruct1()
    assert schema.getDescription('exampleKey1') == "Example key 1 description"


def test_setDescription():
    schema = cppTestStruct1SchemaTestStruct1()
    schema.setDescription('exampleKey1', "No description")
    assert schema.getDescription('exampleKey1') == "No description"


def test_getOptions():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
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
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_setOptions():
    try:
        schema = cppSomeClassSchemaSomeClassId()

        options = schema.getOptions("x")
        assert options[0] == 5
        assert options[1] == 25
        assert options[2] == 10
        assert schema.getOptions("x") == [5, 25, 10]

        schema.setOptions('x', '20, 5, 11, 13, 25')
        options = schema.getOptions("x")
        assert options == [20, 5, 11, 13, 25]
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_displayType():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        display = schema.getDisplayType("myNode")
        assert display == "WidgetNode"
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_classId():
    try:
        schema = cppShapeSchemaCircle()
        assert schema.hasClassId("state")
        assert schema.getClassId("state") == "State"
        assert schema.hasClassId("alarm")
        assert schema.getClassId("alarm") == "AlarmCondition"
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_getDefaultValue():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        assert schema.getDefaultValue("exampleKey1") == "Navigation"
        assert schema.getDefaultValue("exampleKey2") == 10
        assert schema.getDefaultValueAs("exampleKey2", Types.STRING) == "10"
        assert schema.getDefaultValue("exampleKey5") == 1442244
        assert schema.getDefaultValueAs("exampleKey5", Types.STRING) == \
            "1442244"
        assert schema.getDefaultValue("exampleKey6") == 1.11
        assert schema.getDefaultValueAs("exampleKey6", Types.DOUBLE) == 1.11
        assert schema.getDefaultValue("exampleKey7") == [1, 2, 3]
        assert schema.getDefaultValue("exampleKey8") == [1.1, 2.2, 3.3]
        assert schema.getDefaultValue("exampleKey9") == ["Hallo", "World"]

        # 'readOnly'-element (vector as well) that does not specify
        #  'initialValue' has 'defaultValue' equal to string "" :
        assert schema.getDefaultValue("testPath2") == ""
        assert schema.getDefaultValue("vectInt") == []
        assert schema.getDefaultValue("exampleIntKey") == 20
        assert schema.getDefaultValue("exampleKey5") == 1442244
        assert schema.getDefaultValue("exampleKey7") == [1, 2, 3]

        # readOnly default specified by 'defaultValue'. not 'initialValue':
        assert schema.getDefaultValue("exampleKey5b") == 42
        assert schema.getDefaultValue("exampleKey7b") == [11, 22, 33]
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_setDefaultValue():
    try:
        schema = cppSomeClassSchemaSomeClassId()
        assert schema.isAssignmentOptional('x')
        assert schema.hasDefaultValue('x')
        assert schema.getDefaultValue("x") == 5
        schema.setDefaultValue("x", 10)
        assert schema.getDefaultValue("x") == 10
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_getAllowedStates():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        allowedStates = schema.getAllowedStates("exampleKey3")
        assert allowedStates[0] == State.STARTED
        assert allowedStates[1] == State.STOPPED
        assert allowedStates[2] == State.NORMAL
        assert schema.getAllowedStates("exampleKey3")[2] == State.NORMAL
        assert schema.getAllowedStates("exampleKey7")[0] == State.STARTED
        assert schema.getAllowedStates("exampleKey7")[1] == State.NORMAL
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_getUnit():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        assert schema.getUnit("exampleKey2") == Unit.METER
        assert schema.getUnitName("exampleKey2") == "meter"
        assert schema.getUnitSymbol("exampleKey2") == "m"
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_setUnit():
    try:
        schema = cppSomeClassSchemaSomeClassId()
        assert schema.getUnit("x") == Unit.AMPERE
        schema.setUnit('x', METER)
        assert schema.getUnit("x") == METER
        assert schema.getUnit("x") == Unit.METER
        assert schema.getUnitName("x") == "meter"
        assert schema.getUnitSymbol("x") == "m"
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_getMetricPrefix():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        assert schema.getMetricPrefix("exampleKey2") == MetricPrefix.MILLI
        assert schema.getMetricPrefixName("exampleKey2") == "milli"
        assert schema.getMetricPrefixSymbol("exampleKey2") == "m"
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_setMetricPrefix():
    try:
        schema = cppSomeClassSchemaSomeClassId()
        assert schema.getMetricPrefix("x") == MetricPrefix.MILLI
        schema.setMetricPrefix("x", MetricPrefix.MICRO)
        assert schema.getMetricPrefix("x") == MICRO
        assert schema.getMetricPrefixName("x") == "micro"
        assert schema.getMetricPrefixSymbol("x") == "u"
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_getMinIncMaxInc():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        assert schema.getMinInc("exampleKey2") == 5
        assert schema.getMinIncAs("exampleKey2", Types.STRING) == "5"
        assert schema.getMaxInc("exampleKey2") == 25
        assert schema.getMaxIncAs("exampleKey2", Types.STRING) == "25"
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_setMinIncMaxInc():
    try:
        schema = cppSomeClassSchemaSomeClassId()
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
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_getMinExcMaxExc():
    schema = cppTestStruct1SchemaTestStruct1()
    try:
        assert schema.getMinExc("exampleKey3") == 10
        assert schema.getMinExc("exampleKey4") == -2.22
    except Exception as e:
        pytest.fail(e, pytrace=True)

    try:
        assert schema.getMaxExc("exampleKey3") == 20
        assert schema.getMaxExc("exampleKey4") == 5.55
    except Exception as e:
        pytest.fail(e, pytrace=True)

    try:
        assert schema.getMinExcAs("exampleKey3", Types.STRING) == "10"
        assert schema.getMinExcAs("exampleKey4", Types.STRING) == "-2.22"
    except Exception as e:
        pytest.fail(e, pytrace=True)

    try:
        assert schema.getMaxExcAs("exampleKey3", Types.STRING) == "20"
        assert schema.getMaxExcAs("exampleKey4", Types.STRING) == "5.55"
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_setMinExcMaxExc():
    try:
        schema = cppSomeClassSchemaSomeClassId()
        assert schema.getMinExc("y") == 0
        assert schema.getMaxExc("y") == 29
        schema.setMinExc("y", 2)
        schema.setMaxExc("y", 30)
        assert schema.getMinExc("y") == 2
        assert schema.getMaxExc("y") == 30
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_hasgetsetMinMaxSize():
    schema = cppTestStruct1SchemaTestStruct1()
    assert schema.hasMinSize('exampleKey10')
    assert schema.hasMaxSize('exampleKey10')
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
    assert schema.hasMinSize('exampleKey11')
    assert schema.hasMaxSize('exampleKey11')
    assert schema.getMinSize('exampleKey11') == 1
    assert schema.getMaxSize('exampleKey11') == 42


def test_hasgetsetMinMax():
    schema = cppOtherSchemaElementsSchemaOtherSchemaElements()
    assert schema.hasMin('shapeList') is False
    assert schema.hasMax('shapeList') is False
    schema.setMin('shapeList', 1)
    schema.setMax('shapeList', 5)
    assert schema.hasMin('shapeList')
    assert schema.hasMax('shapeList')
    assert schema.getMin('shapeList') == 1
    assert schema.getMax('shapeList') == 5


def test_getWarnAlarmLowHigh():
    schema = cppTestStruct1SchemaTestStruct1()
    try:
        assert schema.getWarnLow("exampleKey5") == -10
        assert schema.getWarnLow("exampleKey6") == -5.5
    except Exception as e:
        pytest.fail(e, pytrace=True)

    try:
        assert schema.getWarnHigh("exampleKey5") == 10
        assert schema.getWarnHigh("exampleKey6") == 5.5
    except Exception as e:
        pytest.fail(e, pytrace=True)

    try:
        assert schema.getAlarmLow("exampleKey5") == -20
        assert schema.getAlarmLow("exampleKey6") == -22.1
    except Exception as e:
        pytest.fail(e, pytrace=True)

    try:
        assert schema.getAlarmHigh("exampleKey5") == 20
        assert schema.getAlarmHigh("exampleKey6") == 22.777
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_getWarnAlarmLowHighAs():
    schema = cppTestStruct1SchemaTestStruct1()
    try:
        assert schema.getWarnLowAs("exampleKey5", Types.STRING) == "-10"
        assert schema.getWarnLowAs("exampleKey6", Types.STRING) == "-5.5"
    except Exception as e:
        pytest.fail(e, pytrace=True)

    try:
        assert schema.getWarnHighAs("exampleKey5", Types.STRING) == "10"
        assert schema.getWarnHighAs("exampleKey6", Types.STRING) == "5.5"
    except Exception as e:
        pytest.fail(e, pytrace=True)

    try:
        assert schema.getAlarmLowAs("exampleKey5", Types.STRING) == "-20"
        assert schema.getAlarmLowAs("exampleKey6", Types.STRING) == "-22.1"
    except Exception as e:
        pytest.fail(e, pytrace=True)

    try:
        assert schema.getAlarmHighAs("exampleKey5", Types.STRING) == "20"
        assert schema.getAlarmHighAs("exampleKey6", Types.STRING) == "22.777"
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_hasWarnAlarm():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        assert schema.hasWarnLow("exampleKey5")
        assert schema.hasWarnHigh("exampleKey5")

        assert schema.hasWarnLow("exampleKey6")
        assert schema.hasWarnHigh("exampleKey6")
        assert schema.hasAlarmLow("exampleKey6")
        assert schema.hasAlarmHigh("exampleKey6")

        assert schema.hasAlarmHigh("exampleKey1") is False
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_vectorElement():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        assert schema.isAccessReadOnly("exampleKey7")
        assert schema.hasDefaultValue("exampleKey7")
        assert schema.hasDefaultValue("exampleKey10")
        assert schema.hasDefaultValue("exampleKey11")
        assert schema.hasDefaultValue("exampleKey12")
        assert schema.hasDefaultValue("exampleKey14")
        assert schema.hasDefaultValue("exampleKey15")

        assert schema.getDefaultValue("exampleKey10") == [10, 20, 30]
        assert schema.getDefaultValue("exampleKey12") == [1.1, -2.2, 3.3]
        assert schema.getDefaultValue("exampleKey11") == [10, 20, 30]
        assert schema.getDefaultValueAs("exampleKey11", Types.STRING) == \
            "10,20,30"
        assert schema.getDefaultValueAs("exampleKey11",
                                        Types.VECTOR_INT32) == [10, 20, 30]
        assert schema.getDefaultValue("exampleKey14") == \
            ["Hallo", "World", "Test"]
        assert schema.getDefaultValueAs("exampleKey14", Types.STRING) == \
            "Hallo,World,Test"
        assert schema.getDefaultValue("exampleKey15") == \
            ["word1", "word2", "test"]
        assert schema.getDefaultValueAs("exampleKey15", Types.STRING) == \
            "word1,word2,test"
        assert schema.getMinSize("exampleKey10") == 2
        assert schema.getMaxSize("exampleKey10") == 7
    except Exception as e:
        pytest.fail(e, pytrace=True)

    try:
        schema = cppSomeClassSchemaSomeClassId()
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
        assert validated is not None
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_ndarrayElement():
    schema = Schema()
    (
        NDARRAY_ELEMENT(schema).key("example16")
        .dtype("BOOL")
        .shape([2, 3])
        .unit(Unit.DEGREE_CELSIUS)
        .metricPrefix(MetricPrefix.CENTI)
        .skipValidation()
        .commit(),

        NDARRAY_ELEMENT(schema).key("example17")
        .dtype("UINT32")
        .shape([2, 5, 0])
        .skipValidation()
        .commit(),
        # NOTE: validation is skipped for NDArray even when not explicitly
        # asked for. However, this will change in the future.
        NDARRAY_ELEMENT(schema).key("example18")
        .dtype("FLOAT")
        .shape("3,2,1")
        .commit()
    )

    assert schema.getDefaultValue("example16.shape") == [2, 3]
    assert schema.getDefaultValue("example17.shape") == [2, 5, 0]
    assert schema.getDefaultValue("example18.shape") == [3, 2, 1]
    assert schema.getUnit("example16.data") == Unit.DEGREE_CELSIUS
    assert schema.getMetricPrefix("example16.data") == MetricPrefix.CENTI

    assert schema.isAccessReadOnly("example16")
    assert schema.isAccessReadOnly("example17")
    assert schema.isAccessReadOnly("example18")
    assert schema.getSkipValidation("example16")
    assert schema.getSkipValidation("example17")
    assert not schema.getSkipValidation("example18")


# def test_listElement():
#     schema = \
#       Configurator(GraphicsRenderer2).getSchema("GraphicsRenderer2")
#     assert schema.getDisplayType("chars") == "unitTest"
#     assert schema.isAccessReconfigurable("chars") is True


def test_getDisplayType():
    schema = cppTestStruct1SchemaTestStruct1()
    assert schema.getDisplayType("exampleBitsKey1") == "bin"
    assert schema.getDisplayType("exampleBitsKey2") == \
        "bin|10:In Error, 21:Busy, 35:HV On, 55:Crate On"
    assert schema.getDisplayType("exampleBitsKey3") == "oct"
    assert schema.getDisplayType("exampleBitsKey4") == "hex"
    assert schema.getDisplayType("exampleKey5") == "Int64DisplayType"
    assert schema.getDisplayType("exampleKey8") == "Curve"
    assert schema.getDisplayType("exampleKey9") == "TestDisplayType"


def test_setDisplayType():
    try:
        schema = cppSomeClassSchemaSomeClassId()
        assert schema.hasDisplayType('y') is False
        schema.setDisplayType('y', 'blabla')
        assert schema.hasDisplayType('y')
        assert schema.getDisplayType("y") == "blabla"
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_isCommand():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        assert schema.isCommand("slotTest")
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_isProperty():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        assert schema.isProperty("slotTest") is False
        assert schema.isProperty("testPath2")
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_hasArchivePolicy():
    schema = cppTestStruct1SchemaTestStruct1()
    assert schema.hasArchivePolicy("exampleKey5") is False
    assert schema.hasArchivePolicy("exampleKey6")
    assert schema.hasArchivePolicy("exampleKey7")
    assert schema.hasArchivePolicy("exampleKey8")


def test_getArchivePolicy():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        assert schema.getArchivePolicy("exampleKey6") == \
            ArchivePolicy.EVERY_100MS
        assert schema.getArchivePolicy("exampleKey7") == \
            ArchivePolicy.EVERY_1S
        assert schema.getArchivePolicy("exampleKey8") == \
            ArchivePolicy.NO_ARCHIVING
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_setArchivePolicy():
    try:
        schema = cppSomeClassSchemaSomeClassId()
        assert schema.getArchivePolicy("a") == ArchivePolicy.EVERY_100MS
        schema.setArchivePolicy('a', ArchivePolicy.EVERY_10MIN)
        assert schema.getArchivePolicy("a") == ArchivePolicy.EVERY_10MIN
    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_perKeyFunctionality():
    try:
        schema = cppTestStruct1SchemaTestStruct1()
        keys = schema.getKeys()
        for key in keys:
            if key == "exampleKey1":
                assert schema.hasAssignment(key)
                assert schema.isAssignmentOptional(key)
                assert schema.hasDefaultValue(key)
                assert schema.hasAccessMode(key)
                assert schema.isAccessReconfigurable(key)
                assert schema.hasOptions(key)
                assert schema.hasTags(key)
                assert schema.hasUnit(key) is False
                assert schema.hasMetricPrefix(key) is False

            if key == "exampleKey2":
                assert schema.hasDefaultValue(key)
                assert schema.hasAccessMode(key)
                assert schema.isAccessInitOnly(key)
                assert schema.hasOptions(key)
                assert schema.hasTags(key)
                assert schema.hasAllowedStates(key) is False
                assert schema.hasUnit(key)
                assert schema.hasMetricPrefix(key)
                assert schema.hasMinInc(key)
                assert schema.hasMaxInc(key)

            if key == "exampleKey3":
                assert schema.hasAssignment(key)
                assert schema.isAssignmentMandatory(key)
                assert schema.hasDefaultValue(key) is False
                assert schema.hasOptions(key) is False
                assert schema.hasAllowedStates(key)
                assert schema.hasMinExc(key)
                assert schema.hasMaxExc(key)

            if key == "exampleKey4":
                assert schema.hasDefaultValue(key) is False
                assert schema.isAssignmentInternal(key)
                assert schema.hasAccessMode(key)
                assert schema.isAccessInitOnly(key)

            if key == "exampleKey5":
                assert schema.hasDefaultValue(key)
                assert schema.hasAssignment(key)
                assert schema.isAssignmentOptional(key)
                assert schema.hasAccessMode(key)
                assert schema.isAccessReadOnly(key)

    except Exception as e:
        pytest.fail(e, pytrace=True)


def test_merge():
    schema = Schema()
    (
        STRING_ELEMENT(schema).key("a")
        .description("a").displayedName("a")
        .assignmentOptional().defaultValue("a value")
        .tags("CY,CY,NC,JS,KW,NC")
        .commit(),

    )
    assert "a" in schema
    assert "x" not in schema
    assert "y" not in schema
    assert "z" not in schema

    schema2 = Schema()
    (
        STRING_ELEMENT(schema2).key("x")
        .description("x")
        .displayedName("x")
        .assignmentOptional().defaultValue("a value")
        .tags("LM,BH")
        .commit(),

        STRING_ELEMENT(schema2).key("y")
        .tags("CY")
        .displayedName("Example key 1")
        .description("Example key 1 description")
        .options("Radio,Air Condition,Navigation", ",")
        .assignmentOptional().defaultValue("Radio")
        .reconfigurable()
        .commit(),

        INT32_ELEMENT(schema2).key("z").alias(10)
        .tags("CY,LM,KW")
        .displayedName("Example key 2")
        .description("Example key 2 description")
        .options("5, 25, 10")
        .minInc(5).maxInc(25).unit(AMPERE).metricPrefix(MILLI)
        .assignmentOptional().defaultValue(10)
        .init()
        .commit(),
    )
    assert "x" in schema2
    assert "y" in schema2
    assert "z" in schema2

    schema += schema2

    assert "a" in schema
    assert "x" in schema
    assert "y" in schema
    assert "z" in schema


def test_logger():
    config = Hash("priority", "DEBUG")
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


def test_helpFunction():
    pass
    # uncomment to see help:
    # schema = TestStruct1.getSchema("TestStruct1")
    # schema.help()


def test_schemaImageElement():
    schema = Schema()
    (
        IMAGEDATA_ELEMENT(schema).key("myImage1")
        .displayedName("myImage")
        .description("Image1 Element")
        .setDimensions("100, 200")
        .operatorAccess()
        .commit(),

        IMAGEDATA_ELEMENT(schema).key("myImage2")
        .displayedName("myImage2")
        .description("Image2 Element")
        .setDimensions([110, 210])
        .operatorAccess()
        .commit(),
    )
    assert schema.getDisplayType("myImage1") == "ImageData"
    assert schema.isCustomNode("myImage1") is True
    assert schema.getCustomNodeClass("myImage1") == "ImageData"
    assert schema.getAccessMode("myImage1") == AccessType.READ
    assert schema.getNodeType("myImage1") == NodeType.NODE
    assert schema.getRequiredAccessLevel("myImage1") == AccessLevel.OPERATOR
    assert schema.getDisplayedName("myImage1") == "myImage"
    assert schema.getDescription("myImage1") == "Image1 Element"
    assert schema.getDefaultValue("myImage1.dims") == [100, 200]
    assert schema.getDefaultValue("myImage2.dims") == [110, 210]

    ide = IMAGEDATA_ELEMENT(schema).key("moreImageElement")
    with pytest.raises(RuntimeError):
        ide.setDimensions(123)  # only list ans str are allowed

    assert schema.getDescription("myImage1.pixels") == \
        "The N-dimensional array containing the pixels"
    assert schema.getValueType("myImage1.pixels.data") == Types.BYTE_ARRAY
    assert schema.getDisplayedName("myImage1.dims") == "Dimensions"
    assert schema.getValueType("myImage1.dims") == Types.VECTOR_UINT64
    assert schema.getDisplayType("myImage1.dims") == "Curve"
    assert schema.getDisplayedName("myImage1.encoding") == "Encoding"
    assert schema.getValueType("myImage1.encoding") == Types.INT32

    assert schema.getDisplayedName("myImage1.pixels.isBigEndian") == \
        "Is big-endian"
    assert schema.getValueType("myImage1.pixels.isBigEndian") == Types.BOOL
    assert schema.getDefaultValue("myImage1.pixels.isBigEndian") is False


def test_alarm_info():
    schema = cppTestStruct1SchemaTestStruct1()
    assert schema.getInfoForAlarm("exampleKey6", AlarmCondition.WARN_LOW) == \
        "Some info"


def test_allowed_states():
    # The 'states' argument is tuple
    schema = cppTestStruct1SchemaTestStruct1()
    schema.setAllowedStates("exampleKey3", (State.ACQUIRING, State.NORMAL,
                            State.PASSIVE))
    allowedStates = schema.getAllowedStates("exampleKey3")
    assert allowedStates == [State.ACQUIRING, State.NORMAL, State.PASSIVE]
    # The 'states' argument is list
    schema = cppTestStruct1SchemaTestStruct1()
    schema.setAllowedStates("exampleKey3", [State.ACQUIRING, State.NORMAL,
                                            State.PASSIVE])
    allowedStates = schema.getAllowedStates("exampleKey3")
    assert allowedStates == [State.ACQUIRING, State.NORMAL, State.PASSIVE]
    # The 'states' argument is a 'set'
    schema = cppTestStruct1SchemaTestStruct1()
    schema.setAllowedStates("exampleKey3", {State.ACQUIRING, State.NORMAL,
                                            State.PASSIVE})
    allowedStates = schema.getAllowedStates("exampleKey3")
    # the insertion order is not guaranteed for python 'set' ...
    assert State.ACQUIRING in allowedStates
    assert State.NORMAL in allowedStates
    assert State.PASSIVE in allowedStates
    # The 'states' are *args
    schema = cppTestStruct1SchemaTestStruct1()
    schema.setAllowedStates("exampleKey3", State.ACQUIRING, State.NORMAL,
                            State.PASSIVE)
    allowedStates = schema.getAllowedStates("exampleKey3")
    assert allowedStates == [State.ACQUIRING, State.NORMAL, State.PASSIVE]


def test_daq_data_type():
    schema = Schema()

    NODE_ELEMENT(schema).key(
        "trainData").commit()  # Has no DAQ data type yet
    NODE_ELEMENT(schema).key("pulseData").setDaqDataType(
        DaqDataType.PULSE).commit()

    assert schema.hasDaqDataType("trainData") is False
    assert schema.hasDaqDataType("pulseData") is True
    assert schema.getDaqDataType("pulseData") == DaqDataType.PULSE

    # Now add DAQ data type to node "trainData"
    schema.setDaqDataType("trainData", DaqDataType.TRAIN)
    assert schema.hasDaqDataType("trainData")
    assert schema.getDaqDataType("trainData") == DaqDataType.TRAIN


def test_state_element():
    schema = Schema()
    (
        STATE_ELEMENT(schema)
        .key("state1")
        .commit(),

        STATE_ELEMENT(schema)
        .key("state2")
        .daqPolicy(DAQPolicy.SAVE)
        .commit(),

        STATE_ELEMENT(schema)
        .key("state3")
        .daqPolicy(DAQPolicy.OMIT)
        .commit()
    )

    assert schema.getDAQPolicy("state1") == DAQPolicy.UNSPECIFIED
    assert schema.getDAQPolicy("state2"), DAQPolicy.SAVE
    assert schema.getDAQPolicy("state3"), DAQPolicy.OMIT


def test_table_element():
    schema = cppTestStruct1SchemaTestStruct1()

    # Both, defaultValue and initialValue should work for read-only tables
    default = schema.getDefaultValue("tableI")
    assert len(default) == 1
    assert fullyEqual(default[0], Hash("int", 2))
    default = schema.getDefaultValue("tableD")
    assert len(default) == 1
    assert fullyEqual(default[0], Hash("int", 3))

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
        .commit()
    )

    assert s.hasAllowedActions("node")
    assert not s.hasAllowedActions("node.int")
    actions = s.getAllowedActions("node")
    assert len(actions) == 2
    assert actions[0] == "action1"
    assert actions[1] == "action2"

    assert s.hasAllowedActions("node.arr")
    actions = s.getAllowedActions("node.arr")
    assert len(actions) == 1
    assert actions[0] == "otherAction"

    assert s.hasAllowedActions("image")
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
