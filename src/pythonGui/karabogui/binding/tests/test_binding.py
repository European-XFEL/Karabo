from contextlib import contextmanager
from glob import glob
import os.path as op

import numpy as np
from traits.api import Undefined

from karabo.middlelayer import (
    AccessLevel, AccessMode, Assignment, Hash, MetricPrefix, Schema, State,
    Unit, decodeBinary
)
from ..api import (
    BoolBinding, ByteArrayBinding, CharBinding, ChoiceOfNodesBinding,
    ComplexBinding, FloatBinding, HashBinding,
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
    extract_attribute_modifications, extract_configuration, flat_iter_hash,
    KARABO_SCHEMA_DEFAULT_VALUE,
    KARABO_SCHEMA_METRIC_PREFIX_ENUM, KARABO_SCHEMA_METRIC_PREFIX_SYMBOL,
    KARABO_SCHEMA_UNIT_ENUM, KARABO_SCHEMA_UNIT_SYMBOL
)
from .schema import get_all_props_schema, get_vectorattr_schema

TEST_DATA_DIR = op.join(op.dirname(__file__), 'data')


@contextmanager
def watch_config_update_notification(binding, expected):
    """Watch for binding.config_update notification, assert that trait event
    should be fired/not fired according to `expected`
    """
    notified = False

    def _event_fired():
        nonlocal notified
        notified = True

    try:
        binding.on_trait_event(_event_fired, 'config_update')
        yield
    finally:
        binding.on_trait_event(_event_fired, 'config_update', remove=True)
        assert notified == expected


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

        apply_configuration(config, binding)
        extracted = extract_configuration(binding)

        # Check that the configuration was applied
        for key, value, _ in flat_iter_hash(config):
            assert key in extracted
            if isinstance(value, np.ndarray):
                assert all(extracted[key] == value)
            else:
                assert extracted[key] == value


def test_complete_schema():
    expected = {
        'a': BoolBinding, 'b': CharBinding, 'c': ComplexBinding,
        'd': ComplexBinding, 'e': FloatBinding, 'f': FloatBinding,
        'g': HashBinding, 'h': Int16Binding, 'i': Int32Binding,
        'j': Int64Binding, 'k': Int8Binding, 'll': SchemaBinding,
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
        'k1': SlotBinding, 'mm': ByteArrayBinding,
    }

    schema = get_all_props_schema()
    binding = build_binding(schema)

    namespace = binding.value
    for name in namespace:
        node = getattr(namespace, name)
        assert isinstance(node, expected[name])


def test_build_binding_stability():
    schema = get_all_props_schema()
    binding = build_binding(schema)

    before_names = list(binding.value)
    binding = build_binding(schema, existing=binding)
    after_names = list(binding.value)

    # The order must be unchanged!
    assert before_names == after_names


def test_default_values():
    schema = get_all_props_schema()
    binding = build_binding(schema)

    assert not binding.value.a.value
    assert binding.value.b.value == ''

    apply_default_configuration(binding)

    assert binding.value.a.value
    assert binding.value.b.value == 'c'

    # Make sure the extracted default conversion is minimal
    # It should include properties with default values, options, or node types
    config = extract_configuration(binding)
    default_props = ('a', 'b', 'm', 'h1', 'i1', 'j1')
    for prop in default_props:
        assert prop in config, '{!r} missing from config'.format(prop)


def test_apply_configuration():
    schema = get_all_props_schema()
    binding = build_binding(schema)

    config = Hash('a', False)
    with watch_config_update_notification(binding, expected=True):
        apply_configuration(config, binding)
    assert not binding.value.a.value

    config = Hash('a', True)
    # configuration is applied but no notification fired
    with watch_config_update_notification(binding, expected=False):
        apply_configuration(config, binding, notify=False)
    assert binding.value.a.value

    config = Hash('not', 'exist')
    apply_configuration(config, binding)
    # Non exist property is ignored
    assert 'not' not in binding.value

    config = Hash('mm', b'foo')
    # bytes type value is converted to bytearray by traits handler
    apply_configuration(config, binding)
    assert binding.value.mm.value == bytearray(b'foo')


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


def test_extract_attribute_modifications_vectorattr():
    schema = get_vectorattr_schema()
    binding = build_binding(schema)

    apply_default_configuration(binding)
    ret = extract_attribute_modifications(schema, binding)
    assert ret is None

    # Change defaultValue from [True, True] to [False, False]
    newv = np.array([False, False])
    binding.value.vec.attributes[KARABO_SCHEMA_DEFAULT_VALUE] = newv
    ret = extract_attribute_modifications(schema, binding)
    # XXX: middlelayer Hash can't compare np array either
    assert all(ret[0]['value'] == newv)
