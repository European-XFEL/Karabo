from ast import literal_eval

import numpy as np
from traits.api import TraitError, Undefined

from karabo.common import const
from karabo.native import (
    AccessMode, Hash, HashList, MetricPrefix, Unit, Timestamp)
from .proxy import PropertyProxy
from .recursive import ChoiceOfNodesBinding, ListOfNodesBinding
from .types import (
    BindingNamespace, BindingRoot, BoolBinding, ByteArrayBinding, CharBinding,
    ComplexBinding, FloatBinding, IntBinding, NodeBinding, SlotBinding,
    StringBinding, VectorBinding, VectorDoubleBinding, VectorFloatBinding,
    VectorHashBinding, VectorNumberBinding)
from .util import (
    array_equal, attr_fast_deepcopy, is_equal, realign_hash)

VECTOR_FLOAT_BINDINGS = (VectorFloatBinding, VectorDoubleBinding)
RECURSIVE_BINDINGS = (NodeBinding, ListOfNodesBinding,
                      ChoiceOfNodesBinding)


def apply_fast_data(config, binding, timestamp):
    """Recursively set values from a fast data Hash object to a binding
    object.
    """
    namespace = binding.value
    assert isinstance(namespace, BindingNamespace)

    for key, value, _ in config.iterall():
        if key not in namespace:
            continue

        node = getattr(namespace, key)
        if isinstance(value, Hash) and isinstance(node, NodeBinding):
            apply_fast_data(value, node, timestamp)
        else:
            traits = {'value': value}
            traits['timestamp'] = timestamp
            # Set everything at once and notify via the config_update event
            try:
                node.trait_set(trait_change_notify=False, **traits)
            except TraitError:
                # value in the configuration is not compatible to schema
                continue
            node.config_update = True

    binding.config_update = True


def apply_configuration(config, binding):
    """Recursively set values from a configuration Hash object to a binding
    object.

    This method sets an `online` (network) configuration Hash to a proxy
    binding and thus always notifies and does not include attributes!
    """
    namespace = binding.value
    assert isinstance(namespace, BindingNamespace)

    for key, value, attrs in config.iterall():
        if key not in namespace:
            continue

        node = getattr(namespace, key)
        if isinstance(value, Hash) and isinstance(node, NodeBinding):
            apply_configuration(value, node)
        else:
            traits = {'value': value}
            # Set the timestamp no matter what
            ts = Timestamp.fromHashAttributes(attrs)
            traits['timestamp'] = ts or Timestamp()
            # Set everything at once and notify via the config_update event
            try:
                node.trait_set(trait_change_notify=False, **traits)
            except TraitError:
                # value in the configuration is not compatible to schema
                continue
            node.config_update = True

    # Notify listeners
    binding.config_update = True


def apply_default_configuration(binding):
    """Recursively set default values for a binding object.
    """
    assert isinstance(binding, BindingRoot)

    def _iter_binding(node):
        namespace = node.value
        for name in namespace:
            subnode = getattr(namespace, name)
            if isinstance(subnode, NodeBinding):
                yield from _iter_binding(subnode)
            if isinstance(subnode, ChoiceOfNodesBinding):
                yield subnode
                for choice in subnode.choices:
                    child = getattr(subnode.choices, choice)
                    yield from _iter_binding(child)
            elif isinstance(subnode, ListOfNodesBinding):
                yield subnode
                for lnode in subnode.value:
                    yield from _iter_binding(lnode)
            elif not isinstance(subnode, SlotBinding):
                yield subnode

    for node in _iter_binding(binding):
        default_value = node.attributes.get(const.KARABO_SCHEMA_DEFAULT_VALUE)
        if default_value is not None:
            node.value = default_value
        elif len(node.options) > 0:
            node.value = node.options[0]
        else:
            # XXX: Nodes either don't have a value or are not designed!
            if not isinstance(node, (ChoiceOfNodesBinding,
                                     ListOfNodesBinding, NodeBinding)):
                node.value = Undefined


