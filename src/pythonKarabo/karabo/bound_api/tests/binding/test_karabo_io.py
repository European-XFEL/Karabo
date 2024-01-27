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
    ALARM_ELEMENT, BOOL_ELEMENT, FLOAT_ELEMENT, KARABO_CLASSINFO,
    KARABO_CONFIGURATION_BASE_CLASS, STATE_ELEMENT, STRING_ELEMENT,
    BinarySerializerHash, BinarySerializerSchema, Hash, MetricPrefix, Schema,
    TextSerializerHash, TextSerializerSchema, Unit, fullyEqual,
    loadHashFromFile, loadSchemaFromFile, saveHashToFile, saveSchemaToFile)
from karabo.common.alarm_conditions import AlarmCondition
from karabo.common.states import State


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("ShapeY", "1.0")
class ShapeY:
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


@KARABO_CLASSINFO("CircleY", "1.0")
class CircleY(ShapeY):
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


def test_serializer_Hash():
    config = Hash("indentation", -1)
    ser = TextSerializerHash.create("Xml", config)
    data = Hash('a.b.c', 1, 'x.y.z', [1, 2, 3, 4, 5, 6, 7])
    archive = ser.save(data)
    assert archive == '<?xml version="1.0"?><root KRB_Artificial="" \
KRB_Type="HASH"><a KRB_Type="HASH"><b KRB_Type="HASH"><c KRB_Type\
="INT32">1</c></b></a><x KRB_Type="HASH"><y KRB_Type="HASH"><z \
KRB_Type="VECTOR_INT32">1,2,3,4,5,6,7</z></y></x></root>'
    assert isinstance(archive, str) is True
    g = ser.load(archive)
    assert isinstance(g, Hash) is True
    assert fullyEqual(g, data) is True
    ser = BinarySerializerHash.create("Bin")
    archive = ser.save(data)
    assert isinstance(archive, bytes) is True
    g = ser.load(archive)
    assert isinstance(g, Hash) is True
    assert fullyEqual(g, data) is True


def test_serializer_schema():
    schema = Schema()
    CircleY.expectedParameters(schema)
    config = Hash("indentation", -1)
    ser = TextSerializerSchema.create("Xml", config)
    archive = ser.save(schema)
    assert isinstance(archive, str) is True
    schema2 = ser.load(archive)
    assert isinstance(schema2, Schema) is True
    assert schema.getParameterHash() == schema2.getParameterHash()
    ser = BinarySerializerSchema.create("Bin")
    archive = ser.save(schema)
    assert isinstance(archive, bytes) is True
    schema2 = ser.load(archive)
    assert isinstance(schema2, Schema) is True
    assert schema2.getParameterHash() == schema.getParameterHash()


def test_file_tools_hash():
    h = Hash("a", 10, "b.c", "Hallo World", "x.y.z", [1, 2, 3, 4, 5])
    saveHashToFile(h, '/tmp/hallo.xml')
    g = loadHashFromFile('/tmp/hallo.xml')
    assert fullyEqual(h, g) is True
    saveHashToFile(h, '/tmp/hallo.bin')
    g = loadHashFromFile('/tmp/hallo.bin')
    assert fullyEqual(h, g) is True


def test_file_tools_schema():
    schema = Schema()
    CircleY.expectedParameters(schema)
    saveSchemaToFile(schema, '/tmp/circle.xml')
    schema2 = loadSchemaFromFile('/tmp/circle.xml')
    assert fullyEqual(schema.getParameterHash(), schema2.getParameterHash())
    saveSchemaToFile(schema, '/tmp/circle.bin')
    schema2 = loadSchemaFromFile('/tmp/circle.bin')
    assert fullyEqual(schema.getParameterHash(), schema2.getParameterHash())
