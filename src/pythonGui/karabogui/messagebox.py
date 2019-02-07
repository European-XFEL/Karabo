#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on June 1, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from textwrap import dedent

from PyQt4.QtGui import QMessageBox


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
    message_box = QMessageBox(parent=parent)
    message_box.setWindowTitle(title)
    message_box.setIcon(icon)
    message_box.setText(dedent(text))
    message_box.setModal(False)
    if parent is not None:
        return message_box.open()

    return message_box.exec()