def sanitize_table_value(binding, value):
    """Sanitize a hash list `value` against existing vector hash `binding`

    :param binding: The existing `VectorHashBinding`
    :param value: the table value (`HashList`)

    :return sanitized value of type `HashList`
    """
    msg = "Expected a value of type `HashList`, got %s instead" % type(value)
    assert isinstance(value, (list, HashList)), msg

    def _sanitize_row(row_bindings, row_hash):
        """Validate a single row of the table"""
        ret = Hash()
        if list(row_bindings.keys()) != list(row_hash.keys()):
            row_hash = realign_hash(row_hash, keys=row_bindings.keys())

        for path, value in row_hash.items():
            binding = row_bindings.get(path, None)
            if binding is None:
                # Tables might lose a property in a row schema. This is fine
                # as we can simply ignore this case. We don't report this as
                # invalid but continue gracefully ...
                continue

            if value is None:
                # The property is a `None` value or is not existent, e.g.
                # a column can have been added.
                # Use the default value from the binding, if necessary force!
                value = get_default_value(binding, force=True)
            else:
                value = validate_value(binding, value)
                # XXX: This is of course critical, if an apple becomes an
                # orange, we should have at least an orange default value.
                if value is None:
                    value = get_default_value(binding, force=True)

            ret[path] = value

        return ret

    ret = HashList()
    for row_hash in value:
        ret.append(_sanitize_row(binding.bindings, row_hash))

    return ret


def apply_project_configuration(config, binding):
    """Recursively set values from a configuration Hash object to a binding
    object of a project device.

    A project configuration consists of `values` and `attributes` and is
    in certain cases sanitized if necessary (table element).

    Setting a project configuration sets the value directly and does not
    notify!
    """
    namespace = binding.value
    assert isinstance(namespace, BindingNamespace)

    for key, value, attrs in config.iterall():
        if key not in namespace:
            continue

        node = getattr(namespace, key)
        if isinstance(value, Hash) and isinstance(node, NodeBinding):
            apply_project_configuration(value, node)
        else:
            if isinstance(node, VectorHashBinding):
                value = sanitize_table_value(node, value)

            traits = {'value': value}
            # Set the timestamp no matter what
            ts = Timestamp.fromHashAttributes(attrs)
            traits['timestamp'] = ts or Timestamp()
            try:
                node.trait_set(trait_change_notify=False, **traits)
                # XXX: pass an empty dictionary to get only editable attributes
                node.update_attributes(attr_fast_deepcopy(attrs, {}))
            except TraitError:
                # value in the configuration is not compatible to schema
                continue


def extract_attribute_modifications(schema, binding):
    """Extract modified attributes from a binding relative to some schema
    object.

    Returns a list of Hashes containing the differences, or None if there are
    none.

    NOTE: `_NAME_MAP` and `_remap_value` are handling conversion of str
    'Symbol' values to C++ enumeration values (integers). This is because the
    C++ Schema code only knows how to assign from enumeration values, not from
    their stringified representations.
    NOTE2: This function is solely used to send attributes modification to
    GUI server during device instantiation
    """
    assert isinstance(binding, BindingRoot)

    _NAME_MAP = {
        const.KARABO_SCHEMA_METRIC_PREFIX_SYMBOL:
            const.KARABO_SCHEMA_METRIC_PREFIX_ENUM,
        const.KARABO_SCHEMA_UNIT_SYMBOL: const.KARABO_SCHEMA_UNIT_ENUM
    }
    _SYMBOL_MAP = {const.KARABO_SCHEMA_METRIC_PREFIX_SYMBOL: MetricPrefix,
                   const.KARABO_SCHEMA_UNIT_SYMBOL: Unit}

    def _iter_binding(node, base=''):
        _recursive_types = (ChoiceOfNodesBinding, ListOfNodesBinding)
        namespace = node.value
        base = base + '.' if base else ''
        for name in namespace:
            subname = base + name
            subnode = getattr(namespace, name)
            if isinstance(subnode, NodeBinding):
                yield from _iter_binding(subnode, base=subname)
            elif isinstance(subnode, _recursive_types):
                # XXX: We need to decide what to do with the attributes of
                # child nodes of these nodes. Currently, they are ignored.
                yield subname, subnode
            elif not isinstance(subnode, SlotBinding):
                yield subname, subnode

    def _remap_value(name, value):
        enum = _SYMBOL_MAP.get(name, None)
        return list(enum).index(enum(value)) if enum else value

    def _get_updates(path, attrs):
        # Format is specified by Device::slotUpdateSchemaAttributes
        return [Hash("path", path, "attribute", k, "value", v)
                for k, v in attrs.items()]

    retval = []
    for key, node in _iter_binding(binding):
        schema_attrs = schema.hash[key, ...]
        binding_attrs = node.attributes
        # What values in `binding_attrs` are different from those in
        # `schema_attrs`?
        diff = attr_fast_deepcopy(binding_attrs, schema_attrs)
        if not diff:
            continue
        diff = {_NAME_MAP.get(k, k): _remap_value(k, v)
                for k, v in diff.items()}
        retval.extend(_get_updates(key, diff))

    if retval:
        return retval
    return None


