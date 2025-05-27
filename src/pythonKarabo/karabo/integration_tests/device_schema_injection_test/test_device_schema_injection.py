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

from karabo.bound import (
    DOUBLE_ELEMENT, INT32_ELEMENT, Configurator, Hash, PythonDevice, Schema)

from .device_with_limit import DeviceWithLimit
from .device_with_table_parameter import DeviceWithTableElementParam


@pytest.fixture(scope="function")
def device_cfg():
    return Hash('log.level', "FATAL")


def test_schema_injection(device_cfg):
    device = Configurator(PythonDevice).create("PythonDevice", device_cfg)
    device.startInitialFunctions()

    schema = Schema()
    INT32_ELEMENT(schema).key(
        "injectedInt32").assignmentOptional().defaultValue(
        1).reconfigurable().commit()
    device.appendSchema(schema)

    assert device.get("injectedInt32") == 1
    device.set("injectedInt32", 5)
    assert device.get("injectedInt32") == 5

    with pytest.raises(RuntimeError):
        device.set("unknownProperty", 42)
    assert "unknownProperty" not in device.getCurrentConfiguration().getPaths()

    schema = Schema()
    INT32_ELEMENT(schema).key(
        "injectedInt32").assignmentOptional().defaultValue(
        2).reconfigurable().minInc(1).commit()
    device.appendSchema(schema)
    assert device.get("injectedInt32") == 5
    assert device.getFullSchema().getMinInc("injectedInt32") == 1

    schema = Schema()
    INT32_ELEMENT(schema).key(
        "injectedInt32").assignmentOptional().defaultValue(
        3).reconfigurable().minInc(2).maxInc(10).commit()
    device.updateSchema(schema)
    assert device.get("injectedInt32") == 5
    assert device.getFullSchema().getMinInc("injectedInt32") == 2
    assert device.getFullSchema().getMaxInc("injectedInt32") == 10

    schema = Schema()
    INT32_ELEMENT(schema).key(
        "somethingElse").assignmentOptional().defaultValue(
        4).reconfigurable().commit()
    device.updateSchema(schema)
    config = device.getCurrentConfiguration()
    assert "injectedInt32" not in config.getPaths()
    assert "somethingElse" in config.getPaths()

    device.set("somethingElse", 42)
    schema = Schema()
    INT32_ELEMENT(schema).key(
        "somethingElse").assignmentOptional().defaultValue(
        5).reconfigurable().commit()
    device.updateSchema(schema)

    schema = Schema()
    INT32_ELEMENT(schema).key("somethingElse").readOnly().commit()
    device.updateSchema(schema)

    schema = Schema()
    INT32_ELEMENT(schema).key(
        "somethingElse").assignmentOptional().defaultValue(5).minInc(
        3).reconfigurable().commit()
    device.updateSchema(schema)
    assert device.get("somethingElse") == 42

    schema = Schema()
    device.updateSchema(schema)
    assert "somethingElse" not in device.getCurrentConfiguration().getPaths()
    assert "somethingElse" not in device.getFullSchema().getPaths()
    assert device.getFullSchema().getPaths() == PythonDevice.getSchema(
        device.classid).getPaths()

    schema = Schema()
    INT32_ELEMENT(schema).key(
        "somethingElse").assignmentOptional().defaultValue(
        5).reconfigurable().commit()
    device.appendSchema(schema)
    device.set("somethingElse", 42)

    schema = Schema()
    INT32_ELEMENT(schema).key("somethingElse").readOnly().commit()
    device.appendSchema(schema)

    schema = Schema()
    INT32_ELEMENT(schema).key(
        "somethingElse").assignmentOptional().defaultValue(5).minInc(
        3).reconfigurable().commit()
    device.appendSchema(schema)
    assert device.get("somethingElse") == 42

    for idx in range(10):
        schema = Schema()
        INT32_ELEMENT(schema).key(
            f"property{idx}").assignmentOptional().defaultValue(
            idx).reconfigurable().commit()
        device.appendSchema(schema)

    for idx in range(10):
        key = f"property{idx}"
        assert key in device.getCurrentConfiguration().getPaths()
        assert key in device.getFullSchema().getPaths()
        assert device.get(key) == idx


def test_schema_with_table_element_update(device_cfg):
    device = Configurator(PythonDevice).create("DeviceWithTableElementParam",
                                               device_cfg)
    assert isinstance(device, DeviceWithTableElementParam)
    device.startInitialFunctions()

    assert "deviceTable" in device.getSchema(
        "DeviceWithTableElementParam").getPaths()

    schema = Schema()
    INT32_ELEMENT(schema).key(
        "somethingNew").assignmentOptional().defaultValue(
        4).reconfigurable().commit()
    device.updateSchema(schema)

    assert "deviceTable" in device.getSchema(
        "DeviceWithTableElementParam").getPaths()
    table = device.get("deviceTable")
    assert len(table) == 2


