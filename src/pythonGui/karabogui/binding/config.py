from traits.api import TraitError, Undefined

from karabo.common import const
from karabo.native import (
    AccessMode, Assignment, DaqPolicy, Hash, MetricPrefix, Timestamp, Unit,
    has_changes, is_equal)

from .binding_types import (
    BindingNamespace, BindingRoot, NodeBinding, SlotBinding, VectorHashBinding)
from .proxy import PropertyProxy
from .recursive import ChoiceOfNodesBinding, ListOfNodesBinding
from .util import attr_fast_deepcopy
from .validate import sanitize_table_value

RECURSIVE_NODES = (ChoiceOfNodesBinding, ListOfNodesBinding)


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
            elif isinstance(subnode, ChoiceOfNodesBinding):
                yield subnode
                for choice in subnode.choices:
                    child = getattr(subnode.choices, choice)
                    yield from _iter_binding(child)
            elif isinstance(subnode, ListOfNodesBinding):
                yield subnode
                for lnode in subnode.value:
                    yield from _iter_binding(lnode)
            else:
                yield subnode

    for node in _iter_binding(binding):
        default_value = node.attributes.get(const.KARABO_SCHEMA_DEFAULT_VALUE)
        if default_value is not None:
            node.value = default_value
        else:
            # XXX: Nodes either don't have a value or are not designed!
            if not isinstance(node, RECURSIVE_NODES):
                node.value = Undefined


def apply_project_configuration(config, binding, base=''):
    """Recursively set values from a configuration Hash object to a binding
    object of a project device.

    A project configuration consists of `values` and `attributes` and is
    in certain cases sanitized if necessary (table element).

    Setting a project configuration sets the value directly and does not
    notify!

    :Returns: failure `dict` of keys and error messages
    """
    namespace = binding.value
    assert isinstance(namespace, BindingNamespace)

    fails = Hash()
    base = base + '.' if base else ''

    for key, value, attrs in config.iterall():
        if key not in namespace:
            continue

        # For storage in fail dict!
        subkey = base + key

        node = getattr(namespace, key)
        if isinstance(value, Hash) and isinstance(node, NodeBinding):
            fails.update(apply_project_configuration(value, node, base=subkey))
        else:
            if isinstance(node, VectorHashBinding):
                success, value = sanitize_table_value(node, value)
                if not success:
                    fails.update(
                        {subkey: "The table configuration was "
                                 "sanitized - <b>The table is corrupted</b>"})
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
                fails.update({subkey: f"Setting value of {value} failed"})

    return fails


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


def extract_online_edits(schema, binding):
    """Extract all user edited (non default) online values on a binding into a
    Hash object.

    This function takes a class schema as reference to extract the online
    edits and compares them to the default of the schema.
    Only values are considered which are different from the default.

    :param schema: The class schema
    :param binding: The binding of the online device

    :returns success: Boolean to indicate if we have a successful online to
                      offline transition, e.g. no DeviceNodes, ChoiceOfNodes,
                      ListOfNodes.

    """
    assert isinstance(binding, BindingRoot)

    # Backward compatibility protection for difficult keys. Reconsider
    # in Karabo 2.13 FIXME
    BLACKLIST_KEYS = ["_deviceId_", "_serverId_", "hostName"]
    BLACKLIST_NODE = ["Logger."]  # Of course Bound!

    def _get_binding_default(binding):
        value = binding.attributes.get(const.KARABO_SCHEMA_DEFAULT_VALUE)
        return value

    def _iter_binding(node, base=''):
        namespace = node.value
        base = base + '.' if base else ''
        for name in namespace:
            if base in BLACKLIST_NODE:
                continue

            subname = base + name
            subnode = getattr(namespace, name)
            if isinstance(subnode, (ChoiceOfNodesBinding, ListOfNodesBinding)):
                yield subname, subnode
            elif isinstance(subnode, NodeBinding):
                yield from _iter_binding(subnode, base=subname)
            elif not isinstance(subnode, SlotBinding):
                # All rest binding types
                yield subname, subnode

    def not_writable(key):
        """Check if a property is writable from external"""
        attributes = schema.hash[key, ...]
        read_only = attributes['accessMode'] == AccessMode.READONLY.value
        internal = attributes['assignment'] == Assignment.INTERNAL.value
        # Note: One can define tags such as `plc` here to filter out.
        return read_only or internal

    config = Hash()
    success = True
    for key, node in _iter_binding(binding):
        # Injected properties are not considered! Blacklisted keys are
        # filtered out as well!
        if key not in schema.hash or key in BLACKLIST_KEYS:
            continue

        if isinstance(node, (ChoiceOfNodesBinding, ListOfNodesBinding)):
            # ChoiceOfNodes and ListOfNodes cannot be properly compared nor
            # validated and are deprecated.
            success = False
            continue

        value = node.value
        if node.display_type == "deviceNode":
            # DeviceNode may not be successful, until ~2.15 or forever ...
            # Future versions of the device node always have a value
            if not value or value is Undefined:
                success = False
                continue

        if value is Undefined:
            # Note: This did happen for output channels if they don't have
            # clients! (C++ / Bound). Fixed now ...
            # Other examples?
            continue

        # We take the online defaults to account schema injection. If a default
        # value from the online device is_equal, it should not go into the
        # configuration
        default_val = _get_binding_default(node)
        if not has_changes(default_val, value) or not_writable(key):
            continue
        config[key] = value

    return success, config


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


def extract_init_configuration(binding, config):
    """Extract an init configuration from `config` with `binding`

    This function will filter out read only properties without runtime
    attributes from the configuration
s    """

    def _iter_binding(node, base=''):
        """Flat iterate over the binding namespace to yield `key` and `binding`

        Note: Remove recursive binding iteration in future
        """
        namespace = node.value
        base = base + '.' if base else ''
        for name in namespace:
            subname = base + name
            subnode = getattr(namespace, name)
            if isinstance(subnode, NodeBinding):
                yield from _iter_binding(subnode, base=subname)
            elif not isinstance(subnode, SlotBinding):
                yield subname, subnode

    ret = Hash()

    for key, node in _iter_binding(binding):
        # key must be in configuration for processing!
        if key in config:
            value, attrs = config.getElement(key)
            # We only want reconfigurable properties, but in Karabo 2.11
            # read only parameter can still have attributes.
            read_only = node.access_mode is AccessMode.READONLY
            is_internal = node.assignment is Assignment.INTERNAL
            run_attr = attr_fast_deepcopy(attrs, {})
            # Unspecified daqPolicy can be removed as well!
            if run_attr.get("daqPolicy") == DaqPolicy.UNSPECIFIED:
                run_attr.pop("daqPolicy")
            if (read_only or is_internal) and not run_attr:
                continue
            default = node.attributes.get(const.KARABO_SCHEMA_DEFAULT_VALUE)
            if is_equal(default, value) and not run_attr:
                continue
            # We make use of and also filter the runtime attributes
            ret.setElement(key, value, run_attr)

    return ret
