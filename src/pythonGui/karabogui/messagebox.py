#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on June 1, 2017
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
#############################################################################
from textwrap import dedent

from qtpy.QtCore import QEventLoop, QPoint, Qt
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import QApplication, QMessageBox

from karabogui.dialogs.messagebox import KaraboMessageBox
from karabogui.singletons.api import get_panel_wrangler


def show_alarm(text, title="Alarm", details=None, parent=None):
    return _show_message_box(QMessageBox.Critical, text, title,
                             details, parent)


def show_error(text, title="Error", details=None, parent=None):
    return _show_message_box(QMessageBox.Critical, text, title,
                             details, parent)


def show_information(text, title="Information", details=None, parent=None):
    return _show_message_box(QMessageBox.Information, text, title,
                             details, parent)


def show_warning(text, title="Warning", details=None, parent=None):
    return _show_message_box(QMessageBox.Warning, text, title,
                             details, parent)


def _show_message_box(icon, text, title, details=None, parent=None):
    """A wrapper to simplify the different message box styles defined below.
    """
    message_box = KaraboMessageBox(parent=parent)
    message_box.setWindowTitle(title)
    message_box.setIcon(icon)
    message_box.setText(dedent(text))
    if details is not None:
        message_box.setDetailedText(details)

    if parent is not None:
        return message_box.open()

    _move_to_main_window(message_box)
    return message_box.exec()


def _move_to_main_window(msg_box):
    """Move a message box to the center of main window if available"""
    main_window = get_panel_wrangler().main_window
    if main_window is None or main_window.windowState() & Qt.WindowMinimized:
        return
    main_center = main_window.rect().center()
    msg_box_ref = QPoint(msg_box.width() // 2, msg_box.height() // 2)
    global_center = main_window.mapToGlobal(main_center - msg_box_ref)
    msg_box.move(global_center)


# --------------------------------------

def _move_to_cursor(msg_box):
    """Move the message box to the cursor"""
    pos = QCursor.pos()
    size = msg_box.size()

    x = pos.x() - size.width() // 2
    if x > 0:
        pos.setX(x)
    y = pos.y() - size.height() // 2
    if y > 0:
        pos.setY(y)
    msg_box.move(pos)


def _show_cursor_box(icon, text, title, details=None):
    message_box = KaraboMessageBox(parent=None)
    message_box.setWindowTitle(title)
    message_box.setIcon(icon)
    message_box.setText(dedent(text))
    if details is not None:
        message_box.setDetailedText(details)

    _move_to_cursor(message_box)
    QApplication.processEvents(QEventLoop.AllEvents, 100)
    return message_box.exec()


def show_error_at_cursor(text, title="Error", details=None):
    return _show_cursor_box(QMessageBox.Critical, text, title, details)
