import os

from karabogui.programs.base import create_gui_app
from karabogui.testing import GuiTestCase
from karabogui.widgets.codeeditor import CodeEditor


class TestConst(GuiTestCase):

    def setUp(self):
        os.environ["KARABO_TEST_GUI"] = "1"
        self.app = create_gui_app([])
        super(TestConst, self).setUp()

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
