import numpy as np
from traits.api import ArrayOrNone, HasStrictTraits, Int, Property

DEFAULT_STACK = 100


class RollImage(HasStrictTraits):
    data = Property
    _data = ArrayOrNone
    stack = Int
    sliceCounter = Int

    def _stack_default(self):
        return DEFAULT_STACK

    def _get_data(self):
        if self._data is None:
            return
        return self._data[0:self.sliceCounter + 1]

    def _stack_changed(self):
        self._data = None

    def add(self, value):
        if (self._data is None or (value.shape[0] != self._data.shape[1])
                or (value.dtype != self._data.dtype)):
            self._data = np.vstack(value for _ in range(self.stack))
            self.sliceCounter = 0
        elif self.sliceCounter < self.stack:
            self.sliceCounter += 1
        self._data = np.roll(self._data, 1, axis=0)
        self._data[0] = value
