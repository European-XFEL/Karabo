#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on June 1, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from textwrap import dedent

from PyQt5.QtCore import pyqtSlot, QTimer
from PyQt5.QtWidgets import QAbstractButton, QMessageBox

MESSAGE_POPUP = 60  # Seconds


class KaraboMessageBox(QMessageBox):
    """A pop up message box which will automatically close after
    `MESSAGE_POPUP` seconds!
    """
    def __init__(self, parent=None):
        super(KaraboMessageBox, self).__init__(parent)
        self.setModal(False)
        self.setDefaultButton(QMessageBox.Ok)
        self.buttonClicked.connect(self.buttonPressed)

        # Set up a ticker to eventually close the pop if not already closed!
        self._ticker = QTimer(self)
        self._ticker.setInterval(MESSAGE_POPUP * 1000)
        self._ticker.setSingleShot(True)
        self._ticker.timeout.connect(self.close)
        # We already start ticking here!
        self._ticker.start()

    @pyqtSlot(QAbstractButton)
    def buttonPressed(self, button):
        """Handle the `buttonClicked` signal of the `QMessageBox`

        If the ticker is still `active`, we gracefully stop it here!
        """
        if self._ticker.isActive():
            self._ticker.stop()


def show_alarm(text, title="Alarm", parent=None):
    _show_message_box(QMessageBox.Critical, text, title, parent)


def show_error(text, title="Error", parent=None):
    _show_message_box(QMessageBox.Critical, text, title, parent)


def show_information(text, title="Information", parent=None):
    _show_message_box(QMessageBox.Information, text, title, parent)


def show_warning(text, title="Warning", parent=None):
    _show_message_box(QMessageBox.Warning, text, title, parent)


def _show_message_box(icon, text, title, parent=None):
    """A wrapper to simplify the different message box styles defined below.

    Defining a parent provides a modal non-blocking dialog, otherwise the
    messagebox will block the eventloop!
    """
    message_box = KaraboMessageBox(parent=parent)
    message_box.setWindowTitle(title)
    message_box.setIcon(icon)
    message_box.setText(dedent(text))
    if parent is not None:
        return message_box.open()

    return message_box.exec()
