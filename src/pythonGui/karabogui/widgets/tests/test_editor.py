from unittest import mock

from karabogui.testing import GuiTestCase, click_button
from karabogui.widgets.codeeditor import (
    DEFAULT_BACKGROUND_COLOR, TEXT_HIGHLIGHT_COLOR, CodeBook, CodeEditor)


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
            widget.appendPlainText("{}".format(text))
        # We have double digits now
        self.assertGreaterEqual(widget.numberWidgetArea(), 15)
        for text in range(90):
            widget.appendPlainText("{}".format(text))
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

    def test_code_book(self):
        code_book = CodeBook(parent=None, code="Hello karabo")

        # set code to the editor
        assert code_book.code_editor.toPlainText() == "Hello karabo"

        # get code from editor
        assert code_book.getEditorCode() == "Hello karabo"

    def test_move_cursor(self):
        code = """
        This is a dummy code
        containing multiple lines
        text 1
        text 2
        text 3
        """
        code_book = CodeBook(parent=None, code=code)
        assert code_book.code_editor.textCursor().position() == 0

        code_book.moveCursorToLine(3, 0)
        assert code_book.code_editor.firstVisibleBlock().blockNumber() == 2
        assert code_book.code_editor.textCursor().columnNumber() == 0

        code_book.moveCursorToLine(4, 2)
        assert code_book.code_editor.firstVisibleBlock().blockNumber() == 3
        assert code_book.code_editor.textCursor().columnNumber() == 2
