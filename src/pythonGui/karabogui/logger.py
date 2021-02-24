import logging

from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import (
    QFrame, QHBoxLayout, QPushButton, QTextEdit, QVBoxLayout)

import karabogui.icons as icons

# The main status bar logger entity
_logger = logging.getLogger('KaraboGUI')
_logger.setLevel(logging.DEBUG)

LOG_HEIGHT = 75
CLEAR_BUTTON_SIZE = 25


class StatusBarHandler(logging.Handler):
    """A logging handler holding a QTextEdit widget for the status bar
    """

    def __init__(self):
        super().__init__(level=logging.DEBUG)
        self.widget = QFrame()
        horizontal_layout = QHBoxLayout(self.widget)
        horizontal_layout.setContentsMargins(0, 0, 0, 0)
        self.log_widget = QTextEdit(self.widget)
        self.log_widget.setFixedHeight(LOG_HEIGHT)

        # Transparent background and no focus
        viewport = self.log_widget.viewport()
        viewport.setAutoFillBackground(False)
        self.log_widget.setFocusPolicy(Qt.NoFocus)

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

        formatter = logging.Formatter(
            fmt='%(asctime)s - %(levelname)s - %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S')
        self.setFormatter(formatter)

    def emit(self, record):
        self.log_widget.append(self.format(record))
        bar = self.log_widget.verticalScrollBar()
        bar.setValue(bar.maximum())

    def get_widget(self):
        return self.widget

    def get_log_widget(self):
        return self.log_widget


def get_logger():
    """Convenience function to get the logger to the gui status"""
    return _logger
