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
import pytest

from karabogui.widgets.scintilla_api import create_symbols
from karabogui.widgets.scintilla_editor import (
    ERROR_INDICATOR, HIGHLIGHT_INDICATOR, STYLE_ISSUE_INDICATOR, CodeBook,
    CodeEditor, FlakeReporter, check_style)

MULTILINE_CODE = """
This is a dummy code
containing multiple lines
Number 1
Number 2
Number 3
"""

REAL_CODE = """from karabo.middlelayer import Macro, MacroSlot, String
class New(Macro):  # miss blank lines above
    name = String(defaultValue="New")

    @MacroSlot()
    def execute(self):
        print("Hello {}!".format(self.name))
        x =
"""


@pytest.fixture()
def code_editor(gui_app):
    editor = CodeEditor(use_api=False)
    yield editor
    editor.destroy()


@pytest.fixture()
def code_book(gui_app):
    book = CodeBook()
    yield book
    book.destroy()


def test_scintilla_editor(code_book):
    symbols = create_symbols()
    assert len(symbols) > 150

    editor = code_book.code_editor

    editor.setText("from karabo.native import Hash")
    # Basic default features
    assert editor.tabWidth() == 4
    assert editor.autoIndent()
    assert editor.indentationGuides()
    assert not editor.indentationsUseTabs()
    assert code_book.getEditorCode() is not None


def test_find_match(code_editor):
    code_editor.setText(MULTILINE_CODE)

    def assert_mouse_position(line, index):
        assert code_editor.getCursorPosition() == (line, index)

    assert_mouse_position(6, 0)

    assert code_editor.find_match("Number", False, False)
    assert code_editor.selectedText() == "Number"
    assert_mouse_position(3, 6)

    assert not code_editor.find_match("number", match_case=True,
                                      find_backward=False)

    assert code_editor.find_match("number", match_case=False,
                                  find_backward=False)
    assert_mouse_position(4, 6)

    assert code_editor.find_match("number", match_case=False,
                                  find_backward=False)
    assert_mouse_position(5, 6)

    # Loop over the code.
    assert code_editor.find_match("number", match_case=False,
                                  find_backward=False)
    assert_mouse_position(3, 6)

    # Search backward
    assert code_editor.find_match("number", match_case=False,
                                  find_backward=True)
    assert_mouse_position(5, 6)

    # Non-existing text
    assert not code_editor.find_match("foo", match_case=False,
                                      find_backward=True)


def test_replace(code_editor):
    code_editor.setText(MULTILINE_CODE)

    def assert_mouse_position(line, index):
        assert code_editor.getCursorPosition() == (line, index)

    assert_mouse_position(6, 0)
    match_case = False
    # When no selection, just selects the next hit.
    code_editor.replace_text("number", "Line", match_case)
    assert_mouse_position(3, 6)
    assert code_editor.selectedText() == "Number"

    # Replace first occurrence and select the next
    code_editor.replace_text("number", "Line", match_case)
    assert_mouse_position(4, 6)
    assert code_editor.selectedText() == "Number"
    expected = MULTILINE_CODE.replace("Number", "Line", 1)
    assert code_editor.text() == expected

    # Replace all
    code_editor.highlight("Number", match_case)
    code_editor.replace_all("Number", "Line", match_case)
    expected = MULTILINE_CODE.replace("Number", "Line")
    assert code_editor.text() == expected

    # Case-sensitive replace. Should not change the 'lines' in 2nd line
    match_case = True
    code_editor.highlight("Line", match_case)
    code_editor.replace_all("Line", "Number", match_case)
    assert code_editor.text() == MULTILINE_CODE

    # Undo should revert all the replaces together.
    code_editor.undo()
    assert code_editor.text() == expected


def test_highlight(code_editor):
    """ Test for highlight and clear highlight"""
    code_editor.setText(MULTILINE_CODE)

    def has_highlight(pos):
        """Check if the given position has highlight"""
        # This scintilla api returns 1 if has highlight else 0.
        return bool(code_editor.SendScintilla(
            code_editor.SCI_INDICATORVALUEAT, HIGHLIGHT_INDICATOR, pos))

    code_editor.highlight("Number", match_case=False)
    expected = [
        {'line_start': 3, 'index_start': 0, 'line_end': 3, 'index_end': 6},
        {'line_start': 4, 'index_start': 0, 'line_end': 4, 'index_end': 6},
        {'line_start': 5, 'index_start': 0, 'line_end': 5, 'index_end': 6}]

    assert code_editor._highlights == expected

    # Highlight position as key line number and value (index_start, index_end)
    highlights = {3: (0, 6), 4: (0, 6), 5: (0, 6)}
    # Test only the expected positions has highlight in the entire code.
    for pos in range(len(code_editor.text())):
        line, index = code_editor.lineIndexFromPosition(pos)
        highlight_expected = False
        if line in highlights:
            highlight_expected = index in range(*highlights[line])
        assert has_highlight(pos) == highlight_expected

    # Clear highlight
    code_editor.clearHighlight()
    assert code_editor._highlights == []

    # No letter should have highlight
    for pos in range(len(code_editor.text())):
        assert not has_highlight(pos)


