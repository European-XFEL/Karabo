from platform import system
from unittest import skipIf

from karabogui.testing import GuiTestCase
from karabogui.widgets.codeeditor import CodeEditor


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

    @skipIf(system() == "Windows",
            reason="numberWidgetArea is 9 on windows")
    def test_margins(self):
        # The line numbers are derived from 8 per digit + 3.
        # Hence, 2 digits give 19 and 3 digits give 27
        widget = CodeEditor(None)
        widget.appendPlainText("Karabo")
        # NOTE: Depending on the machine (OS), we either match the value or
        # are constantly shifted
        self.assertGreaterEqual(widget.numberWidgetArea(), 11)
        for text in range(10):
            widget.appendPlainText("{}".format(text))
        # We have double digits now
        self.assertGreaterEqual(widget.numberWidgetArea(), 19)
        for text in range(90):
            widget.appendPlainText("{}".format(text))
        # We have three digits now
        self.assertGreaterEqual(widget.numberWidgetArea(), 27)
        self.assertEqual(widget.textCursor().block().lineCount(), 1)
        widget.destroy()
