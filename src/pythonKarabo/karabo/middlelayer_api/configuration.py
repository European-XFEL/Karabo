from karabo.common import const
from karabo.common.api import (
    KARABO_SCHEMA_ACCESS_MODE, KARABO_SCHEMA_ASSIGNMENT,
    KARABO_SCHEMA_CLASS_ID, KARABO_SCHEMA_OPTIONS)
from karabo.native import (
    AccessMode, Assignment, Hash, MetricPrefix, NodeType, Schema, Unit,
    is_equal)

# Broken path accumulated in the history of karabo ...
# Choice of Nodes connection and Beckhoff properties that will throw!

BROKEN_PATHS = set([
    "_connection_",
])


def sanitize_init_configuration(schema, config):
    """Sanitize a configuration to be used as INIT configuration

    - Remove all readOnly configuration that are not in the schema!
    - Remove all assignment internal variables

    :param schema: The `Schema` object of the device
    :param config: The configuration `Hash`
    """
    assert isinstance(schema, Schema)

    config = extract_configuration(schema, config, init=True)

    readonly_paths = [pth for pth, _, _ in Hash.flat_iterall(config)
                      if schema.hash[pth, KARABO_SCHEMA_ACCESS_MODE] ==
                      AccessMode.READONLY.value
                      or schema.hash[pth, KARABO_SCHEMA_ASSIGNMENT] ==
                      Assignment.INTERNAL.value]
    for key in readonly_paths:
        config.erase(key)

    return config


def sanitize_write_configuration(schema, config):
    """Sanitize a configuration to be applied as runtime configuration

    :param schema: state dependent runtime schema
    :param config: configuration hash
    """
    assert isinstance(schema, Schema)

    config = extract_configuration(schema, config)

    readonly_paths = [pth for pth, _, _ in Hash.flat_iterall(config)
                      if schema.hash[pth, KARABO_SCHEMA_ACCESS_MODE] in
                      [AccessMode.READONLY.value, AccessMode.INITONLY.value]]
    for key in readonly_paths:
        config.erase(key)

    return config


def extract_configuration(schema, config, init=False):
    """Extract a configuration with a reference schema from a config `Hash`

    :param schema: The schema for filtering
    :param config: The configuration to be used
    :param init: Declare if this configuration is used for initialization

    Note: For runtime use, the schema should be the state dependent schema

    Note: Returns a configuration Hash, that:
        - Does not contain Slots
        - Does not have obsolete paths, e.g. key has to be in schema
        - The configuration won't have `None` values
        - Attribute Options is taken into account
        - Init takes into account ChoiceOfNodes
        - Special Types: ListOfNodes, ChoiceOfNodes are omitted on runtime
    """
    assert isinstance(schema, Schema)

    def _iter_schema(schema_hash, base=''):
        base = base + '.' if base else ''
        for key, value, attrs in schema_hash.iterall():
            subkey = base + key
            is_slot = attrs.get(KARABO_SCHEMA_CLASS_ID, "") == "Slot"
            is_node = attrs["nodeType"] == NodeType.Node.value
            is_choice = attrs["nodeType"] == NodeType.ChoiceOfNodes.value
            is_special = attrs["nodeType"] in [NodeType.ListOfNodes.value,
                                               NodeType.ChoiceOfNodes.value]
            if is_node and not is_slot:
                yield from _iter_schema(value, base=subkey)
            elif init and is_choice:
                yield subkey, attrs
            elif not is_slot and not is_special:
                yield subkey, attrs

    retval = Hash()

    for key, attrs in _iter_schema(schema.hash):
        value = config.get(key, None)
        # If the key is not in the config, we continue
        if value is None:
            continue
        # Check if we have options and act accordingly, yes
        # some fancy schema evolution can always appear
        options = attrs.get(KARABO_SCHEMA_OPTIONS, None)
        if options is not None and value not in options:
            continue
        retval[key] = value

    # XXX: This for general backward compatibility ...
    for key in BROKEN_PATHS:
        if key in retval:
            retval.erase(key)

    return retval


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

    NODE_TYPE = (NodeType.Node.value,)
    SPECIAL_TYPES = (NodeType.ChoiceOfNodes.value,
                     NodeType.ListOfNodes.value)

    def flat_iter_schema_hash(schema_hash, base=''):
        """Expose a flat iteration over a schema Hash.

        :param schema: The schema Hash or Schema object

        Note: Schema Hashes are special because every property
        comes with an empty `Hash` as value. Hence, we ask for the Nodetype!
        """
        assert isinstance(schema_hash, Hash)

        base = base + '.' if base else ''
        for key, value, attrs in schema_hash.iterall():
            subkey = base + key
            is_node = attrs["nodeType"] in NODE_TYPE
            is_special = attrs["nodeType"] in SPECIAL_TYPES
            if is_node:
                yield from flat_iter_schema_hash(value, base=subkey)
            elif not is_special:
                yield subkey

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


def config_changes(a, b):
    """Compare two `Hash` configurations by `key`, `value`

    Note: This function does not consider attributes modifications.

    :param a: input `Hash` a
    :param b: input `Hash` b

    :returns: changes Hash
    """
    assert isinstance(a, Hash), isinstance(b, Hash)

    changes = Hash()
    for key, a_value, _ in Hash.flat_iterall(a):
        if key not in b:
            changes[key] = [a_value, None]
            continue
        b_value = b[key]
        if not is_equal(a_value, b_value):
            changes[key] = [a_value, b_value]

    for key, b_value, _ in Hash.flat_iterall(b):
        if key not in a:
            changes[key] = [None, b_value]

    return changes


def validate_init_configuration(schema: Schema, config: Hash) -> str:
    """Validate an incoming configuration against a schema

    Readonly parameters are removed for the time being. Not existent
    parameters are declared invalid and reported!

    Returns a text (str) which is empty if no violations are found,
    otherwise the issues are reported.
    Hence, an empty string means success!

    :returns: text (str)
    """
    text = ""
    not_available_paths = []

    to_erase = []
    for path, _, attrs in Hash.flat_iterall(config, empty=False):
        try:
            if (schema.hash[path, KARABO_SCHEMA_ACCESS_MODE]
                    == AccessMode.READONLY.value):
                # Note: We can still receive configs for read only params,
                # Extend for read only when attributes are removed.
                to_erase.append(path)
                continue
        except KeyError:
            not_available_paths.append(path)

    if not_available_paths:
        text += ("Configuration contains paths that are not "
                 f"available {not_available_paths}. ")

    to_erase.extend(not_available_paths)
    for path in to_erase:
        config.erase(path)

    # Cleanup for left over nodes (empty Hashes)!
    to_erase = [path for path, v, _ in Hash.flat_iterall(
        config, empty=True) if isinstance(config[path], Hash)
        and config[path].empty()]

    for path in to_erase:
        config.erase(path)

    # Note: If not an empty text is returned, we have invalid paths!
    return text
