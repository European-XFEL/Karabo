import asyncio
import os
from functools import reduce, wraps
from time import perf_counter

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

    This function has been added in Karabo >= 2.14.

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


class profiler:
    """A versatile profiling class

    Use this class either as a context manager or as decorator::

        with profiler():
            # classic context manager

        with profiler("Long computation 1"):
            # do something but provide a name for the context

        The class can also be used for decoration of functions (async works)

        @profiler()
        async def do_something()
            # do something

        async with profiler("Async with statement"):
            # do something with name in context

    """

    def __init__(self, name=None):
        self.name = name
        self.t_start = None

    def __enter__(self):
        self.t_start = perf_counter()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        elapsed = perf_counter() - self.t_start
        name = f"{self.name}:" if self.name is not None else ":"
        print(f"With block {name} time elapsed {elapsed}")

    async def __aenter__(self):
        return self.__enter__()

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        return self.__exit__(exc_type, exc_val, exc_tb)

    def __call__(self, func):
        """Decorate a function to profile the execution time"""
        name = func.__name__ if self.name is None else self.name
        if not asyncio.iscoroutine(func):
            @wraps(func)
            def wrapper(*args, **kwargs):
                t_start = perf_counter()
                ret = func(*args, **kwargs)
                elapsed = perf_counter() - t_start
                print(f"{name} took {elapsed}")
                return ret
        else:
            @wraps(func)
            async def wrapper(*args, **kwargs):
                t_start = perf_counter()
                ret = await func(*args, **kwargs)
                elapsed = perf_counter() - t_start
                print(f"{name} took {elapsed}")
                return ret

        return wrapper