def extract_configuration(binding, include_attributes=False):
    """Extract all the values set on a binding into a Hash object.
    """
    assert isinstance(binding, BindingRoot)

    def _get_binding_value(binding):
        if isinstance(binding, ListOfNodesBinding):
            return [Hash(value.class_id, extract_configuration(value))
                    for value in binding.value]
        elif isinstance(binding, ChoiceOfNodesBinding):
            value = getattr(binding.value, binding.choice)
            return Hash(value.class_id, extract_configuration(value))
        elif isinstance(binding, VectorHashBinding):
            value = binding.value
            return [] if value is Undefined else value
        else:
            return binding.value

    def _iter_binding(node, base=''):
        namespace = node.value
        base = base + '.' if base else ''
        for name in namespace:
            subname = base + name
            subnode = getattr(namespace, name)
            if isinstance(subnode, NodeBinding):
                yield from _iter_binding(subnode, base=subname)
            elif not isinstance(subnode, SlotBinding):
                yield subname, subnode

    retval = Hash()
    for key, node in _iter_binding(binding):
        value = _get_binding_value(node)
        if value is Undefined:
            continue
        retval[key] = _get_binding_value(node)
        if include_attributes:
            # only copy editable attributes
            retval[key, ...] = attr_fast_deepcopy(node.attributes, {})

    return retval


def extract_edits(schema, binding):
    """Extract all user edited (non default) values on a binding into a
    Hash object.
    """
    assert isinstance(binding, BindingRoot)

    def _get_binding_default(binding):
        val = binding.attributes.get(const.KARABO_SCHEMA_DEFAULT_VALUE)
        if val is None and len(binding.options) > 0:
            return binding.options[0]
        return val

    def _iter_binding(node, base=''):
        namespace = node.value
        base = base + '.' if base else ''
        for name in namespace:
            subname = base + name
            subnode = getattr(namespace, name)
            if isinstance(subnode, ChoiceOfNodesBinding):
                chosen = subnode.choice
                yield from _iter_binding(getattr(subnode.value, chosen),
                                         '{}.{}'.format(subname, chosen))
            elif isinstance(subnode, NodeBinding):
                yield from _iter_binding(subnode, base=subname)
            elif not isinstance(subnode, SlotBinding):
                # All rest binding types (including ListOfNodesBinding)
                yield subname, subnode

    def _get_attr_modification(key, node):
        schema_attrs = schema.hash[key, ...]
        binding_attrs = node.attributes
        return attr_fast_deepcopy(binding_attrs, schema_attrs)

    def _is_readonly(key):
        return schema.hash[key, 'accessMode'] == AccessMode.READONLY.value

    retval = Hash()
    for key, node in _iter_binding(binding):
        if isinstance(node, ListOfNodesBinding):
            value = []
            for lnode in node.value:
                # XXX: Each node in ListOfNodesBinding is a device equivalent
                # We don't have machenism to horizontally loop through sub
                # devices, so we save the device class id and an empty Hash
                # i.e sub device's config is not saved
                # possible fix (currently this fix breaks old GUI):
                # value.append(Hash(lnode.class_id,
                #                   extract_configuration(lnode)))
                value.append(Hash(lnode.class_id, Hash()))
            retval[key] = value
            continue

        default_val = _get_binding_default(node)
        value = None if node.value is Undefined else node.value
        attr_changes = _get_attr_modification(key, node)

        has_attr_changes = len(attr_changes) > 0
        has_val_changes = not is_equal(default_val, value)
        is_readonly = _is_readonly(key)
        if (not (has_val_changes or has_attr_changes) or
                is_readonly and not has_attr_changes):
            continue
        retval[key] = value
        if has_attr_changes:
            retval[key, ...] = attr_changes

    return retval


def extract_sparse_configurations(proxies, devices=None):
    """Extract values set on the bindings of a list of `PropertyProxy`
    instances into a dictionary of `Hash` objects (one per device).

    If desired, an existing dictionary object can be provided so that a
    configuration can be iteratively accumulated.
    """
    assert all(isinstance(p, PropertyProxy) for p in proxies)

    def _get_value(proxy):
        if isinstance(proxy.binding, ListOfNodesBinding):
            return [Hash(value.class_id, extract_configuration(value))
                    for value in proxy.edit_value]
        else:
            return proxy.edit_value

    devices = {} if devices is None else devices
    for proxy in proxies:
        key, binding = proxy.path, proxy.binding
        if binding is None or proxy.edit_value is None:
            continue

        device_id = proxy.root_proxy.device_id
        hsh = devices.setdefault(device_id, Hash())
        hsh[key] = _get_value(proxy)

    return devices


