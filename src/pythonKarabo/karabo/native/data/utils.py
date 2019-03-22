import numpy as np

from .hash import Type


def dtype_from_number(number):
    """Return the corresponding dtype matching the Karabo Types to
    numpy dtypes

    >> dtype_from_number(16)
    >> dtype('int64')
    """
    return np.dtype(Type.types[number].numpy)
