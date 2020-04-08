from functools import partial

from traits.api import (
    Any, Callable, Constant, Dict, Enum, HasStrictTraits, List)

from .factory import export_factory, mouse_mode_factory, roi_factory
from ..enums import ExportTool, MouseMode, ROITool


class BaseToolsetController(HasStrictTraits):
    default_tool = Any
    current_tool = Any

    tools = List
    factory = Callable

    buttons = Dict

    def _buttons_default(self):
        buttons = {}
        for tool in self.tools:
            button = self.factory(tool)
            if button is not None:
                button.clicked.connect(partial(self.select, tool=tool))
                buttons[tool] = button

        # If there's a default button, enable it
        default_button = buttons.get(self.current_tool)
        if default_button:
            default_button.setChecked(True)

        return buttons

    def select(self, tool):
        self.current_tool = tool

    def add(self, tool):
        button = self.factory(tool)
        if button is not None:
            self.buttons[tool] = button
        return button

    def check(self, tool):
        if tool in self.buttons:
            self.buttons[tool].setChecked(True)

    def uncheck_all(self):
        for button in self.buttons.values():
            button.setChecked(False)
            # Uncheck default action if any:
            action = button.defaultAction()
            if action:
                action.setChecked(False)


class MouseModeToolset(BaseToolsetController):
    tools = List([MouseMode.Pointer, MouseMode.Zoom, MouseMode.Move])
    factory = Callable(mouse_mode_factory)

    current_tool = Enum(*MouseMode)
    default_tool = Constant(MouseMode.Pointer)

    def select(self, tool):
        """Sets the selected mouse mode and uncheck any previously tool"""

        # Uncheck current tool
        self.buttons[self.current_tool].setChecked(False)

        # If current tool is unchecked, select default tool
        if self.current_tool is tool and self.default_tool in self.buttons:
            tool = self.default_tool
            self.buttons[tool].setChecked(True)

        super(MouseModeToolset, self).select(tool)


class ROIToolset(BaseToolsetController):
    tools = List([ROITool.Rect, ROITool.Crosshair])
    factory = Callable(roi_factory)

    current_tool = Enum(*ROITool)

    def select(self, tool):
        """The toolset has can have one or more buttons, with check states
           being exclusive. When a button is unchecked, the toolset returns
           ROI.NoROI. When selected, on the other hand, the other button
           (if existing), should be unchecked."""

        cross_button = self.buttons[ROITool.Crosshair]
        rect_button = self.buttons[ROITool.Rect]

        # Uncheck the other button
        if not cross_button.isChecked() and not rect_button.isChecked():
            tool = ROITool.NoROI
        elif tool in [ROITool.Rect, ROITool.DrawRect]:
            if rect_button.isChecked():
                cross_button.setChecked(False)
        elif tool in [ROITool.Crosshair, ROITool.DrawCrosshair]:
            if cross_button.isChecked():
                rect_button.setChecked(False)

        # Get the proper tool from the actions
        if tool in self.buttons:
            tool = self.buttons[tool].defaultAction().data()
        super(ROIToolset, self).select(tool)

    def check(self, tool):
        """Reimplementing since we want to change the default action"""

        # Uncheck all if no ROI is selected
        if tool is ROITool.NoROI:
            self.uncheck_all()

        if tool in [ROITool.Rect, ROITool.DrawRect]:
            button = self.buttons[ROITool.Rect]
        elif tool in [ROITool.Crosshair, ROITool.DrawCrosshair]:
            button = self.buttons[ROITool.Crosshair]
        else:
            # Do nothing if no button is associated with the tool
            return

        button.setChecked(True)
        for action in button.menu().actions():
            if action.data() is tool:
                button.setDefaultAction(action)
            action.setChecked(action.data() is tool)


class ExportToolset(BaseToolsetController):

    tools = List([ExportTool.Image, ExportTool.Data])
    factory = Callable(export_factory)


TOOLSET_MAP = {
    ExportTool: ExportToolset,
    MouseMode: MouseModeToolset,
    ROITool: ROIToolset}


def get_toolset(toolset, tools=None):
    klass = TOOLSET_MAP.get(toolset)
    if klass is not None:
        if tools is not None:
            return klass(tools=tools)
        else:
            return klass()


def register_toolset(toolset, klass):
    if toolset not in TOOLSET_MAP:
        TOOLSET_MAP[toolset] = klass
