#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on June 1, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from textwrap import dedent

from qtpy.QtCore import QPoint, Qt
from qtpy.QtWidgets import QMessageBox

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

    # If no parent is provided, we can place it in the center of the
    # main window if available!
    main_window = get_panel_wrangler().main_window
    if main_window is not None:
        move_to_widget(main_window, message_box)
    return message_box.exec()


def move_to_widget(reference_widget, widget):
    """Move a widget to the center of reference widget"""
    if reference_widget.windowState() & Qt.WindowMinimized:
        return
    main_center = reference_widget.rect().center()
    widget_ref = QPoint(widget.width() / 2, widget.height() / 2)

    global_center = reference_widget.mapToGlobal(main_center - widget_ref)
    widget.move(global_center)
