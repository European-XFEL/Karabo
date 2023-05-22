# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import numpy as np
from traits.api import BaseCBool, BaseFloat, BaseRange


class NumpyRange(BaseRange):
    """A fast-validating trait type who casts to a `numpy` dtype on validate
    """

    def __init__(self, dtype=None, **kwargs):
        self._dtype = dtype
        super().__init__(**kwargs)

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


class CBool(BaseCBool):
    """ A fast-validating trait type who can coerce, but does not accept `None`
    """

    def validate(self, object, name, value):
        """Reimplemented validate for `None` strings

        Note: `None` check can be removed when the configs are sanitized
        """
        if value in ("None", None):
            self.error(object, name, value)
        try:
            return bool(value)
        except Exception:
            self.error(object, name, value)
