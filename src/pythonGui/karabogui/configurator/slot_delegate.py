#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 4, 2017
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
from qtpy.QtCore import QEvent, QRect, Qt
from qtpy.QtWidgets import (
    QApplication, QStyle, QStyledItemDelegate, QStyleOptionButton)

import karabogui.access as krb_access
from karabo.native import AccessLevel
from karabogui import icons
from karabogui.binding.api import DeviceProxy, SlotBinding

from .utils import (
    ButtonState, get_device_state_string, handle_default_state, set_fill_rect)

ICON_SIZE = 32
ICON_PADDING = 3
__slot_pixmap = None


def _get_button_rect(rect):
    """Adjust a Qt-provided rectangle to the appropriate bounds

    XXX: Revisit this later. It's not clear what the appropriate way to get
    this information is. `QStyle` has methods which should help, but
    QStyle.getSubelementRect(QStyle.SE_ItemViewItemDecoration, ...) doesn't
    quite return the correct rectangle for a row's icon. Not sure why...
    """
    slot_pixmap = _slot_pixmap()
    rect.setLeft(rect.x() + slot_pixmap.size().width() + ICON_PADDING * 2)
    return rect


def _slot_pixmap():
    """Lazily load the slot icon"""
    global __slot_pixmap
    if __slot_pixmap is None:
        __slot_pixmap = icons.slot.pixmap(ICON_SIZE)
    return __slot_pixmap


class SlotButtonDelegate(QStyledItemDelegate):
    """A QStyledItemDelegate for slot buttons
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self._button_states = {}
        self._global_access = krb_access.GLOBAL_ACCESS_LEVEL

    def paint(self, painter, option, index):
        """Reimplemented function of QStyledItemDelegate."""
        model = index.model()
        proxy = model.index_ref(index)
        if not isinstance(getattr(proxy, 'binding', None), SlotBinding):
            super().paint(painter, option, index)
            return

        set_fill_rect(painter, option, index)

        self._draw_icon(painter, option)
        self._draw_button(painter, option, index, proxy)

    def editorEvent(self, event, model, option, index):
        """Reimplemented function of QStyledItemDelegate."""
        handled_types = (QEvent.MouseButtonPress, QEvent.MouseButtonRelease)
        if event.type() in handled_types:
            proxy = model.index_ref(index)
            if isinstance(getattr(proxy, 'binding', None), SlotBinding):
                self._handle_event_state(proxy, event, option)
                return True

        return super().editorEvent(
            event, model, option, index)

    def setAccessLevel(self, level: AccessLevel):
        self._global_access = level

    def _draw_button(self, painter, option, index, proxy):
        """Draw a button for this slot delegate"""
        key = proxy.key
        state = self._button_states.get(key, ButtonState.DISABLED)
        allowed = self.is_allowed(proxy)
        button_state = handle_default_state(allowed, state)
        self._button_states[key] = state
        button = QStyleOptionButton()
        button.state = button_state
        button.rect = _get_button_rect(option.rect)
        button.text = index.data()
        button.features = QStyleOptionButton.AutoDefaultButton
        QApplication.style().drawControl(QStyle.CE_PushButton, button, painter)

    def _draw_icon(self, painter, option):
        """Draw a slot icon"""
        rect = option.rect
        slot_pixmap = _slot_pixmap()
        pix_size = slot_pixmap.size()
        w, h = pix_size.width(), pix_size.height()
        pix_rect = QRect(rect.x() + ICON_PADDING, rect.center().y() - h // 2,
                         w, h)
        QApplication.style().drawItemPixmap(painter, pix_rect, Qt.AlignCenter,
                                            slot_pixmap)

    def is_allowed(self, proxy):
        is_device = isinstance(proxy.root_proxy, DeviceProxy)
        allowed = (is_device and proxy.binding.is_allowed(
            get_device_state_string(proxy.root_proxy))
                   and (self._global_access >=
                        proxy.binding.required_access_level))
        return allowed

    def _handle_event_state(self, proxy, event, option):
        """Determine the state of a given proxy's button during user
        interaction.
        """
        key = proxy.key
        state = self._button_states.get(key, ButtonState.DISABLED)
        if self.is_allowed(proxy):
            rect = _get_button_rect(option.rect)
            if rect.contains(event.pos()):
                if event.type() == QEvent.MouseButtonPress:
                    state = ButtonState.PRESSED
                elif state == ButtonState.PRESSED:
                    proxy.execute()
            if (state == ButtonState.PRESSED and
                    event.type() == QEvent.MouseButtonRelease):
                state = ButtonState.ENABLED
        self._button_states[key] = state
        return state
