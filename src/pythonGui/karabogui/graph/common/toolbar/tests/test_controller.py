from karabogui.graph.common.enums import MouseMode, ROITool
from karabogui.testing import GuiTestCase

from ..controller import ToolbarController
from ..toolsets import MouseModeToolset


class TestToolbarController(GuiTestCase):

    def setUp(self):
        super(TestToolbarController, self).setUp()
        self.toolbar = ToolbarController()

    def test_basics(self):
        # Check if toolset contains only MouseMode
        self.assertEqual(len(self.toolbar.toolsets), 1)
        self.assertTrue(MouseMode in self.toolbar.toolsets)

        # Check if default mouse mode toolset contains three buttons
        controller = self.toolbar.toolsets[MouseMode]
        self.assertTrue(isinstance(controller, MouseModeToolset))
        buttons = controller.buttons
        self.assertEqual(len(buttons), 3)
        self.assertEqual(set(buttons.keys()),
                         {MouseMode.Pointer, MouseMode.Zoom, MouseMode.Move})

        # Check if toolbar contains the buttons
        actions = self.toolbar.widget.actions()
        toolbar_buttons = [action._toolbar_button for action in actions]
        self.assertEqual(len(toolbar_buttons), 3)
        self.assertEqual(set(toolbar_buttons),
                         set(controller.buttons.values()))

        # Check mouse mode buttons default checked state
        self.assertTrue(buttons[MouseMode.Pointer].isChecked())
        self.assertFalse(buttons[MouseMode.Zoom].isChecked())
        self.assertFalse(buttons[MouseMode.Move].isChecked())

    def test_add_toolset(self):
        widget = self.toolbar.widget

        # Add existing toolset
        self.toolbar.add_toolset(MouseMode)
        self.assertEqual(len(self.toolbar.toolsets), 1)
        # Count the number of buttons in the toolbar
        mouse_mode = self.toolbar.toolsets[MouseMode]
        self.assertEqual(len(widget.actions()), len(mouse_mode.buttons))

        # Add new toolset
        self.toolbar.add_toolset(ROITool)
        self.assertEqual(len(self.toolbar.toolsets), 2)
        roi = self.toolbar.toolsets[ROITool]
        # Count the number of buttons in the toolbar, including one separator
        num_actions = len(mouse_mode.buttons) + len(roi.buttons) + 1
        self.assertEqual(len(widget.actions()), num_actions)

        # Add invalid toolset
        self.toolbar.add_toolset("foo")
        # Check if there is no changes.
        self.assertEqual(len(self.toolbar.toolsets), 2)
        self.assertEqual(len(widget.actions()), num_actions)

    def test_add_optional_tool(self):
        # Add optional tool to toolbar with MouseMode toolset
        self.toolbar.add_tool(MouseMode.Picker)
        self.assertEqual(len(self.toolbar.toolsets), 1)
        # Count the number of buttons in the toolbar
        controller = self.toolbar.toolsets[MouseMode]
        self.assertTrue(isinstance(controller, MouseModeToolset))
        buttons = controller.buttons
        self.assertEqual(len(buttons), 4)
        self.assertEqual(set(buttons.keys()),
                         {MouseMode.Pointer, MouseMode.Zoom, MouseMode.Move,
                          MouseMode.Picker})
