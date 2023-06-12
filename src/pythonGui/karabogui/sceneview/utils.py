#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 7, 2016
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
#############################################################################
import math
from contextlib import contextmanager

from qtpy.QtCore import QPoint
from qtpy.QtGui import QFontMetrics

from karabogui.fonts import get_qfont

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
    q_font = get_qfont(font)
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


def calc_rotated_point(x, y, angle=0, scale=1):
    """Rotate a point with the angle and the scale.
       Angle should be in degrees."""

    radians = math.radians(angle)
    cos, sin = math.cos(radians), math.sin(radians)

    rotated_x = x * cos + y * sin
    rotated_y = - x * sin + y * cos

    return QPoint(int(rotated_x * scale), int(rotated_y * scale))


def add_offset(model, x=0, y=0):
    # Check if the models has x or y, as there are some that do not follow
    # the item interface. We do nothing instead.
    trait_names = model.copyable_trait_names()
    if "x" not in trait_names or "y" not in trait_names:
        return

    model.trait_setq(x=model.x + x, y=model.y + y)

    # Recursively offset the children
    if "children" in model.trait_names():
        for child in model.children:
            add_offset(child, x=x, y=y)


def calc_relative_pos(models):
    x, y = (SCREEN_MAX_VALUE, SCREEN_MAX_VALUE)
    for model in models:
        x = min(x, model.x)
        y = min(y, model.y)
    return x, y
