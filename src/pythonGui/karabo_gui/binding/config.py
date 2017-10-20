from karabo.middlelayer import Hash, Timestamp
from .const import KARABO_SCHEMA_DEFAULT_VALUE
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
        default_value = node.attributes.get(KARABO_SCHEMA_DEFAULT_VALUE)
        if default_value is not None:
            node.value = default_value
            node.modified = False


def extract_attribute_modifications(schema, binding):
    """Extract modified attributes from binding relative to some schema object
    """
    assert isinstance(binding, BindingRoot)

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

    retval = Hash()
    for key, node in _iter_binding(binding):
        schema_attrs = schema.hash[key, ...]
        binding_attrs = node.attributes
        # What values in `binding_attrs` are different from those in
        # `schema_attrs`?
        diff = {k: v for k, v in binding_attrs.items()
                if schema_attrs.get(k) != v}
        if diff:
            retval[key] = None
            retval[key, ...] = diff

    return retval


def extract_configuration(binding):
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
        if not node.modified:
            continue
        if isinstance(node, ListOfNodesBinding):
            retval[key] = [Hash(value.class_id, extract_configuration(value))
                           for value in node.value]
        else:
            retval[key] = node.value

    return retval
