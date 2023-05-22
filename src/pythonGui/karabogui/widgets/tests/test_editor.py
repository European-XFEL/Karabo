# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest import mock

from qtpy.QtCore import Qt
from qtpy.QtTest import QTest

from karabogui.testing import GuiTestCase, click_button
from karabogui.widgets.codeeditor import (
    DEFAULT_BACKGROUND_COLOR, MAX_FONT_SIZE, MIN_FONT_SIZE,
    TEXT_HIGHLIGHT_COLOR, CodeBook, CodeEditor)

MULTILINE_CODE = """
This is a dummy code
containing multiple lines
Number 1
Number 2
Number 3
"""

INDENTED_CODE = """
This is a dummy code
containing multiple lines
    Number 1
    Number 2
Number 3
"""


class TestConst(GuiTestCase):

    def test_numbers(self):
        widget = CodeEditor(None)
        widget.appendPlainText("Karabo")
        self.assertEqual(widget.cache_blocks, 1)
        self.assertEqual(widget.cache_lines, 1)
        widget.appendPlainText("Karabo 2")
        self.assertEqual(widget.cache_blocks, 2)
        self.assertEqual(widget.cache_lines, 1)
        widget.appendPlainText("Karabo 3")
        self.assertEqual(widget.cache_blocks, 3)
        self.assertEqual(widget.cache_lines, 1)
        widget.destroy()

    def test_margins(self):
        # The line numbers are derived from 6 per digit + 3.
        # Hence, 2 digits give 15 and 3 digits give 21
        widget = CodeEditor(None)
        widget.appendPlainText("Karabo")
        self.assertGreaterEqual(widget.numberWidgetArea(), 9)
        for text in range(10):
            widget.appendPlainText(f"{text}")
        # We have double digits now
        self.assertGreaterEqual(widget.numberWidgetArea(), 15)
        for text in range(90):
            widget.appendPlainText(f"{text}")
        # We have three digits now
        self.assertGreaterEqual(widget.numberWidgetArea(), 21)
        self.assertEqual(widget.textCursor().block().lineCount(), 1)
        widget.destroy()

    def test_findAndHighlight(self):
        editor = CodeEditor(None)
        editor.setPlainText("one two three one one")
        assert editor.textCursor().position() == 0

        editor.findAndHighlight("one", False, False)
        assert editor.textCursor().position() == 3
        assert editor.textCursor().selectedText() == "one"
        editor.textCursor().movePosition(2)
        assert (editor.textCursor().charFormat().background().color()
                == TEXT_HIGHLIGHT_COLOR)

        editor.findAndHighlight("one", False, False)
        assert editor.textCursor().position() == 17

        editor.findAndHighlight("one", False, False)
        assert editor.textCursor().position() == 21

        # After hitting the bottom, cursor should go back to
        # the first hit.
        editor.findAndHighlight("one", False, False)
        assert editor.textCursor().position() == 3

        editor.findAndHighlight("two", False, False)
        assert editor.textCursor().position() == 7
        assert editor.textCursor().selectedText() == "two"

        # Case-sensitive search
        editor.findAndHighlight("Three", True, False)
        assert editor.textCursor().selectedText() == ""

        editor.findAndHighlight("three", True, False)
        assert editor.textCursor().selectedText() == "three"

        # Find should emit 'resultFound' signal with number of hits
        mock_slot = mock.Mock()
        editor.resultFound.connect(mock_slot)
        editor.findAndHighlight("one", False, False)
        count = mock_slot.call_args[0][0]
        assert count == 3

        mock_slot.reset_mock()
        editor.findAndHighlight("non_existing_text", False, False)
        count = mock_slot.call_args[0][0]
        assert count == 0

    def test_Highlight(self):
        editor = CodeEditor(None)
        editor.setPlainText("one two three one one")

        editor.findAndHighlight("one", False, False)
        assert (editor.textCursor().charFormat().background().color()
                == TEXT_HIGHLIGHT_COLOR)

        editor.clearHighlight()
        assert (editor.textCursor().charFormat().background().color()
                == DEFAULT_BACKGROUND_COLOR)

        with mock.patch.object(editor, "clearHighlight") as clear_mock:
            editor.findAndHighlight("foo", False, False)
            assert clear_mock.call_count == 1

    def test_case_sensitive_highlight(self):
        code = "one two three One one One"
        widget = CodeBook(code=code, parent=None)
        find_toolbar = widget.find_toolbar
        find_toolbar.match_case.setChecked(True)

        mock_slot = mock.Mock()
        find_toolbar.highlightRequested.connect(mock_slot)
        find_toolbar.find_line_edit.setText("one")

        mock_slot.reset_mock()
        click_button(find_toolbar.match_case)
        assert mock_slot.call_count == 1
        assert mock_slot.call_args[0] == ("one", False)

        mock_slot.reset_mock()
        click_button(find_toolbar.match_case)
        assert mock_slot.call_count == 1
        assert mock_slot.call_args[0] == ("one", True)

    def test_indent_and_deindent(self):
        widget = CodeBook(code=MULTILINE_CODE)
        editor = widget.code_editor
        cursor = editor.textCursor()

        # Indent: No line selected
        cursor.setPosition(1)
        editor.setTextCursor(cursor)

        QTest.keyClick(editor, Qt.Key_Tab)
        assert editor.textCursor().position() == 5

        # Deindent
        QTest.keyClick(editor, Qt.Key_Backtab)
        assert editor.textCursor().position() == 1

        # Hit Tab key twice
        QTest.keyClick(editor, Qt.Key_Tab)
        QTest.keyClick(editor, Qt.Key_Tab)
        assert editor.textCursor().position() == 9

        # Shift+Tab twice.
        QTest.keyClick(editor, Qt.Key_Backtab)
        QTest.keyClick(editor, Qt.Key_Backtab)
        assert editor.textCursor().position() == 1

        # Indent: on multiple lines selected.
        cursor.setPosition(65, cursor.MoveAnchor)
        cursor.setPosition(52, cursor.KeepAnchor)
        editor.setTextCursor(cursor)

        QTest.keyClick(editor, Qt.Key_Tab)

        assert editor.toPlainText() == INDENTED_CODE

        # Verify the selection
        assert editor.textCursor().selectionStart() == 56
        assert editor.textCursor().selectionEnd() == 73

        # Deindent
        QTest.keyClick(editor, Qt.Key_Backtab)
        assert editor.toPlainText() == MULTILINE_CODE
        assert editor.textCursor().hasSelection()

        # For single line, Tab key sets the indentation to next indentation
        # block (ie, multiples of 4 spaces)
        cursor = editor.textCursor()
        cursor.movePosition(cursor.Start)
        editor.setTextCursor(cursor)
        # Add two space.
        QTest.keyClick(editor, Qt.Key_Space)
        QTest.keyClick(editor, Qt.Key_Space)
        assert editor.textCursor().position() == 2
        QTest.keyClick(editor, Qt.Key_Tab)
        assert editor.textCursor().position() == 4
        QTest.keyClick(editor, Qt.Key_Tab)
        assert editor.textCursor().position() == 8

    def test_undo_block_indentation(self):
        """
        Test undoing (Ctrl+Z) the block-indentation changes the entire
        block to previous indentation.
        """
        widget = CodeBook(code=MULTILINE_CODE)
        editor = widget.code_editor
        cursor = editor.textCursor()

        cursor.setPosition(65, cursor.MoveAnchor)
        cursor.setPosition(52, cursor.KeepAnchor)
        editor.setTextCursor(cursor)

        QTest.keyClick(editor, Qt.Key_Tab)

        # Ctrl+Z to undo changes.
        QTest.keyClick(editor, Qt.Key_Z, Qt.ControlModifier)
        assert widget.getEditorCode() == MULTILINE_CODE

    def test_code_book(self):
        code_book = CodeBook(parent=None, code="Hello karabo")

        # set code to the editor
        assert code_book.code_editor.toPlainText() == "Hello karabo"

        # get code from editor
        assert code_book.getEditorCode() == "Hello karabo"

    def test_move_cursor(self):
        code_book = CodeBook(parent=None, code=MULTILINE_CODE)
        assert code_book.code_editor.textCursor().position() == 0

        code_book.moveCursorToLine(3, 0)
        assert code_book.code_editor.firstVisibleBlock().blockNumber() == 2
        assert code_book.code_editor.textCursor().columnNumber() == 0

        code_book.moveCursorToLine(4, 2)
        assert code_book.code_editor.firstVisibleBlock().blockNumber() == 3
        assert code_book.code_editor.textCursor().columnNumber() == 2

    def test_replace_all(self):
        code_book = CodeBook(parent=None, code=MULTILINE_CODE)
        initial_code = code_book.getEditorCode()
        count = initial_code.count('Number')
        expected_code = initial_code.replace('Number', 'row')

        code_book.code_editor.replace_all('number', 'row', match_case=False)

        code = code_book.getEditorCode()
        assert "Number" not in code
        assert code.count('row') == count
        assert code == expected_code

    def test_replace(self):
        code_book = CodeBook(parent=None, code=MULTILINE_CODE)
        editor = code_book.code_editor
        initial_code = code_book.getEditorCode()

        # Case-sensitive
        code_book.code_editor.replace('number', 'row', match_case=True)
        assert not editor.textCursor().selectedText()
        assert code_book.getEditorCode() == initial_code

        # Case-insensitive with no hit
        code_book.code_editor.replace('number', 'row', match_case=False)
        # If existing selection is not the text to be replaced, 'replace'
        # first just selects the next hit and doesn't replace.
        assert editor.textCursor().selectedText() == 'Number'

        code_book.code_editor.replace('number', 'row', match_case=False)
        code = code_book.getEditorCode()
        assert code.count('row') == 1
        assert code.split("\n")[3] == 'row 1'

        # Case-sensitive with hit
        code_book.code_editor.replace('Number', 'row', match_case=True)
        code = code_book.getEditorCode()
        assert code.count('row') == 2
        assert code.split("\n")[4] == 'row 2'

    def test_change_font_size(self):
        """ Test increase/decrease of editor font size"""
        code_book = CodeBook(parent=None, code=MULTILINE_CODE)
        editor = code_book.code_editor
        current_font_size = editor.font().pointSize()

        editor.increase_font_size()
        assert editor.font().pointSize() == current_font_size + 2

        editor.decrease_font_size()
        assert editor.font().pointSize() == current_font_size

        # No increase in font size after MAX_FONT_SIZE
        font = editor.font()
        font.setPointSize(MAX_FONT_SIZE)
        editor.setFont(font)
        for _ in range(4):
            editor.increase_font_size()
            assert editor.font().pointSize() == MAX_FONT_SIZE

        # No decrease in font size after MIN_FONT_SIZE
        font.setPointSize(MIN_FONT_SIZE)
        editor.setFont(font)
        for _ in range(4):
            editor.decrease_font_size()
            assert editor.font().pointSize() == MIN_FONT_SIZE
