from collections.abc import Sequence

from .exceptions import SceneWriterException


def get_numbers(names, element):
    """ Read a list of integer values from an `Element` instance.
    """
    return {name: float(element.get(name)) for name in names}


def set_numbers(names, model, element, xmlnames=None):
    """ Copy a list of integer attribute values to an `Element` instance.

    This is the inverse of `_get_numbers`.
    """
    EPSILON = 1e-2
    if xmlnames is None:
        xmlnames = names
    elif not isinstance(xmlnames, Sequence) or len(names) != len(xmlnames):
        msg = "XML names must match up with object names"
        raise SceneWriterException(msg)

    def _convert(num):
        if abs(num - int(num)) > EPSILON:
            return str(num)
        return str(int(num))

    for modelname, xmlname in zip(names, xmlnames):
        element.set(xmlname, _convert(getattr(model, modelname)))
