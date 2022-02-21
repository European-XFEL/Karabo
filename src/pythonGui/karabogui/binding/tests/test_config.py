import os.path as op
from contextlib import contextmanager
from glob import glob

import numpy as np
from traits.api import Undefined

from karabo.common.api import KARABO_ALARM_LOW, State
from karabo.native import (
    AccessLevel, AccessMode, Assignment, Configurable, Hash, HashList, Schema,
    String, Timestamp, decodeBinary)

from ..api import (
    apply_configuration, apply_default_configuration, apply_fast_data,
    apply_project_configuration, build_binding,
    extract_attribute_modifications, extract_configuration, extract_edits,
    extract_init_configuration, extract_online_edits)
from .schema import (
    ALL_PROPERTIES_MAP, get_all_props_schema, get_simple_props_schema,
    get_simple_schema, get_vectorattr_schema)

TEST_DATA_DIR = op.join(op.dirname(__file__), 'data')


def _dict_diff(d0, d1):
    """Get the new items in dictionary d1"""
    return {k: v for k, v in d1.items() if k not in d0 or d0[k] != v}


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
        for key, value, _ in Hash.flat_iterall(config):
            assert key in extracted
            if isinstance(value, np.ndarray):
                assert all(extracted[key] == value)
            else:
                assert extracted[key] == value


def test_complete_schema():
    schema = get_all_props_schema()
    binding = build_binding(schema)

    namespace = binding.value
    for name in namespace:
        node = getattr(namespace, name)
        assert isinstance(node, ALL_PROPERTIES_MAP[name])


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

    assert binding.value.a.value is Undefined
    assert binding.value.b.value is Undefined
    assert binding.value.c.value is Undefined

    apply_default_configuration(binding)

    assert binding.value.a.value
    assert binding.value.b.value == 'c'
    assert binding.value.c.value is Undefined

    # Make sure the extracted default conversion is minimal
    # It should include properties with default values, options, or node types
    config = extract_configuration(binding)
    default_props = ('a', 'b', 'm', 'h1', 'i1', 'j1')
    for prop in default_props:
        assert prop in config, '{!r} missing from config'.format(prop)

    # Since no user edits, an empty Hash should have been extracted
    # XXX: ListOfNodeBinding is always extracted
    config = extract_edits(schema, binding)
    assert config == Hash('j1', [])


def test_apply_configuration():
    schema = get_all_props_schema()
    binding = build_binding(schema)

    config = Hash('a', False)
    # Applying a configuration always notifies listeners
    with watch_config_update_notification(binding, expected=True):
        apply_configuration(config, binding)
    assert not binding.value.a.value

    config = Hash('not', 'exist')
    apply_configuration(config, binding)
    # Non exist property is ignored
    assert 'not' not in binding.value

    config = Hash('mm', b'foo')
    # bytes type value is converted to bytearray by traits handler
    apply_configuration(config, binding)
    assert binding.value.mm.value == bytearray(b'foo')


def test_apply_project_configuration():
    schema = get_all_props_schema()
    binding = build_binding(schema)

    config = Hash('a', False)
    # Applying a project configuration always does not notify listeners
    with watch_config_update_notification(binding, expected=False):
        apply_project_configuration(config, binding)
    assert not binding.value.a.value

    config = Hash('e', 0.5)
    # change one item in attributes
    attr = {KARABO_ALARM_LOW: 0.0}
    config['e', ...] = attr
    old_attrs = {k: v for k, v in binding.value.e.attributes.items()}
    apply_project_configuration(config, binding)
    new_attrs = {k: v for k, v in binding.value.e.attributes.items()}
    assert _dict_diff(old_attrs, new_attrs) == attr

    config = Hash('not', 'exist')
    apply_project_configuration(config, binding)
    # Non exist property is ignored
    assert 'not' not in binding.value

    config = Hash('mm', b'foo')
    # bytes type value is converted to bytearray by traits handler
    apply_project_configuration(config, binding)
    assert binding.value.mm.value == bytearray(b'foo')

    # try to apply a wrong table value from a project device
    config = Hash('x', HashList([Hash('start', 'not a string', 'stop', 10)]))
    apply_project_configuration(config, binding)
    assert binding.value.x.value == HashList(
        [Hash('start', 0.0, 'stop', 10.0)])


def test_apply_fast_data():
    schema = get_all_props_schema()
    binding = build_binding(schema)
    ts = Timestamp()
    ts.tid = 1337

    config = Hash('a', False)
    with watch_config_update_notification(binding, expected=True):
        apply_fast_data(config, binding, ts)
    assert not binding.value.a.value
    assert binding.value.a.timestamp.tid == 1337

    config = Hash('a', True)
    # configuration is applied but no notification fired
    with watch_config_update_notification(binding, expected=True):
        apply_fast_data(config, binding, ts)
    assert binding.value.a.value
    assert binding.value.a.timestamp.tid == 1337


def test_extract_edit():
    schema = get_all_props_schema()
    binding = build_binding(schema)
    apply_default_configuration(binding)

    # 'a' has default value == True, give 'e' an alarm low attribute
    # XXX: j1 will always be extracted
    config = Hash('a', False, 'e', 0.0, 'j1', [])
    config['e', ...] = {KARABO_ALARM_LOW: 42.0}
    apply_project_configuration(config, binding)

    extracted = extract_edits(schema, binding)
    assert extracted == config


