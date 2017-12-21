from collections.abc import Iterable

from traits.api import TraitError, Undefined

from karabo.middlelayer import Hash, MetricPrefix, Timestamp, Unit
from . import const
from .proxy import PropertyProxy
from .recursive import ChoiceOfNodesBinding, ListOfNodesBinding
from .types import BindingNamespace, BindingRoot, NodeBinding, SlotBinding
from .util import fast_deepcopy


def apply_configuration(config, binding, notify=True,
                        include_attributes=False):
    """Recursively set values from a configuration Hash object to a binding
    object.

    If `notify` is False, trait change notifications of binding
    nodes won't be triggered.
    """
    namespace = binding.value
    assert isinstance(namespace, BindingNamespace)

    for key, value, attrs in config.iterall():
        if key not in namespace:
            continue

        node = getattr(namespace, key)
        if isinstance(value, Hash) and isinstance(node, NodeBinding):
            apply_configuration(value, node, notify=notify,
                                include_attributes=include_attributes)
        else:
            traits = {'value': value}
            # Set the timestamp no matter what
            ts = Timestamp.fromHashAttributes(attrs)
            traits['timestamp'] = ts or Timestamp()
            if include_attributes:
                traits['attributes'] = fast_deepcopy(attrs)
            # Set everything at once and notify via the config_update event
            try:
                node.trait_set(trait_change_notify=False, **traits)
            except TraitError:
                # value in the configuration is not compatible to schema
                continue
            if notify:
                node.config_update = True

    # Notify listeners
    if notify:
        binding.config_update = True


def apply_default_configuration(binding):
    """Recursively set default values for a binding object.
    """
    assert isinstance(binding, BindingRoot)
    _special_types = (ChoiceOfNodesBinding, NodeBinding, ListOfNodesBinding)

    def _iter_binding(node):
        namespace = node.value
        for name in namespace:
            subnode = getattr(namespace, name)
            if isinstance(subnode, NodeBinding):
                yield from _iter_binding(subnode)
            if isinstance(subnode, ChoiceOfNodesBinding):
                yield subnode
                child = getattr(subnode.value, subnode.choice)
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
        elif not isinstance(node, _special_types):
            node.value = Undefined


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
        return {k: v for k, v in d0.items()
                if _not_equal(d1.get(k), v) and
                k in const.KARABO_EDITABLE_ATTRIBUTES}

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
            retval[key, ...] = fast_deepcopy(node.attributes)

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
