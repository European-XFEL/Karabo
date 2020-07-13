#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from contextlib import contextmanager
import math

from PyQt5.QtCore import QPoint
from PyQt5.QtGui import QFont, QFontMetrics

from .const import GRID_STEP, SCREEN_MAX_VALUE


def calc_bounding_rect(collection):
    """Compute the bounding rectangle for a collection of objects.

    Each object in the collection must have a geometry method which returns a
    QRect.
    """
    left, top, right, bottom = SCREEN_MAX_VALUE, SCREEN_MAX_VALUE, 0, 0
    for item in collection:
        rect = item.geometry()
        left = min(left, rect.left())
        top = min(top, rect.top())

        # Qt right and bottom are always width - 1 and height - 1,
        # respectively. Therefore, we correct it.
        right = max(right, rect.right() + 1)
        bottom = max(bottom, rect.bottom() + 1)

    # Return an empty rect when nothing is there
    if left == SCREEN_MAX_VALUE and top == SCREEN_MAX_VALUE:
        return (0, 0, 0, 0)

    # Return x, y, width, height
    return (left, top, right - left, bottom - top)


def calc_rect_from_text(font, text):
    """Compute the rectangle to fit the given ``text``.

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


def round_down_to_grid(num):
    """Round down to the nearest grid step"""
    return math.floor(num / GRID_STEP) * GRID_STEP


def round_up_to_grid(num):
    """Round up to the nearest grid step"""
    return math.ceil(num / GRID_STEP) * GRID_STEP


def calc_snap_pos(pos):
    """Calculate the snap position by rounding down the x-y points to the
       nearest grid"""
    return QPoint(round_down_to_grid(pos.x()), round_down_to_grid(pos.y()))
