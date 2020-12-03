import numpy as np
from traits.api import BaseFloat, Range


class NumpyRange(Range):
    """A fast-validating trait type who casts to a `numpy` dtype on validate
    """

    def __init__(self, dtype=None, **kwargs):
        self._dtype = dtype
        super(NumpyRange, self).__init__(**kwargs)

    def validate(self, object, name, value):
        """ Validate that the value is in the specified range.

        Note: Once the value is validated we cast to the numpy dtype. This is
        safe!
        """
        ret = super().validate(object, name, value)
        return self._dtype(ret)


def _validate_float(value):
    """ Convert a Python object to a float, or raise TypeError.

    Note: This validator preserves the numpy `dtype` if present
    """
    value_type = type(value)
    if value_type in (float, np.float32, np.float64):
        return value
    else:
        try:
            nb_float = value_type.__float__
        except AttributeError:
            raise TypeError(
                "Object of type {!r} not convertible to float".format(
                    type(value)))
        return nb_float(value)


class Float(BaseFloat):
    """ A fast-validating trait type who can handle numpy dtypes
    """

    def validate(self, object, name, value):
        """ Validates that a specified value is valid for this trait."""
        try:
            return _validate_float(value)
        except TypeError:
            self.error(object, name, value)
