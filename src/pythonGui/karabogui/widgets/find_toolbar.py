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
from pathlib import Path

from qtpy import uic
from qtpy.QtCore import Signal, Slot
from qtpy.QtWidgets import QToolButton, QWidget

from karabogui import icons


class FindToolBar(QWidget):
    findRequested = Signal(str, bool, bool)
    aboutToClose = Signal()
    highlightRequested = Signal(str, bool)

    replaceRequested = Signal(str, str, bool, bool)

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        ui_file_path = Path(__file__).parent.joinpath("ui", "find_widget.ui")
        uic.loadUi(ui_file_path, self)
        self.find_next.clicked.connect(self.findNext)
        self.find_previous.clicked.connect(self.findPrevious)

        self.find_next.setIcon(icons.arrowDown)
        self.find_previous.setIcon(icons.arrowUp)
        self.close_button.setIcon(icons.close)
        self.close_button.setStyleSheet("border: 0px;")
        self.close_button.clicked.connect(self.close)

        self.find_line_edit.returnPressed.connect(self.findNext)
        self.find_line_edit.textChanged.connect(self._onSearchTextChanged)
        self.match_case.toggled.connect(self._onMatchCaseToggled)

        self.replace_button.clicked.connect(self.requestReplace)
        self.replace_all_button.clicked.connect(self.requestReplaceAll)
        self.show_replace_tool_button.toggled.connect(
            self.set_replace_widgets_visibility)
        button = self.find_line_edit.findChild(QToolButton)
        if button is not None:
            button.setIcon(icons.close_small)
        button = self.replace_line_edit.findChild(QToolButton)
        if button is not None:
            button.setIcon(icons.close_small)

    @Slot(bool)
    def set_replace_widgets_visibility(self, visible):
        for widget in (self.replace_label, self.replace_line_edit,
                       self.replace_button, self.replace_all_button):
            widget.setVisible(visible)
        icon = icons.arrowDown if visible else icons.arrowRight
        self.show_replace_tool_button.setIcon(icon)

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
        result = "Results"
        if count == 1:
            result = "Result"
        text = f"{count} {result}"
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

    @Slot()
    def requestReplaceAll(self):
        self.requestReplace(replace_all=True)

    @Slot()
    def requestReplace(self, replace_all=False):
        text = self.find_line_edit.text()
        new_text = self.replace_line_edit.text()
        match_case = self.match_case.isChecked()
        self.replaceRequested.emit(text, new_text, match_case, replace_all)

    def close(self):
        self.aboutToClose.emit()
        super().close()
