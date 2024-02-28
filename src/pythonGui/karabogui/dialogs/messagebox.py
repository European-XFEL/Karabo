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
from qtpy import uic
from qtpy.QtCore import QEventLoop, QPoint, QSize, Qt, QTimer, Slot
from qtpy.QtWidgets import QApplication, QDialog, QMenu, QMessageBox, QStyle

from .utils import get_dialog_ui

MESSAGE_POPUP = 60  # Seconds
MESSAGEBOX_STANDARD_PIXMAPS = {
    QMessageBox.Information: QStyle.SP_MessageBoxInformation,
    QMessageBox.Warning: QStyle.SP_MessageBoxWarning,
    QMessageBox.Critical: QStyle.SP_MessageBoxCritical,
}


class KaraboMessageBox(QDialog):
    """A pop up message box which will automatically close after
    `MESSAGE_POPUP` seconds!
    """
    existing = set()

    def __init__(self, parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui("messagebox.ui"), self)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.setModal(False)
        self.setWindowFlags(self.windowFlags() | Qt.WindowStaysOnTopHint)
        # Set focus on the OK button
        self.ok_button.clicked.connect(self.accept)
        self.ok_button.setFocus()

        # Initialize the "Show details" part of the widget
        self.show_details_button.clicked.connect(self._show_details)
        self.show_details_button.setVisible(False)
        self.details_widget.setVisible(False)
        self.details_widget.setContextMenuPolicy(Qt.CustomContextMenu)
        self.details_widget.customContextMenuRequested.connect(
            self._show_context_menu)
        self._is_showing_details = False
        self._size_hint = QSize()

        # Set up a ticker to eventually close the pop if not already closed!
        self._ticker = QTimer(self)
        self._ticker.setInterval(MESSAGE_POPUP * 1000)
        self._ticker.setSingleShot(True)
        self._ticker.timeout.connect(self.close)
        # We already start ticking here!
        self._ticker.start()
        self.existing.add(self)

    # ----------------------------------------------------------------------
    # Qt methods

    def closeEvent(self, event):
        """Close the ticker before closing the dialog"""
        if self._ticker.isActive():
            self._ticker.stop()
        # Graceful unregister with discard!
        self.existing.discard(self)
        super().closeEvent(event)
        flags = QEventLoop.AllEvents
        QApplication.processEvents(flags, 100)

    def sizeHint(self):
        return self._size_hint

    # ----------------------------------------------------------------------
    # Reimplemented QMessagebox methods

    def setIcon(self, icon):
        style = QApplication.style()
        icon = style.standardIcon(MESSAGEBOX_STANDARD_PIXMAPS[icon])
        icon_size = style.pixelMetric(QStyle.PM_MessageBoxIconSize)
        self.icon_label.setPixmap(icon.pixmap(icon_size, icon_size))

    def setText(self, text):
        """Adjust the dialog size after setting the text to recompute for an
           ideal size."""
        self.text_label.setText(text)
        self.adjustSize()

    def setDetailedText(self, details):
        """Adjust the dialog size after setting the text to recompute for an
           ideal size."""
        self.show_details_button.setVisible(True)
        self.details_textedit.setText(details)
        self.adjustSize()

    # ----------------------------------------------------------------------
    # Private methods

    @Slot()
    def _show_details(self):
        """Show/hide the details widget area and modify the button text when
           the "Show(Hide) Details..." button is clicked. This also recomputes
           for the ideal size of the dialog."""
        button_text = {False: "Show Details...", True: "Hide Details..."}
        self._is_showing_details = not self._is_showing_details
        self._size_hint = self.size()

        self.show_details_button.setText(button_text[self._is_showing_details])
        self.details_widget.setVisible(self._is_showing_details)
        self.adjustSize()

    @Slot(QPoint)
    def _show_context_menu(self, pos):
        """Show a context menu"""
        menu = QMenu(self)
        widget = self.details_textedit
        select_action = menu.addAction('Select All')
        select_action.triggered.connect(widget.selectAll)
        enable_select = len(widget.toPlainText()) > 0
        select_action.setEnabled(enable_select)

        copy_action = menu.addAction('Copy Selected')
        copy_action.triggered.connect(widget.copy)
        enable_cp = not widget.textCursor().selection().isEmpty()
        copy_action.setEnabled(enable_cp)
        menu.exec(widget.viewport().mapToGlobal(pos))
