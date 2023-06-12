# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from karabogui.graph.common.enums import MouseTool, ROITool
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
        self.assertEqual(toolset.current_tool, MouseTool.Pointer)
        self.assertTrue(toolset.buttons[MouseTool.Pointer].isChecked())

        # Click pointer for the first time
        toolset.buttons[MouseTool.Pointer].click()
        self.assertIsNone(tool)  # no changes
        self.assertTrue(toolset.buttons[MouseTool.Pointer].isChecked())

        # Click zoom for the first time, see if it's checked and
        # the pointer is unchecked
        toolset.buttons[MouseTool.Zoom].click()
        self.assertTrue(tool is MouseTool.Zoom)
        self.assertTrue(toolset.buttons[MouseTool.Zoom].isChecked())
        self.assertFalse(toolset.buttons[MouseTool.Pointer].isChecked())

        # Click zoom for again, see if it's unchecked and
        # the pointer is checked as default
        toolset.buttons[MouseTool.Zoom].click()
        self.assertEqual(tool, MouseTool.Pointer)
        self.assertFalse(toolset.buttons[MouseTool.Zoom].isChecked())
        self.assertTrue(toolset.buttons[MouseTool.Pointer].isChecked())

    def test_init_tool(self):
        # Setup toolset with desired tools
        tools = [MouseTool.Picker, MouseTool.Zoom]
        toolset = MouseModeToolset(tools=tools)

        self.assertEqual(len(toolset.buttons), 2)
        self.assertEqual(set(toolset.buttons), set(tools))

        # Check toolset with invalid tool
        tools = [MouseTool.Picker, 'foo']
        toolset = MouseModeToolset(tools=tools)

        self.assertEqual(len(toolset.buttons), 1)
        self.assertIn(MouseTool.Picker, toolset.buttons)
        self.assertNotIn('foo', toolset.buttons)

    def test_add_tool(self):
        # Setup default toolset
        toolset = MouseModeToolset()

        # Add a valid tool
        tool = MouseTool.Picker
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
