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

    def test_margins(self):
        widget = CodeEditor(None)
        widget.appendPlainText("Karabo")
        self.assertEqual(widget.numberWidgetArea(), 11)
        for text in range(10):
            widget.appendPlainText("{}".format(text))
        # We have double digits now
        self.assertEqual(widget.numberWidgetArea(), 19)
        for text in range(90):
            widget.appendPlainText("{}".format(text))
        # We have three digits now
        self.assertEqual(widget.numberWidgetArea(), 27)
