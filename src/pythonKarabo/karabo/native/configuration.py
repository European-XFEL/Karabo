from karabo.common.api import (
    KARABO_SCHEMA_ACCESS_MODE, KARABO_SCHEMA_ASSIGNMENT)
from .data.hash import Hash
from .data.enums import AccessMode, Assignment


def sanitize_init_configuration(schema, config):
    """Sanitize a configuration

    - Remove all readOnly configuration that are not in the schema!
    - Remove all assignment internal variables

    :param schema: The `Schema` object of the device
    :param config: The configuration `Hash`
    """

    def _walk_config(config, base=''):
        base = base + '.' if base else ''
        for key, value, _ in config.iterall():
            subkey = base + key
            if isinstance(value, Hash):
                yield from _walk_config(value, base=subkey)
            else:
                yield subkey

    obsolete_paths = [pth for pth in _walk_config(config)
                      if pth not in schema.hash]
    for key in obsolete_paths:
        config.erase(key)

    readonly_paths = [pth for pth in _walk_config(config)
                      if schema.hash[pth, KARABO_SCHEMA_ACCESS_MODE] ==
                      AccessMode.READONLY.value
                      or schema.hash[pth, KARABO_SCHEMA_ASSIGNMENT] ==
                      Assignment.INTERNAL.value]
    for key in readonly_paths:
        config.erase(key)

    return config
