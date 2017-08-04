#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 4, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from enum import Enum

from PyQt4.QtCore import QEvent, QRect, Qt
from PyQt4.QtGui import (
    QApplication, QStyle, QStyleOptionButton, QStyledItemDelegate
)

from karabo_gui.icons import slot as slot_icon
from karabo_gui.schema import SlotNode

ICON_SIZE = 32
ICON_PADDING = 3
slot_pixmap = slot_icon.pixmap(ICON_SIZE)


class ButtonState(Enum):
    PRESSED = QStyle.State_Enabled | QStyle.State_Sunken
    ENABLED = QStyle.State_Enabled | QStyle.State_Raised | QStyle.State_Off
    DISABLED = QStyle.State_On


def _get_button_rect(rect):
    """Adjust a Qt-provided rectangle to the appropriate bounds

    XXX: Revisit this later. It's not clear what the appropriate way to get
    this information is. `QStyle` has methods which should help, but
    QStyle.getSubelementRect(QStyle.SE_ItemViewItemDecoration, ...) doesn't
    quite return the correct rectangle for a row's icon. Not sure why...
    """
    rect.setLeft(rect.x() + slot_pixmap.size().width() + ICON_PADDING * 2)
    return rect


class SlotButtonDelegate(QStyledItemDelegate):
    """A QStyledItemDelegate for slot buttons
    """
    def __init__(self, parent=None):
        super(SlotButtonDelegate, self).__init__(parent)
        self._button_states = {}

    def paint(self, painter, option, index):
        """Reimplemented function of QStyledItemDelegate.
        """
        model = index.model()
        box = model.box_ref(index)
        if box is None or not isinstance(box.descriptor, SlotNode):
            super(SlotButtonDelegate, self).paint(painter, option, index)
            return

        self._draw_icon(painter, option)
        self._draw_button(painter, option, index, box)

    def editorEvent(self, event, model, option, index):
        """Reimplemented function of QStyledItemDelegate.
        """
        handled_types = (QEvent.MouseButtonPress, QEvent.MouseButtonRelease)
        if event.type() in handled_types:
            box = model.box_ref(index)
            if box is not None and isinstance(box.descriptor, SlotNode):
                self._handle_event_state(box, event, option)
                return True

        return super(SlotButtonDelegate, self).editorEvent(
            event, model, option, index)

    def _draw_button(self, painter, option, index, box):
        """Draw a button
        """
        button_state = self._handle_default_state(box)
        button = QStyleOptionButton()
        button.state = button_state.value
        button.rect = _get_button_rect(option.rect)
        button.text = index.data()
        button.features = QStyleOptionButton.AutoDefaultButton
        QApplication.style().drawControl(QStyle.CE_PushButton, button, painter)

    def _draw_icon(self, painter, option):
        """Draw a slot icon
        """
        rect = option.rect
        pix_size = slot_pixmap.size()
        w, h = pix_size.width(), pix_size.height()
        pix_rect = QRect(rect.x() + ICON_PADDING, rect.y(), w, h)
        QApplication.style().drawItemPixmap(painter, pix_rect, Qt.AlignCenter,
                                            slot_pixmap)

    def _handle_default_state(self, box):
        """Determine the resting state of a given box's button.
        """
        key = box.key()
        allowed = box.isAllowed()
        state = self._button_states.get(key, ButtonState.DISABLED)
        if allowed and state != ButtonState.PRESSED:
            state = ButtonState.ENABLED
        if not allowed:
            state = ButtonState.DISABLED
        self._button_states[key] = state
        return state

    def _handle_event_state(self, box, event, option):
        """Determine the state of a given box's button during user interaction.
        """
        key = box.key()
        state = self._button_states.get(key, ButtonState.DISABLED)
        if box.isAllowed():
            rect = _get_button_rect(option.rect)
            if rect.contains(event.pos()):
                if event.type() == QEvent.MouseButtonPress:
                    state = ButtonState.PRESSED
                elif state == ButtonState.PRESSED:
                    box.execute()
            if (state == ButtonState.PRESSED and
                    event.type() == QEvent.MouseButtonRelease):
                state = ButtonState.ENABLED
        self._button_states[key] = state
        return state
