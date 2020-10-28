from karabo.common.api import (
    KARABO_SCHEMA_ACCESS_MODE, KARABO_SCHEMA_ASSIGNMENT)
from karabo.common import const
from .data.hash import Hash, Schema
from .data.enums import AccessMode, Assignment, Unit, MetricPrefix
from .data.utils import flat_iter_hash, flat_iter_schema_hash, is_equal


def _erase_obsolete_path(schema, config):
    """Helper function to erase obsolete path of a configuration"""
    obsolete_paths = [pth for pth in flat_iter_hash(config)
                      if pth not in schema.hash]
    for key in obsolete_paths:
        config.erase(key)

    return config


def sanitize_init_configuration(schema, config):
    """Sanitize a configuration to be used as INIT configuration

    - Remove all readOnly configuration that are not in the schema!
    - Remove all assignment internal variables

    :param schema: The `Schema` object of the device
    :param config: The configuration `Hash`
    """

    config = _erase_obsolete_path(schema, config)

    readonly_paths = [pth for pth in flat_iter_hash(config)
                      if schema.hash[pth, KARABO_SCHEMA_ACCESS_MODE] ==
                      AccessMode.READONLY.value
                      or schema.hash[pth, KARABO_SCHEMA_ASSIGNMENT] ==
                      Assignment.INTERNAL.value]
    for key in readonly_paths:
        config.erase(key)

    return config


def sanitize_write_configuration(schema, config):
    """Sanitize a configuration to be applied as runtime configuration"""

    config = _erase_obsolete_path(schema, config)

    # The Assignment.INTERNAL check should not be needed, as it comes
    # by policy with AccessMode.INITONLY. However, we go safe ...
    readonly_paths = [pth for pth in flat_iter_hash(config)
                      if schema.hash[pth, KARABO_SCHEMA_ACCESS_MODE] in
                      [AccessMode.READONLY.value, AccessMode.INITONLY.value]
                      or schema.hash[pth, KARABO_SCHEMA_ASSIGNMENT] ==
                      Assignment.INTERNAL.value]
    for key in readonly_paths:
        config.erase(key)

    return config


def attr_fast_deepcopy(d, ref=None):
    """copy.deepcopy is criminally slow. We can bypass its fanciness as long
    as we only copy 'simple' datastructures.

    Pass a not None attributes dict to `ref` to get only changed attributes
    """
    out = {}

    for k, v in d.items():
        if ref is not None:
            if (k not in const.KARABO_EDITABLE_ATTRIBUTES or
                    is_equal(v, ref.get(k))):
                continue
        try:
            out[k] = v.copy()  # dicts, sets, ndarrays
        except TypeError:
            # This is a Schema, which has a terrible API
            assert isinstance(v, Schema)
            cpy = Schema()
            cpy.copy(v)
            out[k] = cpy
        except AttributeError:
            try:
                out[k] = v[:]  # lists, tuples, strings, unicode
            except TypeError:
                out[k] = v  # simple values

    return out


def extract_modified_schema_attributes(runtime_schema, class_schema):
    """Extract modified attributes from a runtime schema relative to a
    class schema

    :param runtime_schema: Runtime schema of device
    :param class_schema: Class schema of device

    Returns a list of Hashes containing the differences, or None if there are
    none.

    NOTE: `_NAME_MAP` and `_remap_value` are handling conversion of str
    'Symbol' values to C++ enumeration values (integers). This is because the
    C++ Schema code only knows how to assign from enumeration values, not from
    their stringified representations.
    """

    assert isinstance(runtime_schema, Schema)
    assert isinstance(class_schema, Schema)

    _NAME_MAP = {
        const.KARABO_SCHEMA_METRIC_PREFIX_SYMBOL:
            const.KARABO_SCHEMA_METRIC_PREFIX_ENUM,
        const.KARABO_SCHEMA_UNIT_SYMBOL: const.KARABO_SCHEMA_UNIT_ENUM
    }
    _SYMBOL_MAP = {const.KARABO_SCHEMA_METRIC_PREFIX_SYMBOL: MetricPrefix,
                   const.KARABO_SCHEMA_UNIT_SYMBOL: Unit}

    def _remap_value(name, value):
        enum = _SYMBOL_MAP.get(name, None)
        return list(enum).index(enum(value)) if enum else value

    def _get_updates(path, attrs):
        # Format is specified by Device::slotUpdateSchemaAttributes
        return [Hash("path", path, "attribute", k, "value", v)
                for k, v in attrs.items()]

    retval = []
    runtime_schema_hash = runtime_schema.hash
    class_schema_hash = class_schema.hash
    for key in flat_iter_schema_hash(runtime_schema_hash):
        if key not in class_schema_hash:
            # If key is not there offline, we don't care
            continue
        offline_attrs = class_schema_hash[key, ...]
        online_attrs = runtime_schema_hash[key, ...]
        # Reference is the class schema, offline
        diff = attr_fast_deepcopy(online_attrs, ref=offline_attrs)
        if not diff:
            continue
        diff = {_NAME_MAP.get(k, k): _remap_value(k, v)
                for k, v in diff.items()}
        retval.extend(_get_updates(key, diff))

    if retval:
        return retval
    return None
