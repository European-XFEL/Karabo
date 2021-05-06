from karabogui.graph.common.enums import MouseMode, ROITool
from karabogui.testing import GuiTestCase

from ..toolsets import MouseModeToolset, ROIToolset


class TestMouseModeToolset(GuiTestCase):

    def test_basics(self):
        # Set up test. We create a mock slot to check the changes.
        tool = None

        def set_tool(value):
            nonlocal tool
            tool = value

        # Setup default toolset
        toolset = MouseModeToolset()
        toolset.on_trait_change(set_tool, "current_tool")

        # Check if mouse pointer is checked by default
        self.assertEqual(toolset.current_tool, MouseMode.Pointer)
        self.assertTrue(toolset.buttons[MouseMode.Pointer].isChecked())

        # Click pointer for the first time
        toolset.buttons[MouseMode.Pointer].click()
        self.assertIsNone(tool)  # no changes
        self.assertTrue(toolset.buttons[MouseMode.Pointer].isChecked())

        # Click zoom for the first time, see if it's checked and
        # the pointer is unchecked
        toolset.buttons[MouseMode.Zoom].click()
        self.assertTrue(tool is MouseMode.Zoom)
        self.assertTrue(toolset.buttons[MouseMode.Zoom].isChecked())
        self.assertFalse(toolset.buttons[MouseMode.Pointer].isChecked())

        # Click zoom for again, see if it's unchecked and
        # the pointer is checked as default
        toolset.buttons[MouseMode.Zoom].click()
        self.assertEqual(tool, MouseMode.Pointer)
        self.assertFalse(toolset.buttons[MouseMode.Zoom].isChecked())
        self.assertTrue(toolset.buttons[MouseMode.Pointer].isChecked())

    def test_init_tool(self):
        # Setup toolset with desired tools
        tools = [MouseMode.Picker, MouseMode.Zoom]
        toolset = MouseModeToolset(tools=tools)

        self.assertEqual(len(toolset.buttons), 2)
        self.assertEqual(set(toolset.buttons), set(tools))

        # Check toolset with invalid tool
        tools = [MouseMode.Picker, 'foo']
        toolset = MouseModeToolset(tools=tools)

        self.assertEqual(len(toolset.buttons), 1)
        self.assertIn(MouseMode.Picker, toolset.buttons)
        self.assertNotIn('foo', toolset.buttons)

    def test_add_tool(self):
        # Setup default toolset
        toolset = MouseModeToolset()

        # Add a valid tool
        tool = MouseMode.Picker
        toolset.add(tool)
        self.assertIn(tool, toolset.buttons)
        self.assertEqual(len(toolset.buttons), 4)

        # Add an invalid tool
        tool = "foo"
        toolset.add(tool)
        self.assertNotIn(tool, toolset.buttons)
        self.assertEqual(len(toolset.buttons), 4)  # no changes


class TestROIToolset(GuiTestCase):

    def test_basics(self):
        # Setup default toolset
        toolset = ROIToolset()

        # Check if all are unchecked
        for button in toolset.buttons.values():
            self.assertFalse(button.isChecked())
            actions = button.menu().actions()
            self.assertEqual(len(actions), 2)
            for action in actions:
                self.assertFalse(action.isChecked())

    def test_click(self):
        # Set up test. We create a mock slot to check the changes.
        tool = None

        def set_tool(value):
            nonlocal tool
            tool = value

        # Setup default toolset
        toolset = ROIToolset()
        toolset.on_trait_change(set_tool, "current_tool")

        button = toolset.buttons[ROITool.Crosshair]
        button.click()
        self.assertEqual(tool, ROITool.Crosshair)
        self.assertTrue(button.isChecked())

        # Mock switch actions
        actions = button.menu().actions()
        button.setDefaultAction(actions[1])
        button.click()
        self.assertEqual(tool, ROITool.DrawCrosshair)
        self.assertTrue(button.isChecked())

        # Check deselect
        button.click()
        self.assertEqual(tool, ROITool.NoROI)
        self.assertFalse(button.isChecked())

    def test_check(self):
        # Set up test. We create a mock slot to check the changes.
        tool = None

        def set_tool(value):
            nonlocal tool
            tool = value

        # Setup default toolset
        toolset = ROIToolset()
        toolset.on_trait_change(set_tool, "current_tool")

        # Check the ROI.DrawRect action
        toolset.check(ROITool.DrawRect)
        button = toolset.buttons[ROITool.Rect]
        self.assertTrue(button.isChecked())
        self.assertEqual(button.defaultAction().data(), ROITool.DrawRect)
        self.assertIsNone(tool)  # no event triggers
