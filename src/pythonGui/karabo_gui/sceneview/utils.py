#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from contextlib import contextmanager

from PyQt4.QtGui import QFont, QFontMetrics

from karabo_gui import icons
from karabo_gui.const import (
    STATE_OFFLINE, STATE_ALIVE, STATE_MONITORING, STATE_REQUESTED,
    STATE_SCHEMA, STATE_DEAD, STATE_NOSERVER, STATE_NOPLUGIN,
    STATE_INCOMPATIBLE, STATE_MISSING, STATE_ERROR)
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

    # Return an empty rect when nothing is there
    if left == SCREEN_MAX_VALUE and top == SCREEN_MAX_VALUE:
        return (0, 0, 0, 0)

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


def get_status_symbol_as_icon(status, error):
    """ A `QIcon` for the given `status` is returned. """
    status_icons = {
        STATE_OFFLINE: icons.deviceOffline,
        STATE_ALIVE: icons.deviceAlive,
        STATE_REQUESTED: icons.device_requested,
        STATE_SCHEMA: icons.device_schema,
        STATE_DEAD: icons.device_dead,
        STATE_NOSERVER: icons.deviceOfflineNoServer,
        STATE_NOPLUGIN: icons.deviceOfflineNoPlugin,
        STATE_INCOMPATIBLE: icons.deviceIncompatible,
        STATE_MISSING: icons.propertyMissing,
        STATE_ERROR: icons.device_error
    }

    if status == STATE_MONITORING and not error:
        return None
    elif status != STATE_OFFLINE and error:
        return status_icons.get('error')

    return status_icons.get(status)


def get_status_symbol_as_pixmap(status, error, extent=16):
    """ A `QPixmap` for the given `status` is returned.

        `extent` sets the size of the pixmap. The pixmap might be smaller than
        requested, but never larger.
    """
    icon = get_status_symbol_as_icon(status, error)
    if icon is not None:
        return icon.pixmap(extent)
