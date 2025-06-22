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
from copy import deepcopy

from karabo.middlelayer.configuration import (
    config_changes, extract_init_configuration, sanitize_init_configuration,
    sanitize_write_configuration, validate_init_configuration)
from karabo.middlelayer.macro import MacroSlot
from karabo.native import (
    AccessMode, Assignment, Configurable, Double, Hash, Int32, Node, Schema,
    Slot)


class Nested(Configurable):
    double = Double(
        defaultValue=2.0,
        minInc=-10.0, maxInc=10.0,
        accessMode=AccessMode.RECONFIGURABLE)

    readOnlyInteger = Int32(
        defaultValue=20,
        accessMode=AccessMode.READONLY)


class Object(Configurable):
    double = Double(
        defaultValue=2.0,
        minInc=-10.0, maxInc=10.0,
        accessMode=AccessMode.RECONFIGURABLE)

    readOnlyInteger = Int32(
        defaultValue=20,
        accessMode=AccessMode.READONLY)

    internalInteger = Int32(
        defaultValue=20,
        assignment=Assignment.INTERNAL)

    integer = Int32(
        defaultValue=20,
        accessMode=AccessMode.RECONFIGURABLE)

    integerWithOptions = Int32(
        defaultValue=2,
        options=[1, 2, 3, 4],
        accessMode=AccessMode.RECONFIGURABLE)

    initOnlyDouble = Double(
        defaultValue=100.0,
        accessMode=AccessMode.INITONLY)

    internalInitOnlyDouble = Double(
        defaultValue=30.0,
        accessMode=AccessMode.INITONLY,
        assignment=Assignment.INTERNAL)

    nested = Node(Nested)

    @Slot()
    async def move(self):
        """Dummy move slot"""

    @MacroSlot()
    async def moveMacro(self):
        """Dummy move macro slot"""


def test_validate_init_configuration():
    """Test if we can validate a configuration"""
    obj = Object()
    config = Hash("double", 2.0, "readOnlyInteger", 5)
    text = validate_init_configuration(obj.getClassSchema(), config)
    assert config["double"] == 2.0
    # Read only is erase
    assert "readOnlyInteger" not in config
    assert text == ""

    # Test invalid path in config
    config = Hash("definatelyNotSchema", 2.0, "double", 1.0)
    text = validate_init_configuration(obj.getClassSchema(), config)
    assert "definatelyNotSchema" not in config
    assert config["double"] == 1.0
    assert text != ""

    # Nested - read only
    config = Hash("nested.double", 2.0, "nested.readOnlyInteger", 5)
    text = validate_init_configuration(obj.getClassSchema(), config)
    assert config["nested.double"] == 2.0
    assert "nested.readOnlyInteger" not in config
    assert text == ""

    # Nested - Test invalid path in config
    config = Hash("nested.definatelyNotSchema", 2.0, "nested.double", 1.0)
    text = validate_init_configuration(obj.getClassSchema(), config)
    assert "nested.definatelyNotSchema" not in config
    assert config["nested.double"] == 1.0
    assert text != ""

    # Nested - Test invalid path in config
    config = Hash("nested.definatelyNotSchema", 2.0, "double", 1.0)
    text = validate_init_configuration(obj.getClassSchema(), config)
    assert "nested.definatelyNotSchema" not in config
    assert "nested" not in config
    assert config["double"] == 1.0
    assert text != ""


def test_sanitize_init_configuration():
    """Test if we can sanitize a configuration"""
    obj = Object()
    config = obj.configurationAsHash()
    schema = Object.getClassSchema()
    assert isinstance(schema, Schema)
    # Make a deepcopy for testing!
    sanitized = sanitize_init_configuration(schema, deepcopy(config))
    assert config is not None
    assert sanitized is not None
    assert "integer" in sanitized
    assert "internalInteger" in config
    assert "internalInteger" not in sanitized
    assert "readOnlyInteger" in config
    assert "readOnlyInteger" not in sanitized
    assert "double" in config
    assert "double" in sanitized
    assert "nested.readOnlyInteger" in config
    assert "nested.readOnlyInteger" not in sanitized
    assert "nested.double" in config
    assert "nested.double" in sanitized
    assert "initOnlyDouble" in sanitized
    assert "internalInitOnlyDouble" in config
    assert "internalInitOnlyDouble" not in sanitized

    # No Slots in configuration
    assert "move" not in config
    assert "move" not in sanitized
    assert "moveMacro" not in config
    assert "moveMacro" not in sanitized

    # ------------------------------------------------------------
    # Check runtime configuration!
    run_time_conf = deepcopy(config)
    # We are evil no, we provide a value that is not in the options
    # This can happen if we have an outdated configuration not
    # matching the schema
    run_time_conf["integerWithOptions"] = 20
    sanitized = sanitize_write_configuration(schema, run_time_conf)
    assert sanitized is not None
    assert "integer" in sanitized
    assert "internalInteger" in run_time_conf
    assert "internalInteger" in sanitized
    assert "readOnlyInteger" in run_time_conf
    assert "readOnlyInteger" not in sanitized
    assert "double" in run_time_conf
    assert "double" in sanitized
    assert "nested.readOnlyInteger" in run_time_conf
    assert "nested.readOnlyInteger" not in sanitized
    assert "nested.double" in run_time_conf
    assert "nested.double" in sanitized
    assert "initOnlyDouble" in run_time_conf
    assert "initOnlyDouble" not in sanitized
    assert "internalInitOnlyDouble" in run_time_conf
    assert "internalInitOnlyDouble" not in sanitized
    assert "integerWithOptions" in run_time_conf
    assert "integerWithOptions" not in sanitized

    # No Slots in configuration
    assert "move" not in run_time_conf
    assert "move" not in sanitized
    assert "moveMacro" not in config
    assert "moveMacro" not in sanitized


def test_extract_init_configuration():
    obj = Object()
    config = obj.configurationAsHash()
    schema = Object.getClassSchema()
    assert isinstance(schema, Schema)
    # Make a deepcopy for testing!
    extracted = extract_init_configuration(schema, deepcopy(config))
    assert config is not None
    assert extracted is not None
    # No changes from default
    assert len(extracted) == 0

    # modify
    config = obj.configurationAsHash()
    config["double"] = 0.1
    config["integer"] = 20  # default
    config["internalInteger"] = 2
    config["initOnlyDouble"] = 55.1
    config["nested.double"] = 31.2
    extracted = extract_init_configuration(schema, deepcopy(config))
    assert len(extracted) == 3
    assert "double" in extracted
    assert "internalInteger" not in extracted
    assert "integer" not in extracted
    assert "initOnlyDouble" in extracted
    assert "nested.double" in extracted


def test_config_changes():
    obj = Object()
    device_a = obj.configurationAsHash()
    device_b = obj.configurationAsHash()
    changes = config_changes(device_a, device_b)
    assert changes == Hash()
    device_b["double"] = 4.7
    changes = config_changes(device_a, device_b)
    assert changes == Hash("double", [2.0, 4.7])

    # Add properties and check
    device_a["notB"] = True
    device_b["notA"] = True
    changes = config_changes(device_a, device_b)
    assert changes.fullyEqual(
        Hash("double", [2.0, 4.7],
             "notA", [None, True],
             "notB", [True, None]))