def test_schema_with_table_element_append(device_cfg):
    device = Configurator(PythonDevice).create("DeviceWithTableElementParam",
                                               device_cfg)
    device.startInitialFunctions()

    assert "deviceTable" in device.getSchema(
        "DeviceWithTableElementParam").getPaths()

    schema = Schema()
    INT32_ELEMENT(schema).key(
        "somethingNew").assignmentOptional().defaultValue(
        4).reconfigurable().commit()
    device.appendSchema(schema)

    assert "deviceTable" in device.getSchema(
        "DeviceWithTableElementParam").getPaths()
    table = device.get("deviceTable")
    assert len(table) == 2


def test_schema_with_attribute_update(device_cfg):
    device = Configurator(PythonDevice).create("DeviceWithLimit", device_cfg)
    device.startInitialFunctions()

    schema = Schema()
    max_exc = 2 * DeviceWithLimit.LIMIT_HIGH
    DOUBLE_ELEMENT(schema).key(
        "valueWithExc").assignmentOptional().defaultValue(
        0.0).reconfigurable().maxExc(max_exc).commit()
    device.updateSchema(schema)
    assert device.getFullSchema().getMaxExc("valueWithExc") == max_exc

    schema = Schema()
    INT32_ELEMENT(schema).key(
        "somethingNew").assignmentOptional().defaultValue(
        4).reconfigurable().commit()
    device.updateSchema(schema)

    assert device.getFullSchema().getMaxExc(
        "valueWithExc") == DeviceWithLimit.LIMIT_HIGH


def test_schema_with_attribute_append(device_cfg):
    device = Configurator(PythonDevice).create("DeviceWithLimit", device_cfg)
    device.startInitialFunctions()

    schema = Schema()
    max_exc = 2 * DeviceWithLimit.LIMIT_HIGH
    DOUBLE_ELEMENT(schema).key(
        "valueWithExc").assignmentOptional().defaultValue(0.0).maxExc(
        max_exc).reconfigurable().commit()
    device.updateSchema(schema)
    assert device.getFullSchema().getMaxExc("valueWithExc") == max_exc

    schema = Schema()
    INT32_ELEMENT(schema).key(
        "somethingNew").assignmentOptional().defaultValue(
        4).reconfigurable().commit()
    device.appendSchema(schema)
    assert device.getFullSchema().getMaxExc("valueWithExc") == max_exc


def test_ensure_get_copies(device_cfg):
    device = Configurator(PythonDevice).create("DeviceWithLimit", device_cfg)
    device.startInitialFunctions()

    vec = device.get("vector")
    assert vec == [0]
    vec[0] = 1
    assert vec == [1]
    assert device.get("vector")[0] == 0

    node = device.get("node")
    assert node["number"] == 0
    node["number"] = 100
    assert node["number"] == 100
    assert device.get("node.number") == 0

    table = device.get("table")
    assert len(table) == 2
    assert table[0]["int32"] == 1
    assert table[1]["int32"] == 2

    table[1]["int32"] = 3
    assert table[1]["int32"] == 3
    assert device.get("table")[1]["int32"] == 2

    table[0] = Hash("int32", -1)
    assert table[0]["int32"] == -1
    assert device.get("table")[0]["int32"] == 1

    table.append(Hash("int32", 99))
    assert len(table) == 3
    assert len(device.get("table")) == 2

    schema = device.getFullSchema()
    INT32_ELEMENT(schema).key(
        "aNewKeyNotExisting").assignmentOptional().defaultValue(1).commit()
    assert schema.has("aNewKeyNotExisting")
    assert not device.getFullSchema().has("aNewKeyNotExisting")


def test_set_vector_update(device_cfg):
    device_cfg["vector"] = [1, 2, 3]
    device = Configurator(PythonDevice).create("DeviceWithLimit", device_cfg)
    device.startInitialFunctions()
    del device_cfg["vector"]

    assert device.get("vector") == [1, 2, 3]

    t = device.getActualTimestamp()
    device.setVectorUpdate("vector", [3, 3, 1], "add", t)
    assert device.get("vector") == [1, 2, 3, 3, 3, 1]

    device.setVectorUpdate("vector", [1, 7], "addIfNotIn", t)
    assert device.get("vector") == [1, 2, 3, 3, 3, 1, 7]

    device.setVectorUpdate("vector", [3, 1, -99], "removeOne")
    assert device.get("vector") == [2, 3, 3, 1, 7]

    device.setVectorUpdate("vector", [2, -99, 3], "removeAll")
    assert device.get("vector") == [1, 7]

    with pytest.raises(ValueError):
        device.setVectorUpdate("vector", [0], "typo", t)
