#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on May 7, 2021
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from enum import Enum

from qtpy.QtCore import QEvent, QMargins, Qt
from qtpy.QtGui import QPalette
from qtpy.QtWidgets import (
    QApplication, QStyle, QStyledItemDelegate, QStyleOptionButton)


def set_fill_rect(painter, option, index):
    """Update the rectangle of the given `painter` depending on the given
    `options`
    """
    if option.state & QStyle.State_Selected:
        if option.state & QStyle.State_Active:
            painter.fillRect(option.rect, option.palette.highlight())
        elif not (option.state & QStyle.State_HasFocus):
            painter.fillRect(option.rect,
                             option.palette.brush(QPalette.Background))
    else:
        brush = index.data(Qt.BackgroundRole)
        if brush is not None:
            painter.fillRect(option.rect, brush)


class ButtonState(Enum):
    PRESSED = QStyle.State_Enabled | QStyle.State_Sunken
    ENABLED = QStyle.State_Enabled | QStyle.State_Raised | QStyle.State_Off
    DISABLED = QStyle.State_On


MARGINS = QMargins(4, 4, 4, 4)


class TableButtonDelegate(QStyledItemDelegate):
    """A QStyledItemDelegate for slot buttons
    """

    def __init__(self, parent=None):
        super(TableButtonDelegate, self).__init__(parent)
        self._enabled = True
        self._button_states = {}

    def paint(self, painter, option, index):
        """Reimplemented function of QStyledItemDelegate."""
        set_fill_rect(painter, option, index)
        self._draw_button(painter, option, index)

    def editorEvent(self, event, model, option, index):
        """Reimplemented function of QStyledItemDelegate."""
        handled_types = (QEvent.MouseButtonPress, QEvent.MouseButtonRelease,
                         QEvent.MouseMove)
        if event.type() in handled_types:
            self._handle_event_state(event, option, index)
            return True
        return super(TableButtonDelegate, self).editorEvent(
            event, model, option, index)

    def _draw_button(self, painter, option, index):
        if not self.isEnabled(index):
            state = ButtonState.DISABLED
        else:
            key = (index.row(), index.column())
            state = self._button_states.get(key, ButtonState.ENABLED)
        button = QStyleOptionButton()
        button.state = state.value
        button.rect = option.rect
        button.text = self.get_button_text(index) or "No text"
        button.features = QStyleOptionButton.AutoDefaultButton
        QApplication.style().drawControl(QStyle.CE_PushButton, button, painter)

    def _handle_event_state(self, event, option, index):
        if not self.isEnabled(index):
            # We are disabled but still clickable. We prevent that our action
            # is executed here!
            return
        key = (index.row(), index.column())
        state = self._button_states.get(key, ButtonState.ENABLED)
        # Remove some margins in case the mouse moves to bail out the pressed
        # button state!
        rect = option.rect
        rect = rect.marginsRemoved(MARGINS)
        if rect.contains(event.pos()):
            if event.type() == QEvent.MouseButtonPress:
                state = ButtonState.PRESSED
        else:
            state = ButtonState.ENABLED
        if (state == ButtonState.PRESSED and
                event.type() == QEvent.MouseButtonRelease):
            state = ButtonState.ENABLED
            self.click_action(index)
        self._button_states[key] = state

    # -----------------------------------------------------------------------
    # Public interface

    def isEnabled(self, index=None):
        """Return if the ButtonDelegate is enabled for `index`

        This method can be used to subclass for a specific index
        """
        return self._enabled

    def setEnabled(self, enable):
        """Set globally if the ButtonDelegate is enabled"""
        self._enabled = enable
        self._button_states = {}

    # -----------------------------------------------------------------------
    # Abstract interface

    def get_button_text(self, index):
        """Subclass this method to define the button text for `index`"""

    def click_action(self, index):
        """Subclass this method to define the action for `index`"""