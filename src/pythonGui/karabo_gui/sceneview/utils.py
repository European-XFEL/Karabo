#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from contextlib import contextmanager


def calc_bounding_rect(collection):
    """ Compute the bounding rectangle for a collection of objects.

    Each object in the collection must have a geometry method which returns a
    QRect.
    """
    MAX_VALUE = 100000
    left, top, right, bottom = MAX_VALUE, MAX_VALUE, 0, 0
    for item in collection:
        rect = item.geometry()
        left = min(left, rect.left())
        right = max(right, rect.right())
        top = min(top, rect.top())
        bottom = max(bottom, rect.bottom())

    # Return x, y, width, height
    return (left, top, right - left, bottom - top)


@contextmanager
def save_painter_state(painter):
    painter.save()
    yield
    painter.restore()
