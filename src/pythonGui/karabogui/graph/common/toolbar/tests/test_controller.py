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

from ..controller import ToolbarController
from ..toolsets import MouseModeToolset


class TestToolbarController(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.toolbar = ToolbarController()

    def test_basics(self):
        # Check if toolset contains only MouseMode
        assert len(self.toolbar.toolsets) == 1
        assert MouseTool in self.toolbar.toolsets

        # Check if default mouse mode toolset contains three buttons
        controller = self.toolbar.toolsets[MouseTool]
        assert isinstance(controller, MouseModeToolset)
        buttons = controller.buttons
        assert len(buttons) == 3
        assert set(buttons.keys()) == {MouseTool.Pointer, MouseTool.Zoom,
                                       MouseTool.Move}

        # Check if toolbar contains the buttons
        actions = self.toolbar.widget.actions()
        toolbar_buttons = [action._toolbar_button for action in actions]
        assert len(toolbar_buttons) == 3
        assert set(toolbar_buttons) == set(controller.buttons.values())

        # Check mouse mode buttons default checked state
        assert buttons[MouseTool.Pointer].isChecked()
        assert not buttons[MouseTool.Zoom].isChecked()
        assert not buttons[MouseTool.Move].isChecked()

    def test_add_toolset(self):
        widget = self.toolbar.widget

        # Add existing toolset
        self.toolbar.add_toolset(MouseTool)
        assert len(self.toolbar.toolsets) == 1
        # Count the number of buttons in the toolbar
        mouse_mode = self.toolbar.toolsets[MouseTool]
        assert len(widget.actions()) == len(mouse_mode.buttons)
        assert len(self.toolbar.buttons) == 3
        assert "Pointer" in self.toolbar.buttons
        assert "Zoom" in self.toolbar.buttons
        assert "Move" in self.toolbar.buttons

        # Add new toolset
        self.toolbar.add_toolset(ROITool)
        assert len(self.toolbar.toolsets) == 2
        roi = self.toolbar.toolsets[ROITool]
        # Count the number of buttons in the toolbar, including one separator
        num_actions = len(mouse_mode.buttons) + len(roi.buttons) + 1
        assert len(widget.actions()) == num_actions
        assert len(self.toolbar.buttons) == 5
        assert "Crosshair" in self.toolbar.buttons
        assert "Rect" in self.toolbar.buttons

        # Add invalid toolset
        self.toolbar.add_toolset("foo")
        # Check if there is no changes.
        assert len(self.toolbar.toolsets) == 2
        assert len(widget.actions()) == num_actions
        assert len(self.toolbar.buttons) == 5

    def test_add_optional_tool(self):
        # Add optional tool to toolbar with MouseMode toolset
        self.toolbar.add_tool(MouseTool.Picker)
        assert len(self.toolbar.toolsets) == 1
        # Count the number of buttons in the toolbar
        controller = self.toolbar.toolsets[MouseTool]
        assert isinstance(controller, MouseModeToolset)
        buttons = controller.buttons
        assert len(buttons) == 4
        assert set(buttons.keys()) == {MouseTool.Pointer, MouseTool.Zoom,
                                       MouseTool.Move, MouseTool.Picker}
        assert "Picker" in self.toolbar.buttons
        assert "Pointer" in self.toolbar.buttons
        assert "Zoom" in self.toolbar.buttons
        assert "Move" in self.toolbar.buttons
