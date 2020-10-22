from copy import deepcopy
from ..configuration import sanitize_init_configuration

from karabo.native import (
    AccessMode, Assignment, Configurable, Double, Int32)


def test_sanitize_init_configuration():
    """Test if we can sanitize a configuration"""

    class Object(Configurable):
        double = Double(
            defaultValue=2.0,
            minInc=-10.0, maxInc=10.0, absoluteError=0.5,
            accessMode=AccessMode.RECONFIGURABLE)

        readOnlyInteger = Int32(
            defaultValue=20,
            accessMode=AccessMode.READONLY)

        internalInteger = Int32(
            defaultValue=20,
            assignment=Assignment.INTERNAL)

        integer = Int32(
            defaultValue=20,
            accessMode=AccessMode.RECONFIGURABLE)

        initOnlyDouble = Double(
            defaultValue=100.0,
            accessMode=AccessMode.INITONLY)

        internalInitOnlyDouble = Double(
            defaultValue=30.0,
            accessMode=AccessMode.INITONLY,
            assignment=Assignment.INTERNAL)

    obj = Object()
    config = obj.configurationAsHash()
    schema = Object.getClassSchema()
    # Make a deepcopy for testing!
    new_config = sanitize_init_configuration(schema, deepcopy(config))
    assert config is not None
    assert new_config is not None
    assert "integer" in new_config
    assert "internalInteger" in config
    assert "internalInteger" not in new_config
    assert "readOnlyInteger" in config
    assert "readOnlyInteger" not in new_config
    assert "double" in config
    assert "initOnlyDouble" in new_config
    assert "internalInitOnlyDouble" in config
    assert "internalInitOnlyDouble" not in new_config
