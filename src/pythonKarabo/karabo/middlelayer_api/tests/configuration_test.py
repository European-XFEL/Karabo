from copy import deepcopy

import numpy as np

from karabo.common.api import KARABO_SCHEMA_DISPLAYED_NAME, KARABO_WARN_HIGH
from karabo.middlelayer_api.configuration import (
    attr_fast_deepcopy, config_changes, extract_modified_schema_attributes,
    is_equal, sanitize_init_configuration, sanitize_write_configuration,
    validate_init_configuration)
from karabo.native import (
    AccessMode, Assignment, Configurable, Double, Hash, Int32, MetricPrefix,
    Node, Overwrite, Schema, Slot, Unit)


class Nested(Configurable):
    double = Double(
        defaultValue=2.0,
        minInc=-10.0, maxInc=10.0, absoluteError=0.5,
        accessMode=AccessMode.RECONFIGURABLE)

    readOnlyInteger = Int32(
        defaultValue=20,
        accessMode=AccessMode.READONLY)


class Object(Configurable):
    double = Double(
        defaultValue=2.0,
        minInc=-10.0, maxInc=10.0, absoluteError=0.5,
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
    assert "move" in config
    assert "move" not in sanitized

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
    assert "move" in run_time_conf
    assert "move" not in sanitized


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


def test_attr_fast_deepcopy():
    def _safe_compare(a, b):
        # Use repr() to get around the lack of Schema comparison
        return len(a) == len(b) and all(repr(a[k]) == repr(b[k]) for k in a)

    d = {
        'a': [1, 2, 3],
        'b': {'sub': 42},
        'c': 'Hi there!',
        'd': np.zeros((10,)),
        'e': (1, 2, 3),
        'f': Hash('simple', 32),
        'g': Schema()
    }
    copy = attr_fast_deepcopy(d)
    assert _safe_compare(copy, d)

    d0 = {
        KARABO_SCHEMA_DISPLAYED_NAME: 'foo',
        KARABO_WARN_HIGH: 0,
    }
    ref = {
        KARABO_SCHEMA_DISPLAYED_NAME: 'bar',
        KARABO_WARN_HIGH: 1,
    }
    # get diff between d0 and ref, KARABO_SCHEMA_DISPLAYED_NAME is not in
    # KARABO_EDITABLE_ATTRIBUTES, should not be included in the diff
    diff = attr_fast_deepcopy(d0, ref)
    assert diff == {KARABO_WARN_HIGH: 0}


def test_attribute_schema_extract():

    class Offline(Configurable):
        double = Double(
            defaultValue=2.0,
            minInc=-10.0, maxInc=10.0, absoluteError=0.5,
            accessMode=AccessMode.RECONFIGURABLE)

        readOnlyDouble = Double(
            defaultValue=2.0,
            warnLow=-10.0, warnHigh=10.0,
            accessMode=AccessMode.READONLY)

        readOnlyInteger = Int32(
            defaultValue=20,
            accessMode=AccessMode.READONLY)

        integer = Int32(
            defaultValue=20,
            unitSymbol=Unit.METER,
            metricPrefixSymbol=MetricPrefix.MICRO,
            accessMode=AccessMode.RECONFIGURABLE)

    class Online(Offline):
        integer = Overwrite(
            unitSymbol=Unit.METER,
            metricPrefixSymbol=MetricPrefix.MILLI)

        onlyOnline = Int32(
            defaultValue=20,
            unitSymbol=Unit.METER,
            metricPrefixSymbol=MetricPrefix.MICRO,
            accessMode=AccessMode.RECONFIGURABLE)

        readOnlyDouble = Overwrite(
            warnLow=-20.0, warnHigh=15.0)

    offline_schema = Offline.getClassSchema()
    online_schema = Online.getClassSchema()

    attrs = extract_modified_schema_attributes(
        online_schema, offline_schema)
    assert attrs is not None
    assert len(attrs) == 2
    assert attrs[0] == Hash('path', 'readOnlyDouble',
                            'attribute', "warnHigh",
                            'value', 15)
    assert attrs[1] == Hash('path', 'readOnlyDouble',
                            'attribute', "warnLow",
                            'value', -20)


def test_array_equal():
    assert is_equal(np.array([1, 2, 3]), np.array([1, 2, 3]))
