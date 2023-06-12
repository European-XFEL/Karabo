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
import logging

from qtpy.QtCore import QObject, QPoint, Qt, Signal, Slot
from qtpy.QtWidgets import (
    QFrame, QHBoxLayout, QMenu, QPushButton, QTextEdit, QVBoxLayout)

import karabogui.icons as icons

# The main status bar logger entity
_logger = logging.getLogger('KaraboGUI')
_logger.setLevel(logging.INFO)

LOG_HEIGHT = 75
CLEAR_BUTTON_SIZE = 25


class LogMediator(QObject):
    """A mediator class for the `logger` to make sure logging is handled
    gracefully by Qt"""
    signal = Signal(str, logging.LogRecord)


class QtHandler(logging.Handler):
    def __init__(self, slotfunc, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.signaller = LogMediator()
        self.signaller.signal.connect(slotfunc)
        formatter = logging.Formatter(
            fmt='%(asctime)s - %(levelname)s - %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S')
        self.setFormatter(formatter)

    def emit(self, record):
        s = self.format(record)
        self.signaller.signal.emit(s, record)


class StatusLogWidget(QFrame):
    """A StatusLog widget holding a QTextEdit widget for log messages

    The log widget can color the log messages according to their severity.
    """
    COLORS = {
        logging.DEBUG: 'Blue',
        logging.INFO: 'Black',
        logging.WARNING: 'DarkOrange',
        logging.ERROR: 'Red',
        logging.CRITICAL: 'Purple',
    }

    def __init__(self, parent=None):
        super().__init__(parent)
        horizontal_layout = QHBoxLayout(self)
        horizontal_layout.setContentsMargins(0, 0, 0, 0)
        self.log_widget = QTextEdit(self)
        self.log_widget.setReadOnly(True)
        self.log_widget.setFixedHeight(LOG_HEIGHT)

        # Configure the log handler
        self.handler = QtHandler(self.update_status)
        logger = get_logger()
        logger.addHandler(self.handler)

        # Transparent background and no focus
        viewport = self.log_widget.viewport()
        viewport.setAutoFillBackground(False)
        self.log_widget.setFocusPolicy(Qt.NoFocus)
        self.log_widget.setContextMenuPolicy(Qt.CustomContextMenu)
        self.log_widget.customContextMenuRequested.connect(
            self._show_context_menu)

        horizontal_layout.addWidget(self.log_widget)
        vertical_layout = QVBoxLayout()
        vertical_layout.setContentsMargins(0, 0, 0, 0)
        vertical_layout.addStretch(1)
        button = QPushButton()
        button.setFixedSize(CLEAR_BUTTON_SIZE, CLEAR_BUTTON_SIZE)
        button.setFocusPolicy(Qt.NoFocus)
        button.setToolTip('Clear log')
        button.setIcon(icons.editClear)
        button.clicked.connect(self.log_widget.clear)
        vertical_layout.addWidget(button)
        horizontal_layout.addLayout(vertical_layout)

    @Slot(str, logging.LogRecord)
    def update_status(self, status, record):
        color = self.COLORS.get(record.levelno, 'black')
        s = f'<font color="{color}">{status}</font>'
        self.log_widget.append(s)
        bar = self.log_widget.verticalScrollBar()
        bar.setValue(bar.maximum())

    @Slot(QPoint)
    def _show_context_menu(self, pos):
        """Show a context menu"""
        menu = QMenu(self.log_widget)
        select_action = menu.addAction('Select All')
        select_action.triggered.connect(self.log_widget.selectAll)
        enable_select = len(self.log_widget.toPlainText()) > 0
        select_action.setEnabled(enable_select)

        copy_action = menu.addAction('Copy Selected')
        copy_action.triggered.connect(self.log_widget.copy)
        enable_copy = not self.log_widget.textCursor().selection().isEmpty()
        copy_action.setEnabled(enable_copy)
        menu.exec(self.log_widget.viewport().mapToGlobal(pos))


def get_logger():
    """Convenience function to get the logger to the gui status"""
    return _logger
