import os
from functools import reduce

import numpy as np

from karabo.native import MetricPrefix, NumpyVector, QuantityValue, Unit


def get_karabo_version():
    """Return the karabo version from the KARABO VERSION file
    """
    try:
        path = os.path.join(os.environ['KARABO'], 'VERSION')
    except KeyError:
        print("ERROR: $KARABO is not defined. Make sure you have sourced "
              "the 'activate' script.")
        return ''
    with open(path, 'r') as fp:
        version = fp.read()
    return version


def get_property(device, path):
    """Return the property value from a proxy or device

    :param device: The device instance or proxy object
    :param path: The full path of the property as string

    This function is similar to python's builtin ``getattr`` and used with::

        prop = get_property(proxy, 'node.subnode.property')

    which is equivalent to::

        prop = proxy.node.subnode.property
    """
    return reduce(lambda obj, key: getattr(obj, key), path.split('.'), device)


def set_property(device, path, value):
    """Set a property value on a proxy or device

    :param device: The device instance or proxy object
    :param path: The full path of the property as string
    :param value: The value to be set

    This function is similar to python's builtin ``setattr`` and used with::

        set_property(proxy, 'node.subnode.property', 5)

    which is equivalent to::

        proxy.node.subnode.property = 5
    """
    obj = device
    paths = path.split(".")
    for name in paths[:-1]:
        if not hasattr(obj, name):
            raise AttributeError(
                f"Property {path} is not available on device.")
        obj = getattr(obj, name)
    if not hasattr(obj, paths[-1]):
        raise AttributeError(f"Property {path} is not available on device.")
    setattr(obj, paths[-1], value)


def build_karabo_value(device, path, value, timestamp):
    """Build a karabo value for property value from a proxy or device

    In case of `QuantityValues` the units are automatically used to prevent
    casting.

    This function is similar to python's builtin ``getattr`` and used with::

        prop = build_karabo_value(
            proxy, 'node.subnode.property', 20.1, Timestamp())

    :param device: The device instance or proxy object
    :param path: The full path of the property as string
    :param value: The value to be set
    :param timestamp: The timestamp to be applied
    """
    import warnings
    warnings.warn("This function is deprecated, please set a `Hash` via the "
                  "``Configurable.set(h: Hash)`` in the future.",
                  stacklevel=2)

    prop = reduce(lambda obj, key: getattr(obj, key),
                  path.split('.'), device)

    desc = prop.descriptor
    ktype = type(prop)
    if issubclass(ktype, QuantityValue):
        # Note: For vectors we make sure to cast before if we have to!
        if isinstance(desc, NumpyVector):
            value = np.array(value, dtype=desc.basetype.numpy)
        # In case of a QuantityValue, we can build the value with the units
        # to prevent any unhappy unit casting
        else:
            ntype = getattr(desc, 'numpy', None)
            if ntype is not None:
                value = ntype(value)

        unitSymbol = Unit(desc.unitSymbol)
        metricPrefixSymbol = MetricPrefix(desc.metricPrefixSymbol)
        ret = ktype(value, unit=unitSymbol, metricPrefix=metricPrefixSymbol,
                    timestamp=timestamp)
    else:
        ret = ktype(value, timestamp=timestamp)

    return ret
