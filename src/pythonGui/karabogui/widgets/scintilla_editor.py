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
import io
import os
import re
from collections import defaultdict
from contextlib import redirect_stdout
from tempfile import mkstemp

from pycodestyle import Checker, StyleGuide
from pyflakes.api import check
from qtpy.Qsci import QsciAPIs, QsciScintilla
from qtpy.QtCore import (
    Property as pyqtProperty, QRegularExpression, Qt, Signal, Slot)
from qtpy.QtGui import QColor, QKeySequence
from qtpy.QtWidgets import QShortcut, QVBoxLayout, QWidget

from karabogui.widgets.find_toolbar import FindToolBar
from karabogui.widgets.scintilla_api import get_symbols
from karabogui.widgets.scintilla_lexer import PythonLexer

AUTO_COMPLETION_THRESHOLD = 3
LINE_LENGTH = 79
LINE_HIGHLIGHT = QColor("#FFFFCC")
MARGIN_BACKGROUND = QColor("#E0E0E0")
MARGIN_FOREGROUND = QColor("#A0A0A0")

# These are arbitrary numbers to represent an indicator style. There may be
# up to 32 types of indicator defined at a time. The first 8 are normally
# used by lexers.
ERROR_INDICATOR = 19
HIGHLIGHT_INDICATOR = 20
STYLE_ISSUE_INDICATOR = 21

# To match result from pycodestyle as filename:line_no:col_no:pep8Code message
# For example, "test.py:3:1: E302 expected 2 blank lines, found 1"
STYLE_REGEX = re.compile(r".*:(\d+):(\d+):\s+(.*)")

# To match result from pyflakes as filename:line_no:col_no: message
# For example, "test.py:1:1 'karabo.middlelayer.Int32' imported but unused"
FLAKE_REGEX = re.compile(r".*:(\d+):(\d+):?\s+(.*)")

IGNORED_STYLE_CODES = (
    "W292",  # No new line at the end
    "W293",  # Blank line contains whitespace
    )


class CodeBook(QWidget):
    """Python Code editor class based on Scintilla"""

    codeChanged = Signal()
    qualityChecked = Signal(bool)

    def __init__(self, code=None, parent=None):
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setSpacing(0)
        layout.setContentsMargins(0, 0, 0, 0)

        code_editor = CodeEditor(parent=self)
        if code is not None:
            code_editor.setText(code)

        code_editor.textChanged.connect(self.codeChanged)
        code_editor.textChanged.connect(self._mayHighlight)
        code_editor.resultFound.connect(self.updateResultText)
        code_editor.qualityChecked.connect(self.qualityChecked)

        find_toolbar = FindToolBar(parent=self)
        find_toolbar.findRequested.connect(self._findNext)
        find_toolbar.highlightRequested.connect(self._highlight)
        find_toolbar.aboutToClose.connect(self._clearHighlight)
        find_toolbar.replaceRequested.connect(self._replace)
        find_toolbar.setVisible(False)

        find = QShortcut(QKeySequence(Qt.CTRL + Qt.Key_F), self)
        find.activated.connect(self.showFindToolbar)

        replace = QShortcut(QKeySequence(Qt.CTRL + Qt.Key_R), self)
        replace.activated.connect(self.showReplaceToolbar)

        self.find_toolbar = find_toolbar
        self.code_editor = code_editor
        layout.addWidget(self.find_toolbar)
        layout.addWidget(self.code_editor)

    @Slot()
    def showFindToolbar(self):
        self.find_toolbar.setVisible(True)
        selected_text = self.code_editor.selectedText()
        # Consider the text from previous search.
        search_text = self.find_toolbar.find_line_edit.text()
        match_case = self.find_toolbar.match_case.isChecked()
        if selected_text:
            self.find_toolbar.find_line_edit.setText(selected_text)
            search_text = selected_text
        self.code_editor.highlight(search_text, match_case)
        self.find_toolbar.find_line_edit.setFocus()
        self.find_toolbar.show_replace_tool_button.setChecked(False)
        self.find_toolbar.set_replace_widgets_visibility(False)

    @Slot()
    def showReplaceToolbar(self):
        self.showFindToolbar()
        self.find_toolbar.show_replace_tool_button.setChecked(True)

    @Slot(str, bool, bool)
    def _findNext(self, text, match_case, find_backward):
        self.code_editor.find_match(text, match_case, find_backward)

    @Slot(str, str, bool, bool)
    def _replace(self, text, new_text, match_case, replace_all):
        if replace_all:
            self.code_editor.replace_all(text, new_text, match_case)
        else:
            self.code_editor.replace_text(text, new_text, match_case)

    @Slot(int)
    def updateResultText(self, count):
        self.find_toolbar.setResultText(count)

    @Slot(str, bool)
    def _highlight(self, text, match_case):
        self.code_editor.highlight(text, match_case)

    @Slot()
    def _clearHighlight(self):
        self.code_editor.clearHighlight()

    @Slot()
    def _mayHighlight(self):
        """
        Update the search highlight on code change, if Find Toolbar is open.
        """
        if not self.find_toolbar.isVisible():
            return
        text = self.find_toolbar.find_line_edit.text()
        match_case = self.find_toolbar.match_case.isChecked()
        self._highlight(text, match_case)

    def increaseFontSize(self):
        self.code_editor.zoomIn()

    def decreaseFontSize(self):
        self.code_editor.zoomOut()

    def setReadOnly(self, value):
        """ Set the editor read only or not. """
        self.code_editor.setReadOnly(value)

    def moveCursorToLine(self, line_number, offset):
        self.code_editor.setCursorPosition(line_number, offset)

    def getEditorCode(self):
        """ Get the current code from the editor """
        return self.code_editor.text()

    def checkCode(self):
        self.code_editor.checkCodeQuality()

    def clearIndicators(self):
        self.code_editor.clearAnnotations()
        self.code_editor.clearAllIndicators()

    def clear(self):
        self.code_editor.clear()


