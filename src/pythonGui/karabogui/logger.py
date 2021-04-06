import logging

from qtpy.QtCore import Signal, Slot, QObject, Qt
from qtpy.QtWidgets import (
    QFrame, QHBoxLayout, QPushButton, QTextEdit, QVBoxLayout)

import karabogui.icons as icons

# The main status bar logger entity
_logger = logging.getLogger('KaraboGUI')
_logger.setLevel(logging.DEBUG)

LOG_HEIGHT = 75
CLEAR_BUTTON_SIZE = 25


class LogMediator(QObject):
    """A mediator class for the `logger` to make sure logging is handled
    gracefully by Qt"""
    signal = Signal(str, logging.LogRecord)


class QtHandler(logging.Handler):
    def __init__(self, slotfunc, *args, **kwargs):
        super(QtHandler, self).__init__(*args, **kwargs)
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
        logging.DEBUG: 'blue',
        logging.INFO: 'black',
        logging.WARNING: 'orange',
        logging.ERROR: 'red',
        logging.CRITICAL: 'purple',
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
        s = '<font color="%s">%s</font>' % (color, status)
        self.log_widget.append(s)
        bar = self.log_widget.verticalScrollBar()
        bar.setValue(bar.maximum())


def get_logger():
    """Convenience function to get the logger to the gui status"""
    return _logger
