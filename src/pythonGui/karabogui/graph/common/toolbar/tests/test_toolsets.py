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
        assert toolset.current_tool == MouseTool.Pointer
        assert toolset.buttons[MouseTool.Pointer].isChecked()

        # Click pointer for the first time
        toolset.buttons[MouseTool.Pointer].click()
        assert tool is None  # no changes
        assert toolset.buttons[MouseTool.Pointer].isChecked()

        # Click zoom for the first time, see if it's checked and
        # the pointer is unchecked
        toolset.buttons[MouseTool.Zoom].click()
        assert tool is MouseTool.Zoom
        assert toolset.buttons[MouseTool.Zoom].isChecked()
        assert not toolset.buttons[MouseTool.Pointer].isChecked()

        # Click zoom for again, see if it's unchecked and
        # the pointer is checked as default
        toolset.buttons[MouseTool.Zoom].click()
        assert tool == MouseTool.Pointer
        assert not toolset.buttons[MouseTool.Zoom].isChecked()
        assert toolset.buttons[MouseTool.Pointer].isChecked()

    def test_init_tool(self):
        # Setup toolset with desired tools
        tools = [MouseTool.Picker, MouseTool.Zoom]
        toolset = MouseModeToolset(tools=tools)

        assert len(toolset.buttons) == 2
        assert set(toolset.buttons) == set(tools)

        # Check toolset with invalid tool
        tools = [MouseTool.Picker, 'foo']
        toolset = MouseModeToolset(tools=tools)

        assert len(toolset.buttons) == 1
        assert MouseTool.Picker in toolset.buttons
        assert 'foo' not in toolset.buttons

    def test_add_tool(self):
        # Setup default toolset
        toolset = MouseModeToolset()

        # Add a valid tool
        tool = MouseTool.Picker
        toolset.add(tool)
        assert tool in toolset.buttons
        assert len(toolset.buttons) == 4

        # Add an invalid tool
        tool = "foo"
        toolset.add(tool)
        assert tool not in toolset.buttons
        assert len(toolset.buttons) == 4  # no changes


class TestROIToolset(GuiTestCase):

    def test_basics(self):
        # Setup default toolset
        toolset = ROIToolset()

        # Check if all are unchecked
        for button in toolset.buttons.values():
            assert not button.isChecked()
            actions = button.menu().actions()
            assert len(actions) == 2
            for action in actions:
                assert not action.isChecked()

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
        assert tool == ROITool.Crosshair
        assert button.isChecked()

        # Mock switch actions
        actions = button.menu().actions()
        button.setDefaultAction(actions[1])
        button.click()
        assert tool == ROITool.DrawCrosshair
        assert button.isChecked()

        # Check deselect
        button.click()
        assert tool == ROITool.NoROI
        assert not button.isChecked()

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
        assert button.isChecked()
        assert button.defaultAction().data() == ROITool.DrawRect
        assert tool is None  # no event triggers
