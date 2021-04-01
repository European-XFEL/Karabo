import numpy as np
from traits.api import BaseFloat, BaseRange


class NumpyRange(BaseRange):
    """A fast-validating trait type who casts to a `numpy` dtype on validate
    """

    def __init__(self, dtype=None, **kwargs):
        self._dtype = dtype
        super(NumpyRange, self).__init__(**kwargs)

    def validate(self, object, name, value):
        """ Validate that the value is in the specified range.
        """
        if type(value) is str:
            try:
                return self._dtype(value)
            except (OverflowError, ValueError):
                self.error(object, name, value)

        return self._dtype(super().validate(object, name, value))


class Float(BaseFloat):
    """ A fast-validating trait type who can handle numpy dtypes and coerce
    """

    def validate(self, object, name, value):
        """ Validates that a specified value is valid for this trait."""
        value_type = type(value)
        if value_type in (float, np.float32, np.float64):
            return value
        else:
            try:
                return float(value)
            except Exception:
                self.error(object, name, value)
