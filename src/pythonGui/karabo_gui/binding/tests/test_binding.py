from glob import glob
import os.path as op

import numpy as np

from karabo.middlelayer import (
    AccessLevel, AccessMode, Assignment, Hash, MetricPrefix, Schema, State,
    Unit, decodeBinary
)
from ..api import (
    BoolBinding, CharBinding, ChoiceOfNodesBinding, ComplexBinding,
    FloatBinding, HashBinding,
    Int8Binding, Int16Binding, Int32Binding, Int64Binding,
    ListOfNodesBinding, NodeBinding, SchemaBinding, SlotBinding, StringBinding,
    Uint8Binding, Uint16Binding, Uint32Binding, Uint64Binding,
    VectorBoolBinding, VectorCharBinding, VectorComplexDoubleBinding,
    VectorComplexFloatBinding, VectorDoubleBinding, VectorFloatBinding,
    VectorHashBinding, VectorInt8Binding, VectorInt16Binding,
    VectorInt32Binding, VectorInt64Binding, VectorStringBinding,
    VectorUint8Binding, VectorUint16Binding, VectorUint32Binding,
    VectorUint64Binding,
    apply_configuration, apply_default_configuration, build_binding,
    extract_attribute_modifications, extract_configuration, has_modifications,
    KARABO_SCHEMA_DEFAULT_VALUE,
    KARABO_SCHEMA_METRIC_PREFIX_ENUM, KARABO_SCHEMA_METRIC_PREFIX_SYMBOL,
    KARABO_SCHEMA_UNIT_ENUM, KARABO_SCHEMA_UNIT_SYMBOL
)
from .schema import get_all_props_schema

TEST_DATA_DIR = op.join(op.dirname(__file__), 'data')


def _flatten_hash(h, base=''):
    """Flatten a Hash object"""
    base = base + '.' if base else ''
    for key, value, attrs in h.iterall():
        here = base + key
        if isinstance(value, Hash):
            yield from _flatten_hash(value, base=here)
        else:
            yield here, value


def test_data_files():
    for filename in glob(op.join(TEST_DATA_DIR, '*.schema')):
        classname = op.splitext(op.basename(filename))[0]
        with open(filename, 'rb') as fp:
            hsh = decodeBinary(fp.read())

        schema = Schema(name=classname, hash=hsh)
        binding = build_binding(schema)

        config_filename = op.splitext(filename)[0] + '.config'
        with open(config_filename, 'rb') as fp:
            config = decodeBinary(fp.read())

        assert not has_modifications(binding)
        apply_configuration(config, binding, remember_modification=True)
        assert has_modifications(binding)
        extracted = extract_configuration(binding)

        # Check that the configuration was applied
        for key, value in _flatten_hash(extracted):
            assert key in config
            if isinstance(value, np.ndarray):
                assert all(config[key] == value)
            else:
                assert config[key] == value


def test_complete_schema():
    expected = {
        'a': BoolBinding, 'b': CharBinding, 'c': ComplexBinding,
        'd': ComplexBinding, 'e': FloatBinding, 'f': FloatBinding,
        'g': HashBinding, 'h': Int16Binding, 'i': Int32Binding,
        'j': Int64Binding, 'k': Int8Binding, 'l': SchemaBinding,
        'm': StringBinding, 'n': Uint16Binding, 'o': Uint32Binding,
        'p': Uint64Binding, 'q': Uint8Binding, 'r': VectorBoolBinding,
        's': VectorCharBinding, 't': VectorComplexDoubleBinding,
        'u': VectorComplexFloatBinding, 'v': VectorDoubleBinding,
        'w': VectorFloatBinding, 'x': VectorHashBinding,
        'y': VectorInt16Binding, 'z': VectorInt32Binding,
        'a1': VectorInt64Binding, 'b1': VectorInt8Binding,
        'c1': VectorStringBinding, 'd1': VectorUint16Binding,
        'e1': VectorUint32Binding, 'f1': VectorUint64Binding,
        'g1': VectorUint8Binding, 'h1': NodeBinding,
        'i1': ChoiceOfNodesBinding, 'j1': ListOfNodesBinding,
        'k1': SlotBinding,
    }

    schema = get_all_props_schema()
    binding = build_binding(schema)

    namespace = binding.value
    for name in namespace:
        node = getattr(namespace, name)
        assert isinstance(node, expected[name])


def test_default_values():
    schema = get_all_props_schema()
    binding = build_binding(schema)

    assert not binding.value.a.value
    assert binding.value.b.value == ''

    apply_default_configuration(binding)

    assert binding.value.a.value
    assert binding.value.b.value == 'c'


def test_attribute_modification():
    schema = get_all_props_schema()
    binding = build_binding(schema)

    modifications = extract_attribute_modifications(schema, binding)
    assert modifications is None

    attributes = binding.value.h.attributes
    attributes[KARABO_SCHEMA_DEFAULT_VALUE] = 42
    modifications = extract_attribute_modifications(schema, binding)
    assert len(modifications) == 1
    assert modifications[0] == Hash('path', 'h',
                                    'attribute', KARABO_SCHEMA_DEFAULT_VALUE,
                                    'value', 42)

    binding = build_binding(schema)
    attributes = binding.value.h.attributes
    attributes[KARABO_SCHEMA_METRIC_PREFIX_SYMBOL] = 'm'
    modifications = extract_attribute_modifications(schema, binding)
    assert modifications[0] == Hash(
        'path', 'h',
        'attribute', KARABO_SCHEMA_METRIC_PREFIX_ENUM,
        'value', list(MetricPrefix).index(MetricPrefix.MILLI)
    )

    binding = build_binding(schema)
    attributes = binding.value.h.attributes
    attributes[KARABO_SCHEMA_UNIT_SYMBOL] = 'Sv'
    modifications = extract_attribute_modifications(schema, binding)
    assert modifications[0] == Hash('path', 'h',
                                    'attribute', KARABO_SCHEMA_UNIT_ENUM,
                                    'value', list(Unit).index(Unit.SIEVERT))


def test_property_attributes():
    schema = get_all_props_schema()
    binding = build_binding(schema)

    assert binding.value.c.required_access_level is AccessLevel.EXPERT
    assert binding.value.d.access_mode is AccessMode.READONLY
    assert binding.value.d.assignment is Assignment.INTERNAL
    assert binding.value.e.unit_label == 'm'

    assert binding.value.a.access_mode is AccessMode.RECONFIGURABLE
    assert binding.value.a.assignment is Assignment.OPTIONAL
    assert binding.value.a.required_access_level is AccessLevel.OBSERVER

    assert binding.value.m.options == ['foo', 'bar', 'baz', 'qux']

    assert binding.value.k1.is_allowed(State.INTERLOCKED)
    assert binding.value.k1.is_allowed(State.ACTIVE.value)
