#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 4, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QEvent, QRect, Qt
from PyQt4.QtGui import (
    QApplication, QStyle, QStyleOptionButton, QStyledItemDelegate
)

from karabo_gui.icons import slot as slot_icon
from karabo_gui.schema import SlotNode
from .utils import ButtonState, handle_default_state

ICON_SIZE = 32
ICON_PADDING = 3
slot_pixmap = slot_icon.pixmap(ICON_SIZE)


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
        obj = model.index_ref(index)
        if (obj is None or
                not isinstance(getattr(obj, 'descriptor', None), SlotNode)):
            super(SlotButtonDelegate, self).paint(painter, option, index)
            return

        if option.state & QStyle.State_Selected:
            if option.state & QStyle.State_Active:
                painter.fillRect(option.rect, option.palette.highlight())
            elif not (option.state & QStyle.State_HasFocus):
                painter.fillRect(option.rect, option.palette.background())

        self._draw_icon(painter, option)
        self._draw_button(painter, option, index, obj)

    def editorEvent(self, event, model, option, index):
        """Reimplemented function of QStyledItemDelegate.
        """
        handled_types = (QEvent.MouseButtonPress, QEvent.MouseButtonRelease)
        if event.type() in handled_types:
            box = model.index_ref(index)
            descriptor = getattr(box, 'descriptor', None)
            if box is not None and isinstance(descriptor, SlotNode):
                self._handle_event_state(box, event, option)
                return True

        return super(SlotButtonDelegate, self).editorEvent(
            event, model, option, index)

    def _draw_button(self, painter, option, index, box):
        """Draw a button
        """
        key = box.key()
        state = self._button_states.get(key, ButtonState.DISABLED)
        button_state = handle_default_state(box.isAllowed(), state)
        self._button_states[key] = state
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
        pix_rect = QRect(rect.x() + ICON_PADDING, rect.center().y() - h/2,
                         w, h)
        QApplication.style().drawItemPixmap(painter, pix_rect, Qt.AlignCenter,
                                            slot_pixmap)

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
