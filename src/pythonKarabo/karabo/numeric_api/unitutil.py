from functools import wraps
import numpy

from karabo.middlelayer import QuantityValue


@wraps(numpy.linspace)
def linspace(start, stop, *args, **kwargs):
    if not (isinstance(start, QuantityValue) or
            isinstance(stop, QuantityValue)):
        return numpy.linspace(start, stop, *args, **kwargs)

    stop = float(stop / start.units)
    return numpy.linspace(start.magnitude, stop, *args, **kwargs) * start.units


@wraps(numpy.dot)
def dot(a, b, *args, **kwargs):
    if not (isinstance(a, QuantityValue) or isinstance(b, QuantityValue)):
        return numpy.dot(a, b, *args, **kwargs)

    if not isinstance(a, QuantityValue):
        a = QuantityValue(a)

    if not isinstance(b, QuantityValue):
        b = QuantityValue(b)

    return numpy.dot(a.magnitude, b.magnitude) * (a.units * b.units)


@wraps(numpy.cross)
def cross(a, b, *args, **kwargs):
    if not (isinstance(a, QuantityValue) or isinstance(b, QuantityValue)):
        return numpy.cross(a, b, *args, **kwargs)

    if not isinstance(a, QuantityValue):
        a = QuantityValue(a)

    if not isinstance(b, QuantityValue):
        b = QuantityValue(b)

    return numpy.cross(a.magnitude, b.magnitude) * (a.units * b.units)