def test_extract_online_edit():
    schema = get_all_props_schema()
    binding = build_binding(schema)
    apply_default_configuration(binding)

    # 'a' has default value == True
    # `j1` is ListOfNodes
    config = Hash('a', False, 'e', 0.0, 'j1', [])
    apply_project_configuration(config, binding)

    success, extracted = extract_online_edits(schema, binding)
    assert extracted == Hash('a', False, 'e', 0.0)
    assert success is False

    schema = get_simple_schema()
    binding = build_binding(schema)

    # New round
    apply_default_configuration(binding)
    config = Hash('foo', False, 'bar', 'now', 'charlie', 21)
    apply_project_configuration(config, binding)
    success, extracted = extract_online_edits(schema, binding)
    assert success is True
    # Charlie is not writable
    assert extracted == Hash('foo', False, 'bar', 'now')

    # New round, no changes
    apply_default_configuration(binding)
    success, extracted = extract_online_edits(schema, binding)
    assert success is True
    assert extracted == Hash()

    # New round, change foo
    apply_default_configuration(binding)
    config = Hash('foo', False, 'bar', 'default')
    apply_project_configuration(config, binding)
    success, extracted = extract_online_edits(schema, binding)
    assert success is True
    assert extracted == Hash('foo', False)


def test_extract_online_devicenode():
    class Device(Configurable):
        dn = String(displayType="deviceNode")

    schema = Device.getClassSchema()
    binding = build_binding(schema)
    apply_default_configuration(binding)
    success, extracted = extract_online_edits(schema, binding)
    assert success is False
    assert extracted == Hash()

    # DeviceNodes with a value are successful
    config = Hash('dn', "XHQ_EG_CTRL/MDL/1")
    apply_project_configuration(config, binding)
    success, extracted = extract_online_edits(schema, binding)
    assert success is True
    assert extracted == config


def test_attribute_modification():
    schema = get_all_props_schema()
    binding = build_binding(schema)

    modifications = extract_attribute_modifications(schema, binding)
    assert modifications is None

    attributes = binding.value.h.attributes
    attributes[KARABO_ALARM_LOW] = 42
    modifications = extract_attribute_modifications(schema, binding)
    assert len(modifications) == 1
    assert modifications[0] == Hash('path', 'h',
                                    'attribute', KARABO_ALARM_LOW,
                                    'value', 42)


def test_property_attributes():
    schema = get_all_props_schema()
    binding = build_binding(schema)

    assert binding.value.c.required_access_level is AccessLevel.EXPERT
    assert binding.value.d.access_mode is AccessMode.READONLY
    assert binding.value.d.assignment is Assignment.INTERNAL
    assert binding.value.e.unit_label == 'm'

    assert binding.value.a.access_mode is AccessMode.RECONFIGURABLE
    assert binding.value.a.assignment is Assignment.OPTIONAL
    assert binding.value.a.required_access_level is AccessLevel.USER

    assert binding.value.m.options == ['foo', 'bar', 'baz', 'qux']

    assert binding.value.k1.is_allowed(State.INTERLOCKED)
    assert binding.value.k1.is_allowed(State.ACTIVE.value)


def test_extract_attribute_modifications_vectorattr():
    schema = get_vectorattr_schema()
    binding = build_binding(schema)

    apply_default_configuration(binding)
    ret = extract_attribute_modifications(schema, binding)
    assert ret is None

    # Change alarmLow from [True, True] to [False, False]
    newv = np.array([False, False])
    binding.value.vec.attributes[KARABO_ALARM_LOW] = newv
    ret = extract_attribute_modifications(schema, binding)
    # XXX: middlelayer Hash can't compare np array either
    assert all(ret[0]['value'] == newv)


def test_extract_reconfigurable_configuration():
    schema = get_simple_props_schema()
    binding = build_binding(schema)
    config = Hash(
        "boolProperty", False,
        "falseProperty", False,  # Same as default
        "doubleProperty", 2.1,
        "floatProperty", 1.0,  # ReadOnly, but we apply alarm attr
        "intProperty", 0,  # ReadOnly
        "stringProperty", "",
        "table", [],
        "node.charlie", 27,  # Readonly integer
        "node.foo", False,
        "node.bar", "default",  # Same as default
        "internal", False,  # Other value as default, but Assignment.INTERNAL
    )
    config["floatProperty", ...].update({"alarmLow": 2})

    # DaqPolicy runattr check
    config["boolProperty", ...].update({"daqPolicy": -1})
    config["node.foo", ...].update({"daqPolicy": 1})

    extracted = extract_init_configuration(binding, config)
    assert "boolProperty" in extracted
    bool_attrs = extracted["boolProperty", ...]
    assert "daqPolicy" not in bool_attrs
    assert "doubleProperty" in extracted
    assert "falseProperty" not in extracted
    assert "floatProperty" in extracted
    assert "intProperty" not in extracted
    assert "stringProperty" in extracted
    assert "table" in extracted
    assert "node.charlie" not in extracted
    assert "node.foo" in extracted
    foo_attr = extracted["node.foo", ...]
    assert "daqPolicy" not in foo_attr
    assert "node.bar" not in extracted
    assert "floatProperty" in extracted
    assert extracted["floatProperty", "alarmLow"] == 2
    # Assignment.INTERNAL property not considered!
    assert "internal" not in extracted
