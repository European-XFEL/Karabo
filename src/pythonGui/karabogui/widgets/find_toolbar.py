from pathlib import Path

from qtpy import uic
from qtpy.QtCore import Signal, Slot
from qtpy.QtWidgets import QWidget

from karabogui import icons


class FindToolBar(QWidget):
    findRequested = Signal(str, bool, bool)
    aboutToClose = Signal()
    highlightRequested = Signal(str, bool)

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        ui_file_path = Path(__file__).parent.joinpath("ui", "find_widget.ui")
        uic.loadUi(ui_file_path, self)
        self.find_next.clicked.connect(self.findNext)
        self.find_previous.clicked.connect(self.findPrevious)
        self.replace_checkbox.toggled.connect(self.setReplaceWidgetsVisibility)
        self.setReplaceWidgetsVisibility(False)

        self.find_next.setIcon(icons.arrowDown)
        self.find_previous.setIcon(icons.arrowUp)
        self.close_button.setIcon(icons.close)
        self.close_button.setStyleSheet("border: 0px;")
        self.close_button.clicked.connect(self.close)

        self.find_line_edit.returnPressed.connect(self.findNext)
        self.find_line_edit.textChanged.connect(self._onSearchTextChanged)
        self.match_case.toggled.connect(self._onMatchCaseToggled)

    @Slot(bool)
    def setReplaceWidgetsVisibility(self, toggled):
        for widget in (self.replace_label, self.replace_line_edit,
                       self.replace_button, self.replace_all_button):
            widget.setVisible(toggled)

    @Slot()
    def findNext(self):
        self._findRequest()

    @Slot()
    def findPrevious(self):
        self._findRequest(find_backward=True)

    def _findRequest(self, find_backward=False):
        text = self.find_line_edit.text()
        case_sensitive = self.match_case.isChecked()
        self.findRequested.emit(text, case_sensitive, find_backward)

    @Slot(int)
    def setResultText(self, count):
        text = "No Result"
        if count:
            text = f"Found {count} hits"
        self.result_label.setText(text)

    @Slot(bool)
    def _onMatchCaseToggled(self, match_case):
        text = self.find_line_edit.text()
        self._highlightRequest(text, match_case)

    @Slot(str)
    def _onSearchTextChanged(self, text):
        case_sensitive = self.match_case.isChecked()
        self._highlightRequest(text, case_sensitive)

    def _highlightRequest(self, text, match_case):
        self.highlightRequested.emit(text, match_case)

    def close(self):
        self.aboutToClose.emit()
        super().close()
