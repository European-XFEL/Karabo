import os.path as op

from PyQt5 import uic
from PyQt5.QtCore import pyqtSlot, QSize, Qt, QTimer
from PyQt5.QtWidgets import QApplication, QDialog, QMessageBox, QStyle

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
    def __init__(self, parent=None):
        super(KaraboMessageBox, self).__init__(parent)
        uic.loadUi(op.join(op.dirname(__file__), "messagebox.ui"), self)
        self.setWindowFlags(self.windowFlags() | Qt.WindowStaysOnTopHint)
        # Set focus on the OK button
        self.ok_button.clicked.connect(self.close)
        self.ok_button.setFocus()

        # Initialize the "Show details" part of the widget
        self.show_details_button.clicked.connect(self._show_details)
        self.show_details_button.setVisible(False)
        self.details_widget.setVisible(False)
        self._is_showing_details = False
        self._size_hint = QSize()

        # Set up a ticker to eventually close the pop if not already closed!
        self._ticker = QTimer(self)
        self._ticker.setInterval(MESSAGE_POPUP * 1000)
        self._ticker.setSingleShot(True)
        self._ticker.timeout.connect(self.close)
        # We already start ticking here!
        self._ticker.start()

    # ----------------------------------------------------------------------
    # Qt methods

    def closeEvent(self, event):
        """Close the ticker before closing the dialog"""
        if self._ticker.isActive():
            self._ticker.stop()

        # Calling the super method will not close the dialog for messageboxes
        # with details. We just accept the event here.
        event.accept()

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

    @pyqtSlot()
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
