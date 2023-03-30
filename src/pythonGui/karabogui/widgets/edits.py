from qtpy.QtCore import Qt, Slot
from qtpy.QtGui import QPalette
from qtpy.QtWidgets import QLineEdit


class LineEditEditor(QLineEdit):
    """A colored `LineEdit` depending on acceptable input

    Note: This widget is included as promoted widgets in *.ui files.
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self._normal_palette = self.palette()
        self._error_palette = QPalette(self._normal_palette)
        self._error_palette.setColor(QPalette.Text, Qt.red)
        self.textChanged.connect(self._check_background)

    @Slot(str)
    def _check_background(self, text):
        acceptable_input = self.hasAcceptableInput()
        palette = (self._normal_palette if acceptable_input
                   else self._error_palette)
        self.setPalette(palette)
