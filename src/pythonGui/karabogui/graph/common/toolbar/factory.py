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
from karabogui import icons

from ..enums import ExportTool, MouseTool, ROITool
from .widgets import create_action, create_button, create_dropdown_button


def tool_factory(tool):
    factory = FACTORY_MAP.get(tool.__class__)
    if factory is None:
        return

    return factory(tool)


def mouse_mode_factory(tool):
    """Add optional buttons"""
    button = None
    if tool is MouseTool.Pointer:
        button = create_button(icon=icons.pointer,
                               checkable=True,
                               tooltip="Pointer Mode")
    elif tool is MouseTool.Zoom:
        button = create_button(icon=icons.zoomImage,
                               checkable=True,
                               tooltip="Zoom Mode")
    elif tool is MouseTool.Move:
        button = create_button(icon=icons.move,
                               checkable=True,
                               tooltip="Move Mode")
    elif tool is MouseTool.Picker:
        button = create_button(icon=icons.picker,
                               checkable=True,
                               tooltip="Picker Mode")
    return button


def roi_factory(tool):
    button = None
    if tool is ROITool.Rect:
        # Create Rect ROI dropdown button
        show_rect_action = create_action(text="Show Rect ROI",
                                         icon=icons.roi,
                                         data=ROITool.Rect,
                                         checkable=True)
        draw_rect_action = create_action(text="Draw Rect ROI",
                                         icon=icons.drawROI,
                                         data=ROITool.DrawRect,
                                         checkable=True)

        button = create_dropdown_button(
            actions=[show_rect_action, draw_rect_action],
            icon=icons.roi,
            tooltip="Rect ROI",
            checkable=True)
    elif tool is ROITool.Crosshair:
        # Create Crosshair ROI dropdown button
        show_crosshair_action = create_action(text="Show Crosshair ROI",
                                              icon=icons.crosshair,
                                              data=ROITool.Crosshair,
                                              checkable=True)
        draw_crosshair_action = create_action(text="Draw Crosshair ROI",
                                              icon=icons.drawCrosshair,
                                              data=ROITool.DrawCrosshair,
                                              checkable=True)
        button = create_dropdown_button(
            actions=[show_crosshair_action, draw_crosshair_action],
            icon=icons.crosshair,
            tooltip="Rect ROI",
            checkable=True)
    return button


def export_factory(tool):
    button = None
    if tool is ExportTool.Image:
        button = create_button(icon=icons.camera,
                               checkable=False,
                               tooltip="Save Snapshot")
    elif tool is ExportTool.Data:
        button = create_button(icon=icons.export,
                               checkable=False,
                               tooltip="Export Data")
    return button


FACTORY_MAP = {
    ExportTool: export_factory,
    MouseTool: mouse_mode_factory,
    ROITool: roi_factory
}
