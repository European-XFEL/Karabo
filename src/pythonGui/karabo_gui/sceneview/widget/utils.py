import numbers

import numpy as np

from karabo_gui.topology import getDevice


def determine_if_value_unchanged(value, widget_value, box):
    """ Determine if a value in an editable widget has changed
    """
    value_unchanged = False
    if value is None:
        value_unchanged = False
    elif (isinstance(value, (numbers.Complex, np.inexact)) and
            not isinstance(value, numbers.Integral)):
        diff = abs(value - widget_value)
        abs_err = box.descriptor.absoluteError
        rel_err = box.descriptor.relativeError
        if abs_err is not None:
            value_unchanged = diff < abs_err
        elif rel_err is not None:
            value_unchanged = diff < abs(value * rel_err)
        else:
            value_unchanged = diff < 1e-4
    elif isinstance(value, list):
        if len(value) != len(widget_value):
            value_unchanged = False
        else:
            value_unchanged = True
            for i in range(len(value)):
                if value[i] != widget_value[i]:
                    value_unchanged = False
                    break
    else:
        value_unchanged = (str(value) == str(widget_value))

    return value_unchanged


def get_box(device_id, path):
    """ Return the Box for a given device and property path. """
    return getDevice(device_id).getBox(path.split("."))
