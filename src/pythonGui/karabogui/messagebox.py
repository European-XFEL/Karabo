#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on June 1, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from textwrap import dedent

from PyQt5.QtWidgets import QMessageBox

from karabogui.dialogs.messagebox import KaraboMessageBox


def show_alarm(text, title="Alarm", details=None, parent=None):
    _show_message_box(QMessageBox.Critical, text, title, details, parent)


def show_error(text, title="Error", details=None, parent=None):
    _show_message_box(QMessageBox.Critical, text, title, details, parent)


def show_information(text, title="Information", details=None, parent=None):
    _show_message_box(QMessageBox.Information, text, title, details, parent)


def show_warning(text, title="Warning", details=None, parent=None):
    _show_message_box(QMessageBox.Warning, text, title, details, parent)


def _show_message_box(icon, text, title, details=None, parent=None):
    """A wrapper to simplify the different message box styles defined below.

    Defining a parent provides a modal non-blocking dialog, otherwise the
    messagebox will block the eventloop!
    """
    message_box = KaraboMessageBox(parent=parent)
    message_box.setWindowTitle(title)
    message_box.setIcon(icon)
    message_box.setText(dedent(text))
    if details is not None:
        message_box.setDetailedText(details)

    if parent is not None:
        return message_box.open()

    return message_box.exec()