class CodeEditor(QsciScintilla):
    """
    Python Code editor class.
    """

    resultFound = Signal(int)
    qualityChecked = Signal(bool)

    def __init__(self, parent=None, use_api=True):
        super().__init__(parent)

        # Indentation
        self.setTabWidth(4)
        self.setAutoIndent(True)
        self.setIndentationsUseTabs(False)
        self.setIndentationGuides(True)
        self.setBackspaceUnindents(True)
        self.setIndentationGuidesBackgroundColor(MARGIN_BACKGROUND)
        self.setIndentationGuidesForegroundColor(MARGIN_BACKGROUND)

        # Auto completion
        self.setAutoCompletionSource(QsciScintilla.AcsAll)
        self.setAutoCompletionReplaceWord(True)
        self.setAutoCompletionCaseSensitivity(False)
        self.autoCompleteFromDocument()
        self.setAutoCompletionThreshold(AUTO_COMPLETION_THRESHOLD)

        # Highlight the current line
        self.setCaretLineVisible(True)
        self.setCaretLineBackgroundColor(LINE_HIGHLIGHT)

        # lexer
        lexer = PythonLexer(parent=self)

        # Show line numbers
        font = lexer.font(PythonLexer.Default)
        self.setMarginsFont(font)
        self.setMarginWidth(0, "0000")
        self.setMarginLineNumbers(0, True)
        self.setMarginsBackgroundColor(MARGIN_BACKGROUND)
        self.setMarginsForegroundColor(MARGIN_FOREGROUND)

        # Brace matching: highlight the brace when the cursor is on left or
        # on right
        self.setBraceMatching(QsciScintilla.SloppyBraceMatch)

        # Code folding
        self.setFolding(QsciScintilla.PlainFoldStyle)
        self.setFoldMarginColors(MARGIN_BACKGROUND, MARGIN_BACKGROUND)

        # A vertical line to indicate the line-length limit
        self.setEdgeMode(QsciScintilla.EdgeLine)
        self.setEdgeColumn(LINE_LENGTH)
        self.setEdgeColor(QColor("gray"))

        if use_api:
            self._apis = QsciAPIs(lexer)
            for symbol in get_symbols():
                self._apis.add(symbol)
            self._apis.prepare()

        self.setLexer(lexer)

        self._highlights = []
        self.indicatorDefine(self.StraightBoxIndicator, HIGHLIGHT_INDICATOR)
        self.indicatorDefine(self.SquiggleLowIndicator, ERROR_INDICATOR)
        self.indicatorDefine(self.SquiggleLowIndicator, STYLE_ISSUE_INDICATOR)
        self.setIndicatorForegroundColor(QColor("red"), ERROR_INDICATOR)
        self.setIndicatorForegroundColor(QColor("blue"), STYLE_ISSUE_INDICATOR)
        self.has_annotation = False
        self.hit_count = 0

    @Slot(str, bool, bool)
    def find_match(self, text, match_case, find_backward):
        """
        Select the first match from the current cursor position.
        Return True if found else False.
        """
        regular_expression = False
        word_only = False
        wrap = True
        find_forward = not find_backward
        line, index = self.getCursorPosition()
        if find_backward:
            # Move cursor behind by one, so that backward search doesn't get
            # stuck on the just single hit.
            index -= 1
        found = self.findFirst(text, regular_expression, match_case, word_only,
                               wrap, find_forward, line, index)
        if not found:
            self.clearHighlight()
        return found

    def replace_text(self, text, new_text, match_case):
        """
        Replace the first hit of search text with 'new_text' and select the
        next hit. If the search text is not selected already, just select
        the next hit.
        """
        if self.hasSelectedText():
            selected_text = self.selectedText()
            if not match_case:
                selected_text = selected_text.lower()
                text = text.lower()
            if selected_text == text:
                self.replaceSelectedText(new_text)
        self.find_match(text, match_case, find_backward=False)

    def replace_all(self, text, new_text, match_case):
        """
        Recursively replace a text with a new text. Replace all would work
        only if the search text is already highlighted - which always
        happen first in the workflow.
        """
        found = self.find_match(text, match_case, find_backward=False)
        if found:
            self.beginUndoAction()
            for _ in range(self.hit_count):
                if self.findNext():
                    self.replace(new_text)
            self.endUndoAction()

    @Slot(str, bool)
    def highlight(self, text, match_case):
        """ Highlight all the occurrence of the given text."""
        self.clearHighlight()
        if text == "":
            return
        options = QRegularExpression().patternOptions()
        if not match_case:
            options = options | QRegularExpression.CaseInsensitiveOption
        pattern = QRegularExpression(text, options)
        if not pattern.isValid():
            return
        matches = pattern.globalMatch(self.text())

        hit_count = 0
        while matches.hasNext():
            match = matches.next()
            if match.capturedLength() != len(pattern.pattern()):
                # Avoid matching regular expression.
                continue
            start = match.capturedStart()
            end = match.capturedEnd()
            _range = self._range_from_position(start, end)
            line_start, index_start, line_end, index_end = _range

            selection = {"line_start": line_start,
                         "index_start": index_start,
                         "line_end": line_end,
                         "index_end": index_end}
            self._highlights.append(selection)
            self.fillIndicatorRange(
                line_start, index_start, line_end, index_end,
                HIGHLIGHT_INDICATOR)
            hit_count += 1
        self.hit_count = hit_count
        self.resultFound.emit(hit_count)

    def clearHighlight(self):
        for selection in self._highlights:
            self.clearIndicatorRange(
                selection["line_start"],
                selection["index_start"],
                selection["line_end"],
                selection["index_end"],
                HIGHLIGHT_INDICATOR
            )
        self._highlights.clear()
        self.hit_count = 0
        self.resultFound.emit(self.hit_count)

    def _range_from_position(self, start_position, end_position):
        """
        Convert a position which is globally indexed from the start of the
        text to a combination of line number and index from the start of the
        line.
        """
        start_line, start_offset = self.lineIndexFromPosition(start_position)
        end_line, end_offset = self.lineIndexFromPosition(end_position)
        return start_line, start_offset, end_line, end_offset

    def checkCodeQuality(self):
        """ Run 'pyflakes' and 'pycodestyle' to check the code quality"""
        self.clearAnnotations()
        self.clearAllIndicators()

        code = self.text()
        indicators = defaultdict(list)

        flake_feedback = check_flake(code)
        for line_number, feedback in flake_feedback.items():
            indicators[line_number].extend(feedback)

        style_feedback = check_style(code)
        for line_number, feedback in style_feedback.items():
            indicators[line_number].extend(feedback)
        error = False
        if flake_feedback or style_feedback:
            self._annotate_code(indicators)
            error = True
            self.has_annotation = True
        self.qualityChecked.emit(error)

    def _annotate_code(self, indicators):
        for line_number, descriptions in indicators.items():
            messages = []
            # Editor follows zero based indexing.
            line_number = line_number - 1
            for description in descriptions:
                message = description.get("message")
                messages.append(message)
                line_text = self.text(line_number)
                col_end = len(line_text)
                col_start = col_end - len(line_text.strip()) - 1
                indicator = (ERROR_INDICATOR if description.get(
                    "is_error", False) else STYLE_ISSUE_INDICATOR)
                self.fillIndicatorRange(line_number, col_start,
                                        line_number, col_end, indicator)
            message = "\n".join(messages)
            self.annotate(line_number,  message, self.annotationDisplay())

    def clearAllIndicators(self):
        line_start = 0
        col_start = 0
        line_end = self.lines()
        col_end = len(self.text(line_end-1))
        self.clearIndicatorRange(
            line_start, col_start, line_end, col_end, ERROR_INDICATOR
        )
        self.clearIndicatorRange(
            line_start, col_start, line_end, col_end, STYLE_ISSUE_INDICATOR
        )
        self.has_annotation = False

    @pyqtProperty(str)
    def textForSquish(self):
        """
        This is only for the Squish test. Squish can't normally access a
        method in a custom Qt Widget, unless it is a Slot or Property.
        Wrapping this method with pyqtProperty so that Squish can read
        access the editor text as a property.
        """
        return super().text()


