# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
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
