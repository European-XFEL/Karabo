from collections import Iterable
import copy
from functools import wraps

import numpy as np

from karabo.common.states import StateSignifier as SignifierBase
from karabo.native.data.basetypes import (
    newest_timestamp, QuantityValue, wrap, wrap_function)


@wraps(np.linspace)
@wrap_function
def linspace(start, stop, *args, **kwargs):
    if not (isinstance(start, QuantityValue) or
            isinstance(stop, QuantityValue)):
        return np.linspace(start, stop, *args, **kwargs)

    if not isinstance(start, QuantityValue):
        start = QuantityValue(start)

    stop = float(stop / start.units)
    return QuantityValue(
        np.linspace(start.magnitude, stop, *args, **kwargs), start.units)


@wraps(np.dot)
@wrap_function
def dot(a, b, *args, **kwargs):
    if not (isinstance(a, QuantityValue) or isinstance(b, QuantityValue)):
        return np.dot(a, b, *args, **kwargs)

    if not isinstance(a, QuantityValue):
        a = QuantityValue(a)

    if not isinstance(b, QuantityValue):
        b = QuantityValue(b)

    return QuantityValue(np.dot(a.magnitude, b.magnitude), a.units * b.units)


@wraps(np.cross)
@wrap_function
def cross(a, b, *args, **kwargs):
    if not (isinstance(a, QuantityValue) or isinstance(b, QuantityValue)):
        return np.cross(a, b, *args, **kwargs)

    if not isinstance(a, QuantityValue):
        a = QuantityValue(a)

    if not isinstance(b, QuantityValue):
        b = QuantityValue(b)

    return QuantityValue(np.cross(a.magnitude, b.magnitude), a.units * b.units)


def maximum(iterable):
    """Return the maximum value of the iterable

    This function takes into account KaraboValues for newest timestamp
    generation.
    """
    assert isinstance(iterable, Iterable)

    ret = max(iterable)
    ret = wrap(copy.copy(ret))
    ret.timestamp = newest_timestamp(iterable)

    return ret


def minimum(iterable):
    """Return the minimum value of the iterable

    This function takes into account KaraboValues for newest timestamp
    generation.
    """
    assert isinstance(iterable, Iterable)

    ret = min(iterable)
    ret = wrap(copy.copy(ret))
    ret.timestamp = newest_timestamp(iterable)

    return ret


class StateSignifier(SignifierBase):
    """Wrapper of the StateSignifier to provide newest timestamp
    """

    def returnMostSignificant(self, iterable):
        ret = super(StateSignifier, self).returnMostSignificant(iterable)
        ret = wrap(copy.copy(ret))
        ret.timestamp = newest_timestamp(iterable)

        return ret


def removeQuantity(func):
    """Decorate a function to remove QuantityValue input

    NOTE: This decorator does not cast to base units!
    """
    @wraps(func)
    def wrapper(*args, **kwargs):
        new_args = []
        new_kwargs = {}
        for value in tuple(args):
            value = value.value if isinstance(value, QuantityValue) else value
            new_args.append(value)
        for name, value in kwargs.items():
            value = value.value if isinstance(value, QuantityValue) else value
            new_kwargs.update({name: value})
        return func(*new_args, **new_kwargs)
    return wrapper