def validate_value(binding, value):
    if type(value) is str:
        # There might be cases that values are saved as strings. This typically
        # occurs for table elements.
        if not isinstance(binding, (StringBinding, CharBinding)):
            value = convert_string(value)
    try:
        if isinstance(binding, VectorNumberBinding):
            # Check if binding is a vector, then test array against its
            # expected `dtype`.
            casted_value = binding.validate_trait("value", value)
            if isinstance(binding, VECTOR_FLOAT_BINDINGS):
                value = np.array(value, dtype=casted_value.dtype)
            value = casted_value if array_equal(casted_value, value) else None
        elif isinstance(binding, VectorHashBinding):
            # VectorHashBinding is not a valid value
            value = None
        elif isinstance(binding, RECURSIVE_BINDINGS):
            # Nothing to do here! We automatically return the value
            pass
        else:
            # The value is a simple data type. We validate it with the binding
            # traits.
            value = binding.validate_trait("value", value)
    except (TraitError, TypeError, ValueError):
        # TraitError happens when traits cannot cast, e.g. str to bool
        # Validation has failed, we inform that there's no validated value
        value = None

    return value


def validate_table_value(binding, value):
    """Validate a hash list `value` against existing vectorhash binding

    :param binding: The existing `VectorHashBinding`
    :param value: the value to be validated (`HashList`)

    :return valid, invalid: (HashList) The values could contain [None, Hash()]
    """
    def _validate_row(row_bindings, row_hash):
        """Validate a single row of the table

        :param row_bindings: The `rowSchema` attribute binding
        :param row_hash: The hash to be validated, either `Hash` or `None`

        :returns:

            - valid: Either a `Hash` or `None`. `None` if validation failed.
            - invalid: Either a `Hash` or `None`
        """
        if row_hash is None:
            # If `row_hash` Hash is `None`, we report as invalid value
            return None, Hash()

        valid = Hash()
        # Check if order of the keys are respected...
        if list(row_bindings.keys()) != list(row_hash.keys()):
            row_hash = realign_hash(row_hash, keys=row_bindings.keys())

        for path, value in row_hash.items():
            binding = row_bindings.get(path, None)
            if binding is None:
                # Tables might lose a property in a row schema. This is fine
                # as we can simply ignore this case. We don't report this as
                # invalid but continue gracefully ...
                continue

            if value is None:
                # The property value is `None` or the value doesn't exist
                # (from `realign_hash`).
                # Try to use the default value from the binding!
                validated_value = get_default_value(binding)
            else:
                validated_value = validate_value(binding, value)

            if validated_value is None:
                # We report the invalid property
                return None, row_hash

            valid[path] = validated_value

        return valid, None

    # Set default values
    valid, invalid = HashList(), HashList()
    for row_hash in value:
        valid_row, invalid_row = _validate_row(binding.bindings, row_hash)
        if valid_row is not None:
            valid.append(valid_row)
        if invalid_row is not None:
            invalid.append(invalid_row)

    return valid, invalid


def get_default_value(binding, force=False):
    """Get the default value from a binding"""

    def _get_binding_default(binding):
        # Provide a default value for all leafType bindings of `binding`
        if isinstance(binding, (CharBinding, StringBinding)):
            return ""
        if isinstance(binding, IntBinding):
            # XXX: No min and max taken into account for now
            return 0
        elif isinstance(binding, FloatBinding):
            return 0.0
        elif isinstance(binding, VectorBinding):
            # All vectors including table!
            return []
        elif isinstance(binding, ComplexBinding):
            return 0.0
        elif isinstance(binding, BoolBinding):
            return False
        elif isinstance(binding, ByteArrayBinding):
            return bytearray([])

        return None

    attrs = binding.attributes
    value = attrs.get(const.KARABO_SCHEMA_DEFAULT_VALUE, None)
    if value is None and force:
        value = _get_binding_default(binding)
        assert value is not None, f"No default value for {type(binding)} ..."
    return value


def convert_string(value):
    try:
        return literal_eval(value)
    except (SyntaxError, ValueError):
        # Conversion of string to a literal failed, we return the value as is.
        return value
