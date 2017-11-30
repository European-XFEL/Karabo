from collections.abc import Iterable

from karabo.middlelayer import Hash, MetricPrefix, Timestamp, Unit
from . import const
from .proxy import PropertyProxy
from .recursive import ChoiceOfNodesBinding, ListOfNodesBinding
from .types import BindingNamespace, BindingRoot, NodeBinding, SlotBinding


def apply_configuration(config, binding, remember_modification=False,
                        skip_modified=False):
    """Recursively set values from a configuration Hash object to a binding
    object.

    The value of `remember_modification` will be set to each `modified` trait
    only the binding nodes. If `skip_modified` is True, values from `config`
    will not be applied to parts of the binding which have been modified
    previously.
    """
    _node_types = (ChoiceOfNodesBinding, NodeBinding)
    namespace = binding.value
    assert isinstance(namespace, BindingNamespace)

    for key, value, attrs in config.iterall():
        if key not in namespace:
            continue
        node = getattr(namespace, key)
        if isinstance(value, Hash) and isinstance(node, _node_types):
            apply_configuration(value, node)
        else:
            if node.modified and skip_modified:
                continue  # Move along

            node.value = value
            # Set the timestamp if it's there
            ts = Timestamp.fromHashAttributes(attrs)
            # XXX: What should we do if the timestamp is None??
            if ts is not None:
                node.timestamp = ts
            # Clear the modified flag if desired
            node.modified = remember_modification

    # Notify listeners
    binding.config_update = True


def apply_default_configuration(binding):
    """Recursively set default values for a binding object.
    """
    assert isinstance(binding, BindingRoot)

    def _iter_binding(node):
        _node_types = (ChoiceOfNodesBinding, NodeBinding)
        namespace = node.value
        for name in namespace:
            subnode = getattr(namespace, name)
            if isinstance(subnode, _node_types):
                if isinstance(subnode, ChoiceOfNodesBinding):
                    yield subnode
                yield from _iter_binding(subnode)
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
            node.modified = False


def extract_attribute_modifications(schema, binding):
    """Extract modified attributes from a binding relative to some schema
    object.

    Returns a list of Hashes containing the differences, or None if there are
    none.

    NOTE: `_NAME_MAP` and `_remap_value` are handling conversion of str
    'Symbol' values to C++ enumeration values (integers). This is because the
    C++ Schema code only knows how to assign from enumeration values, not from
    their stringified representations.
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

    def _dictdiff(d0, d1):
        def _not_equal(v0, v1):
            ret = (v0 != v1)
            # comparison of numpy arrays result in an array
            return all(ret) if isinstance(ret, Iterable) else ret
        return {k: v for k, v in d0.items() if _not_equal(d1.get(k), v)}

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
        diff = _dictdiff(binding_attrs, schema_attrs)
        if not diff:
            continue
        diff = {_NAME_MAP.get(k, k): _remap_value(k, v)
                for k, v in diff.items()}
        retval.extend(_get_updates(key, diff))

    if retval:
        return retval
    return None


def extract_configuration(binding, modified_only=True):
    """Extract all the values set on a binding into a Hash object.
    """
    assert isinstance(binding, BindingRoot)

    def _iter_binding(node, base=''):
        _node_types = (ChoiceOfNodesBinding, NodeBinding)
        namespace = node.value
        base = base + '.' if base else ''
        for name in namespace:
            subname = base + name
            subnode = getattr(namespace, name)
            if isinstance(subnode, _node_types):
                yield from _iter_binding(subnode, base=subname)
            elif not isinstance(subnode, SlotBinding):
                yield subname, subnode

    retval = Hash()
    for key, node in _iter_binding(binding):
        if modified_only and not node.modified:
            continue
        retval[key] = _get_binding_value(node)

    return retval


def extract_sparse_configurations(proxies, devices=None):
    """Extract values set on the bindings of a list of `PropertyProxy`
    instances into a dictionary of `Hash` objects (one per device).

    If desired, an existing dictionary object can be provided so that a
    configuration can be iteratively accumulated.
    """
    assert all(isinstance(p, PropertyProxy) for p in proxies)

    devices = {} if devices is None else devices
    for proxy in proxies:
        key, binding = proxy.path, proxy.binding
        if binding is None or not binding.modified:
            continue

        device_id = proxy.root_proxy.device_id
        hsh = devices.setdefault(device_id, Hash())
        hsh[key] = _get_binding_value(binding)

    return devices


def has_modifications(binding):
    """Returns True if a binding contains any modified values."""
    assert isinstance(binding, BindingRoot)

    def _iter_binding(node, base=''):
        _node_types = (ChoiceOfNodesBinding, NodeBinding)
        namespace = node.value
        base = base + '.' if base else ''
        for name in namespace:
            subnode = getattr(namespace, name)
            if isinstance(subnode, _node_types):
                yield from _iter_binding(subnode, base=base + name)
            elif not isinstance(subnode, SlotBinding):
                yield subnode

    for node in _iter_binding(binding):
        if node.modified:
            return True
    return False


def _get_binding_value(binding):
    """Extract the value from a single binding instance."""
    if isinstance(binding, ListOfNodesBinding):
        return [Hash(value.class_id, extract_configuration(value))
                for value in binding.value]
    else:
        return binding.value
