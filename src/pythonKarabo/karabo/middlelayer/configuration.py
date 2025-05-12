# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
from karabo.common.api import (
    KARABO_SCHEMA_ACCESS_MODE, KARABO_SCHEMA_ASSIGNMENT,
    KARABO_SCHEMA_CLASS_ID, KARABO_SCHEMA_OPTIONS)
from karabo.native import (
    AccessMode, Assignment, Hash, NodeType, Schema, is_equal)


def sanitize_init_configuration(schema, config):
    """Sanitize a configuration to be used as INIT configuration

    - Remove all readOnly configuration that are not in the schema!
    - Remove all assignment internal variables

    :param schema: The `Schema` object of the device
    :param config: The configuration `Hash`
    """
    assert isinstance(schema, Schema)

    config = extract_configuration(schema, config)

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


def extract_configuration(schema, config):
    """Extract a configuration with a reference schema from a config `Hash`

    :param schema: The schema for filtering
    :param config: The configuration to be used
    :param init: Declare if this configuration is used for initialization

    Note: For runtime use, the schema should be the state dependent schema

    Note: Returns a configuration Hash, that:
        - Does not contain Slots
        - Does not have obsolete paths, e.g. key has to be in schema
        - The configuration won't have `None` values
    """
    assert isinstance(schema, Schema)

    def _iter_schema(schema_hash, base=''):
        base = base + '.' if base else ''
        for key, value, attrs in schema_hash.iterall():
            subkey = base + key
            is_slot = attrs.get(KARABO_SCHEMA_CLASS_ID, "") == "Slot"
            is_node = attrs["nodeType"] == NodeType.Node.value
            if is_node and not is_slot:
                yield from _iter_schema(value, base=subkey)
            elif not is_slot:
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

    return retval


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
