from karabogui import icons

from .widgets import create_action, create_button, create_dropdown_button
from ..enums import ExportTool, MouseMode, ROITool


def tool_factory(tool):
    factory = FACTORY_MAP.get(tool.__class__)
    if factory is None:
        return

    return factory(tool)


def mouse_mode_factory(tool):
    """Add optional buttons"""
    button = None
    if tool is MouseMode.Pointer:
        button = create_button(icon=icons.pointer,
                               checkable=True,
                               tooltip="Pointer Mode")
    elif tool is MouseMode.Zoom:
        button = create_button(icon=icons.zoomImage,
                               checkable=True,
                               tooltip="Zoom Mode")
    elif tool is MouseMode.Move:
        button = create_button(icon=icons.move,
                               checkable=True,
                               tooltip="Move Mode")
    elif tool is MouseMode.Picker:
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
    MouseMode: mouse_mode_factory,
    ROITool: roi_factory
}