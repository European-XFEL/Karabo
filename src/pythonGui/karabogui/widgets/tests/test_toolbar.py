from unittest import mock

from qtpy.QtCore import QSize

from karabogui.testing import GuiTestCase
from karabogui.widgets.toolbar import ToolBar


class TestPopUp(GuiTestCase):

    def test_default(self):
        widget = ToolBar()
        self.assertEqual(widget.windowTitle(), "")
        self.assertEqual(widget.iconSize(), QSize(19, 19))
        self.assertEqual(widget.isMovable(), False)
        self.assertEqual(widget.isFloatable(), True)

        # title test
        widget = ToolBar("new toolbar")
        self.assertEqual(widget.windowTitle(), "new toolbar")

        with mock.patch.object(widget, 'addWidget') as mock_method:
            # add_expander will add a widget to the toolbar. This way
            # we can test if that method was called.
            mock_method.assert_not_called()
            widget.add_expander()
            mock_method.assert_called_once()