class FlakeReporter:
    """
    This class imitates 'pyflakes.reporter.Reporter' class but
    additionally stores the report as 'log' instead of just printing out.
    """
    def __init__(self):
        self.log = defaultdict(list)

    def unexpectedError(self, filename, message):
        self.log[0].append({"filename": filename,
                            "message": str(message)
                            })

    def syntaxError(self, filename, message, line_no, column, source):
        self.log[line_no].append({
            "message": str(message),
            "is_error": True,
        })

    def flake(self, message):
        matcher = FLAKE_REGEX.match(str(message))
        if matcher:
            line_no, col, msg = matcher.groups()
            line_no = int(line_no)
            self.log[line_no].append({
                "message": msg,
            })


def check_flake(code):
    reporter = FlakeReporter()
    # Skipping the filename as it is only used to mention in error report.
    check(code, filename="", reporter=reporter)
    return reporter.log


def check_style(code):
    code_fd, code_filename = mkstemp()
    os.close(code_fd)

    with open(code_filename, "w") as fh:
        fh.write(code)

    style = StyleGuide(config_file="")  # Optionally can add config file here.
    ignore_codes = style.options.ignore + IGNORED_STYLE_CODES
    style.options.ignore = ignore_codes
    checker = Checker(code_filename, options=style.options)
    with redirect_stdout(io.StringIO()) as f:
        checker.check_all()
    results = f.getvalue()

    os.remove(code_filename)

    style_feedback = defaultdict(list)
    for result in results.split("\n"):
        matcher = STYLE_REGEX.match(result)
        if matcher:
            line_no, col, msg = matcher.groups()
            line_no = int(line_no)
            code, description = msg.split(" ", 1)
            style_feedback[line_no].append({
                "message": description,
                "code": code,
            })
    return style_feedback
