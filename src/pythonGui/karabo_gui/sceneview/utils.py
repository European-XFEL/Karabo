#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from contextlib import contextmanager

from PyQt4.QtGui import QFont, QFontMetrics

from .const import SCREEN_MAX_VALUE
from karabo_gui import icons


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


def get_status_symbol_as_icon(status):
    """ Map the given `status` to an icon and return it. """
    status_icons = {
        'requested': icons.device_requested,
        'schema': icons.device_schema,
        'dead': icons.device_dead,
        'noserver': icons.deviceOfflineNoServer,
        'noplugin': icons.deviceOfflineNoPlugin,
        'incompatible': icons.deviceIncompatible,
        'offline': icons.deviceOffline,
        'alive': icons.deviceAlive,
        'missing': icons.propertyMissing,
        'error': icons.device_error
    }
    return status_icons.get(status)


def get_status_symbol_as_pixmap(status, extent=None):
    """ Map the `status` to a pixmap and return it.

        `extent` sets the size of the pixmap. The pixmap might be smaller than
        requested, but never larger.
    """
    if extent is None:
        return get_status_symbol_as_icon(status).pixmap()
    return get_status_symbol_as_icon(status).pixmap(extent)
