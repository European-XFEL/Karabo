#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on June 1, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from textwrap import dedent

from PyQt4.QtGui import QMessageBox


def show_alarm(text, title="Alarm", modal=True):
    _show_message_box(QMessageBox.Critical, text, title, modal)


def show_error(text, title="Error", modal=True):
    _show_message_box(QMessageBox.Critical, text, title, modal)


def show_information(text, title="Information", modal=True):
    _show_message_box(QMessageBox.Information, text, title, modal)


def show_warning(text, title="Warning", modal=True):
    _show_message_box(QMessageBox.Warning, text, title, modal)


def _show_message_box(icon, text, title, modal):
    """A wrapper to simplify the different message box styles defined below.
    """
    message_box = QMessageBox(icon, title, dedent(text), parent=None)
    message_box.setModal(modal)
    return message_box.exec()