def test_highlight_from_find_toolbar(code_book):
    """
    Typing/clearing the search text in the Find Toolbar should highlight it
    and also update the label in the toolbar.
    """
    code_editor = code_book.code_editor
    code_editor.setText(MULTILINE_CODE)
    find_toolbar = code_book.find_toolbar
    find_toolbar.find_line_edit.setText("is")
    assert len(code_editor._highlights) == 2
    assert find_toolbar.result_label.text() == "2 Results"
    find_toolbar.find_line_edit.clear()
    assert len(code_editor._highlights) == 0
    assert find_toolbar.result_label.text() == "0 Results"


def test_code_quality_check(code_book):
    """Test the Code Quality checking in the Macro Editor"""

    code_editor = code_book.code_editor
    code_editor.setText(REAL_CODE)
    code_book.checkCode()
    style_issue_lines = set()
    error_lines = set()
    for pos in range(len(code_editor.text())):
        if code_editor.SendScintilla(
                code_editor.SCI_INDICATORVALUEAT, STYLE_ISSUE_INDICATOR, pos):
            line, _ = code_editor.lineIndexFromPosition(pos)
            style_issue_lines.add(line)
        if code_editor.SendScintilla(
                code_editor.SCI_INDICATORVALUEAT, ERROR_INDICATOR, pos):
            line, _ = code_editor.lineIndexFromPosition(pos)
            error_lines.add(line)

    # Line numbers are zero indexed.
    assert style_issue_lines == {1}
    assert error_lines == {7}

    for line in range(code_editor.lines()):
        has_anno = line in style_issue_lines.union(error_lines)
        # Return 1 if has annotation else 0
        assert code_editor.SendScintilla(
            code_editor.SCI_ANNOTATIONGETSTYLE, line) == has_anno

    # We don't have an easy way to test the annotation text as Scintilla
    # provide only the size of the annotation message and not the text.
    style_message = "expected 2 blank lines, found 0"
    assert code_editor.SendScintilla(code_editor.SCI_ANNOTATIONGETTEXT,
                                     1) == len(style_message)
    assert code_editor.SendScintilla(
        code_editor.SCI_ANNOTATIONGETTEXT, 2) == 0  # No issue

    error_message = "invalid syntax"
    assert code_editor.SendScintilla(
        code_editor.SCI_ANNOTATIONGETTEXT, 7) == len(error_message)

    #  Clear all indicators.
    code_book.clearIndicators()
    for line in range(code_editor.lines()):
        assert not (code_editor.SendScintilla(
            code_editor.SCI_ANNOTATIONGETSTYLE, line))


def test_flake_reporter():
    reporter = FlakeReporter()
    message = "test.py:1:1 'karabo.middlelayer.Int32' imported but unused"
    reporter.flake(message)
    expected = {
        1: [{'message': "'karabo.middlelayer.Int32' imported but unused"}]}
    assert reporter.log == expected

    # Invalid message - column number is missing.
    message = "test.py:1 'karabo.middlelayer.Int32' imported but unused"
    reporter.flake(message)
    assert reporter.log == expected

    # Another issue on the same line.
    message = "test.py:1:6 'foo' : imported : but unused"
    reporter.flake(message)
    assert reporter.log[1][1].get('message') == (
        "'foo' : imported : but unused")
    assert not reporter.log[1][1].get('is_error')

    # A Syntax error
    message = "unexpected EOD while parsing"
    reporter.syntaxError(
        filename="", message=message, line_no=18, column=8, source="")
    assert reporter.log[18][0].get('message') == message
    assert reporter.log[18][0].get('is_error')


def test_check_style():
    feedback = check_style(REAL_CODE)
    assert feedback == {
        2: [{'message': 'expected 2 blank lines, found 0', 'code': 'E302'}]}


def test_result_count(gui_app, mocker):
    """
    Search result text should be updated on code changes only if Find toolbar
    is visible.
    """

    code_book = CodeBook(code=MULTILINE_CODE)
    code_editor = code_book.code_editor
    find_toolbar = code_book.find_toolbar
    find_toolbar.find_line_edit.setText("is")
    is_visible = mocker.patch.object(find_toolbar, "isVisible")

    # Find toolbar is hidden.
    is_visible.return_value = False
    new_code = MULTILINE_CODE.replace("This", "")
    code_editor.setText(new_code)
    assert find_toolbar.result_label.text() == "2 Results"

    # Find toolbar is shown.
    is_visible.return_value = True
    code_editor.setText(new_code)
    assert find_toolbar.result_label.text() == "1 Result"
