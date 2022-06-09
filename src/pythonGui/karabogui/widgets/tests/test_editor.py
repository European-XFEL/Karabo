from unittest import mock

from karabogui.testing import GuiTestCase
from karabogui.widgets.codeeditor import (
    DEFAULT_BACKGROUND_COLOR, TEXT_HIGHLIGHT_COLOR, CodeEditor)


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
