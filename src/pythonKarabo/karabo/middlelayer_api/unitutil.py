# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import copy
from asyncio import iscoroutinefunction
from collections.abc import Iterable
from functools import wraps

import numpy as np

from karabo.common.states import StateSignifier as SignifierBase
from karabo.native import (
    KaraboValue, QuantityValue, newest_timestamp, wrap, wrap_function)


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
        ret = super().returnMostSignificant(iterable)
        ret = wrap(copy.copy(ret))
        ret.timestamp = newest_timestamp(iterable)

        return ret


def removeQuantity(func):
    """Decorate a function to remove QuantityValue/KaraboValue input

    This function works as well with async declarations and can be used as::

    @removeQuantity
    def calculate(x, y):
        assert not isinstance(x, KaraboValue)
        assert not isinstance(y, KaraboValue)
        return x, y

    Note: This decorator does not cast to base units! In case of booleans,
          they can be compared by identity.
    """

    def _convert_input(args, kwargs):
        n_args = [
            value.value if isinstance(value, KaraboValue) else value
            for value in tuple(args)]
        n_kwargs = {
            key: value.value if isinstance(value, KaraboValue) else value
            for key, value in kwargs.items()}
        return n_args, n_kwargs

    if iscoroutinefunction(func):

        @wraps(func)
        async def wrapper(*args, **kwargs):
            n_args, n_kwargs = _convert_input(args, kwargs)
            return await func(*n_args, **n_kwargs)
    else:
        @wraps(func)
        def wrapper(*args, **kwargs):
            n_args, n_kwargs = _convert_input(args, kwargs)
            return func(*n_args, **n_kwargs)

    return wrapper
