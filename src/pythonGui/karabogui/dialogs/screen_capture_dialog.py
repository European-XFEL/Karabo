from qtpy import uic
from qtpy.QtCore import Qt, QTimer, Slot
from qtpy.QtGui import QFont
from qtpy.QtWidgets import (
    QApplication, QDialog, QDialogButtonBox, QLabel, QWidget)

from karabogui.const import IS_LINUX_SYSTEM
from karabogui.dialogs.logbook_preview import LogBookPreview
from karabogui.dialogs.utils import get_dialog_ui


class ScreenNumberPopoup(QWidget):
    def __init__(self, screen, screen_number, parent=None):
        super().__init__(parent=parent)
        self.screen = screen
        self.screen_number = screen_number

        # Show at the middle of the screen
        qtRectangle = self.frameGeometry()
        centerPoint = self.screen.availableGeometry().center()
        qtRectangle.moveCenter(centerPoint)
        self.move(qtRectangle.topLeft())

        self.setWindowFlags(
            Qt.FramelessWindowHint |
            Qt.WindowStaysOnTopHint |
            Qt.Tool
        )

        # Enable transparency
        self.setAttribute(Qt.WA_TranslucentBackground)
        self.setWindowOpacity(100)

        # Label to show number
        label = QLabel(str(screen_number), self)
        label.setAlignment(Qt.AlignCenter)

        font = QFont()
        font.setPointSize(80)
        font.setBold(True)
        label.setFont(font)

        label.setStyleSheet("color: white;")

        self.resize(300, 200)
        label.resize(self.size())
        duration_ms = 2000  # 2 seconds
        QTimer.singleShot(duration_ms, self.close)


class ScreenCaptureDialog(QDialog):

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        ui_file = get_dialog_ui("screen_capture.ui")
        uic.loadUi(ui_file, self)

        self.ok_button = self.buttonBox.button(QDialogButtonBox.Ok)
        self.ok_button.setText("Capture")

        self.show_screen_number_button.clicked.connect(self.show_screen_number)

        screens = QApplication.screens()
        counts = [str(i) for i in range(1, len(screens)+1)]
        self.screen_combobox.addItems(counts)

    def __repr__(self):
        return "Screen Capture"

    @Slot()
    def show_screen_number(self):
        screens = QApplication.screens()
        for number, screen in enumerate(screens, start=1):
            popup = ScreenNumberPopoup(screen, number, parent=self)
            popup.show()

    def capture_screen(self):
        screen = int(self.screen_combobox.currentText())
        return self._capture_from_screen(screen)

    def _capture_from_screen(self, screen_number: int):
        screen = QApplication.screens()[screen_number-1]
        if IS_LINUX_SYSTEM:
            # On linux Composition Manager allows to capture from each screen.
            pixmap = screen.grabWindow(0)
        else:
            # Mac and Windows, all the screens are captured as one, we need
            # to crop.
            geometry = screen.geometry()
            pixmap = screen.grabWindow(0, geometry.x(), geometry.y(),
                                       geometry.width(), geometry.height())
        return pixmap

    def info(self):
        """To satisfy the logbook dialog."""
        return None

    def accept(self):
        pixmap = self.capture_screen()
        logbook_dialog = LogBookPreview(pixmap=pixmap, parent=self)
        logbook_dialog.show()
