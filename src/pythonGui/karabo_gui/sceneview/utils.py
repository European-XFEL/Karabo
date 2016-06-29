#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from contextlib import contextmanager

from PyQt4.QtGui import QFont, QFontMetrics

from .const import SCREEN_MAX_VALUE


def calc_bounding_rect(collection):
    """ Compute the bounding rectangle for a collection of objects.

    Each object in the collection must have a geometry method which returns a
    QRect.
    """
    left, top, right, bottom = SCREEN_MAX_VALUE, SCREEN_MAX_VALUE, 0, 0
    for item in collection:
        rect = item.geometry()
        left = min(left, rect.left())
        right = max(right, rect.right())
        top = min(top, rect.top())
        bottom = max(bottom, rect.bottom())

    # Return x, y, width, height
    return (left, top, right - left, bottom - top)


def calc_rect_from_text(font, text):
    """ Compute the rectangle to fit the given ``text``.

    A tuple including x, y, width and height.
    """
    q_font = QFont()
    q_font.fromString(font)
    fm = QFontMetrics(q_font)
    CONTENT_MARGIN = 10
    width = fm.width(text) + CONTENT_MARGIN
    height = fm.height() + CONTENT_MARGIN

    return (0, 0, width, height)


@contextmanager
def save_painter_state(painter):
    painter.save()
    yield
    painter.restore()
