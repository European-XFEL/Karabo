from copy import deepcopy

import numpy as np

from karabo.common.api import (
    KARABO_SCHEMA_DISPLAYED_NAME, KARABO_SCHEMA_MIN_EXC)
from karabo.native.configuration import (
    attr_fast_deepcopy, sanitize_init_configuration,
    sanitize_write_configuration, extract_modified_schema_attributes)
from karabo.native.data.enums import Unit, MetricPrefix
from karabo.native.data import (
    AccessMode, Assignment, Hash, Configurable, Double, Int32, Node,
    Overwrite, Schema)


def test_sanitize_init_configuration():
    """Test if we can sanitize a configuration"""

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

        initOnlyDouble = Double(
            defaultValue=100.0,
            accessMode=AccessMode.INITONLY)

        internalInitOnlyDouble = Double(
            defaultValue=30.0,
            accessMode=AccessMode.INITONLY,
            assignment=Assignment.INTERNAL)

        nested = Node(Nested)

    obj = Object()
    config = obj.configurationAsHash()
    schema = Object.getClassSchema()
    # Make a deepcopy for testing!
    new_config = sanitize_init_configuration(schema, deepcopy(config))
    assert config is not None
    assert new_config is not None
    assert "integer" in new_config
    assert "internalInteger" in config
    assert "internalInteger" not in new_config
    assert "readOnlyInteger" in config
    assert "readOnlyInteger" not in new_config
    assert "double" in config
    assert "double" in new_config
    assert "nested.readOnlyInteger" in config
    assert "nested.readOnlyInteger" not in new_config
    assert "nested.double" in config
    assert "nested.double" in new_config
    assert "initOnlyDouble" in new_config
    assert "internalInitOnlyDouble" in config
    assert "internalInitOnlyDouble" not in new_config

    # Check runtime configuration!
    new_config = sanitize_write_configuration(schema, deepcopy(config))
    assert config is not None
    assert new_config is not None
    assert "integer" in new_config
    assert "internalInteger" in config
    assert "internalInteger" not in new_config
    assert "readOnlyInteger" in config
    assert "readOnlyInteger" not in new_config
    assert "double" in config
    assert "double" in new_config
    assert "nested.readOnlyInteger" in config
    assert "nested.readOnlyInteger" not in new_config
    assert "nested.double" in config
    assert "nested.double" in new_config
    assert "initOnlyDouble" in config
    assert "initOnlyDouble" not in new_config
    assert "internalInitOnlyDouble" in config
    assert "internalInitOnlyDouble" not in new_config


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
        KARABO_SCHEMA_MIN_EXC: 0,
    }
    ref = {
        KARABO_SCHEMA_DISPLAYED_NAME: 'bar',
        KARABO_SCHEMA_MIN_EXC: 1,
    }
    # get diff between d0 and ref, KARABO_SCHEMA_DISPLAYED_NAME is not in
    # KARABO_EDITABLE_ATTRIBUTES, should not be included in the diff
    diff = attr_fast_deepcopy(d0, ref)
    assert diff == {KARABO_SCHEMA_MIN_EXC: 0}


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


    offline_schema_hash = Offline.getClassSchema()
    online_schema_hash = Online.getClassSchema()

    attrs = extract_modified_schema_attributes(
        online_schema_hash, offline_schema_hash)
    assert attrs is not None
    assert len(attrs) == 3
    assert attrs[0] == Hash('path', 'readOnlyDouble',
                            'attribute', "warnHigh",
                             'value', 15)
    assert attrs[1] == Hash('path', 'readOnlyDouble',
                            'attribute', "warnLow",
                             'value', -20)
    assert attrs[2] == Hash('path', 'integer',
                            'attribute', "metricPrefixEnum",
                            'value', 13)
