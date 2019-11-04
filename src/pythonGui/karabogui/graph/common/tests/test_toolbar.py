from PyQt5.QtCore import pyqtSlot

from karabogui.testing import GuiTestCase

from karabogui.graph.common.enums import MouseMode
from karabogui.graph.common.toolbar import KaraboToolBar, MouseModeToolset


class TestCase(GuiTestCase):

    def setUp(self):
        super(TestCase, self).setUp()
        self.toolbar = KaraboToolBar()

    def test_basics(self):
        # Check if toolset contains only MouseMode
        self.assertEqual(len(self.toolbar.toolset), 1)
        self.assertTrue(MouseMode in self.toolbar.toolset)

        # Check if default mouse mode toolset contains three buttons
        mouse_mode = self.toolbar.toolset[MouseMode]
        self.assertTrue(isinstance(mouse_mode, MouseModeToolset))
        self.assertEqual(len(mouse_mode.buttons), 3)
        self.assertEqual(set(mouse_mode.buttons.keys()),
                         {MouseMode.Pointer, MouseMode.Zoom, MouseMode.Move})

        # Check mouse mode buttons default checked state
        self.assertTrue(mouse_mode.buttons[MouseMode.Pointer].isChecked())
        self.assertFalse(mouse_mode.buttons[MouseMode.Zoom].isChecked())
        self.assertFalse(mouse_mode.buttons[MouseMode.Move].isChecked())

        self.assertTrue(self.toolbar.buttons[MouseMode.Pointer].isChecked())
        self.assertFalse(self.toolbar.buttons[MouseMode.Zoom].isChecked())
        self.assertFalse(self.toolbar.buttons[MouseMode.Move].isChecked())

    def test_mouse_mode_button_clicks(self):
        # Set up test
        tool = None

        @pyqtSlot(object)
        def set_tool(value):
            nonlocal tool
            tool = value

        mouse_mode = self.toolbar.toolset[MouseMode]
        mouse_mode.clicked.connect(set_tool)

        # Click pointer for the first time
        mouse_mode.buttons[MouseMode.Pointer].click()
        self.assertEqual(tool, MouseMode.Pointer)
        self.assertTrue(mouse_mode.buttons[MouseMode.Pointer].isChecked())

        # Click pointer again, see if it's still checked
        mouse_mode.buttons[MouseMode.Pointer].click()
        self.assertTrue(tool is MouseMode.Pointer)
        self.assertTrue(mouse_mode.buttons[MouseMode.Pointer].isChecked())

        # Click zoom for the first time, see if it's checked and
        # the pointer is unchecked
        mouse_mode.buttons[MouseMode.Zoom].click()
        self.assertTrue(tool is MouseMode.Zoom)
        self.assertTrue(mouse_mode.buttons[MouseMode.Zoom].isChecked())
        self.assertFalse(mouse_mode.buttons[MouseMode.Pointer].isChecked())

        # Click zoom for again, see if it's unchecked and
        # the pointer is checked as default
        mouse_mode.buttons[MouseMode.Zoom].click()
        self.assertEqual(tool, MouseMode.Pointer)
        self.assertFalse(mouse_mode.buttons[MouseMode.Zoom].isChecked())
        self.assertTrue(mouse_mode.buttons[MouseMode.Pointer].isChecked())

    def test_add_picker_tool(self):
        mouse_mode = self.toolbar.add_tool(MouseMode.Picker)
        self.assertTrue(mouse_mode is not None)
        self.assertTrue(isinstance(mouse_mode, MouseModeToolset))
        self.assertEqual(len(mouse_mode.buttons), 4)
        self.assertTrue(MouseMode.Picker in mouse_mode.buttons)
