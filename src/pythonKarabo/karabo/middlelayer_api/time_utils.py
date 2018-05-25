from collections import Iterable

from karabo.common.states import StateSignifier as SignifierBase

from .basetypes import wrap_function, wrap_methods


@wrap_function
def maximum(iterable):
    """Return the maximum value of the iterable

    This function takes into account KaraboValues for newest timestamp
    generation.
    """
    assert isinstance(iterable, Iterable)

    return max(iterable)


@wrap_function
def minimum(iterable):
    """Return the minimum value of the iterable

    This function takes into account KaraboValues for newest timestamp
    generation.
    """
    assert isinstance(iterable, Iterable)

    return min(iterable)


@wrap_function
def mean(sequence):
    """Return the mean value of the sequence

    This function takes into account KaraboValues for newest timestamp
    generation.
    """
    assert isinstance(sequence, Iterable)

    return sum(sequence) / len(sequence)


@wrap_methods
class StateSignifier(SignifierBase):
    """Wrapper of the StateSignifier to provide newest timestamp
    """
