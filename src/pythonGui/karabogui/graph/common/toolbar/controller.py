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
from qtpy.QtWidgets import QToolBar
from traits.api import Dict, HasStrictTraits, Instance

from ..enums import MouseTool
from .factory import tool_factory
from .toolsets import get_toolset, register_toolset
from .widgets import ToolBar


class ToolbarController(HasStrictTraits):
    widget = Instance(QToolBar)

    toolsets = Dict
    buttons = Dict

    def __init__(self, parent=None, **traits):
        super().__init__(**traits)
        self.widget = ToolBar(parent=parent)

        default_tool_type = MouseTool
        controller = get_toolset(default_tool_type)
        self.toolsets[default_tool_type] = controller

    def set_background(self, color):
        """Set the background stylesheet of the toolbar widget"""
        self.widget.setBackground(color)

    def _toolsets_items_changed(self, event):
        """Add the buttons"""

        # Add separator in between toolsets
        if len(self.toolsets) > 1:
            self.widget.addSeparator()

        for controller in event.added.values():
            for tool, button in controller.buttons.items():
                self.widget.add_button(button)
                self.buttons[tool.name] = button

    def add_tool(self, tool):
        """Adds individual tools

        :param tool: An enumerable defining the tool
        """
        toolset = tool.__class__
        if toolset in self.toolsets:
            # We add the tool in the existing toolset
            controller = self.toolsets[toolset]
            button = controller.add(tool)
        else:
            # We add the tool as a separate button.
            button = tool_factory(tool)

        self.buttons[tool.name] = button
        self.widget.add_button(button)

        return button

    def add_button(self, button, key=None, separator=False):
        """Adds a custom button outside of the button factories"""
        # Add separator if specified
        if separator:
            self.widget.addSeparator()
        # Add button to widget
        self.widget.add_button(button)
        # Record if key is specified
        if key is not None:
            self.buttons[key] = button

    def add_toolset(self, tool_type, tools=None):
        """Adds a pre-configured set of tools which has interaction with each
           other."""
        if tool_type in self.toolsets:
            return

        controller = get_toolset(tool_type, tools=tools)
        if controller is not None:
            self.toolsets[tool_type] = controller
        return controller

    def register_toolset(self, toolset, klass):
        register_toolset(toolset, klass)
