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
from traits.api import (
    ArrayOrNone, HasStrictTraits, Int, Property, on_trait_change)

DEFAULT_STACK = 100


class RollImage(HasStrictTraits):
    """Class to construct a waterfall image from an array

    This class will build a rolling image from the input arrays up to a limit
    of `stack` size. The default stack size is `100`.

    Arrays can be added via:

    image = RollImage()
    image.add(...)

    The internal data is automatically reset if the `stack` size changes or
    the array input changes in `shape` or `dtype`.

    The rolling image can then be derived via the `data` property:

        data = image.data
    """

    data = Property
    stack = Int

    # Internal traits
    _slice_counter = Int
    _data = ArrayOrNone

    def _stack_default(self):
        return DEFAULT_STACK

    def _get_data(self):
        if self._data is None:
            return None

        return self._data[0:self._slice_counter + 1]

    @on_trait_change('stack')
    def invalidate(self):
        self._data = None

    # Public interface
    # -----------------------------------------------------------------------

    def reset(self):
        self.invalidate()

    def add(self, value):
        if value.size == 0:
            self._data = None
            return

        finite = np.isfinite(value)
        if not np.all(finite):
            value = np.array(value, copy=True)
            value[~finite] = 0

        needs_reset = (self._data is None
                       or (value.shape[0] != self._data.shape[1])
                       or (value.dtype != self._data.dtype))
        if needs_reset:
            self._data = np.vstack([value for _ in range(self.stack)])
            self._slice_counter = 0

        elif self._slice_counter < self.stack:
            self._slice_counter += 1
        self._data = np.roll(self._data, 1, axis=0)
        self._data[0] = value
