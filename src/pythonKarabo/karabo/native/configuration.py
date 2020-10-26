from karabo.common.api import (
    KARABO_SCHEMA_ACCESS_MODE, KARABO_SCHEMA_ASSIGNMENT)
from .data.hash import Hash
from .data.enums import AccessMode, Assignment


def flat_iter_hash(config, base=''):
    """Recursively iterate over all the values in a Hash object such that
    a simple iterator interface is exposed. In this way, a single for-loop
    is sufficient to see an entire Hash.
    """
    base = base + '.' if base else ''
    for key, value, _ in config.iterall():
        subkey = base + key
        if isinstance(value, Hash):
            yield from flat_iter_hash(value, base=subkey)
        else:
            yield subkey


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
