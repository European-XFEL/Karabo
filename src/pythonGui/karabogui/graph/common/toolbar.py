from abc import abstractmethod
from collections import OrderedDict
from functools import partial

from PyQt4.QtCore import Qt, QObject, pyqtSignal, pyqtSlot, QSize
from PyQt4.QtGui import QAction, QMenu, QToolBar, QToolButton, QWidgetAction

from karabogui import icons

from .enums import MouseMode, ExportTool, ROITool
from .const import ICON_SIZE, BUTTON_SIZE


class BaseToolsetController(QObject):
    clicked = pyqtSignal(object)
    tool_type = None

    def __init__(self, tools=None):
        super(BaseToolsetController, self).__init__()
        self.buttons = OrderedDict()

        self.current_tool = None

        if tools is None:
            tools = self._default_buttons()
        for button in tools:
            self.add_button(button)

        # Save the clicked tool
        self.clicked.connect(self._save_clicked_tool)

    @pyqtSlot(object)
    def _save_clicked_tool(self, tool):
        self.current_tool = tool

    @pyqtSlot(object)
    def _select(self, button):
        """Specifies the behavior of the controller when a button is selected
        """
        self.clicked.emit(button)

    @abstractmethod
    def _button_factory(self, button):
        """Contains the buttons to be added in the the controller"""

    @abstractmethod
    def _default_buttons(self):
        """Subclass to return a list of default tools, e.g.:
            return [MouseMode.Pointer]"""
        return []

    def add_button(self, key):
        button = self._button_factory(key)
        if button is not None:
            self.buttons[key] = button
        return button

    def check_button(self, button):
        if button in self.buttons:
            self.buttons[button].setChecked(True)

    def uncheck_all(self):
        for button in self.buttons.values():
            button.setChecked(False)
            # Uncheck default action if any:
            action = button.defaultAction()
            if action:
                action.setChecked(False)


class MouseModeToolset(BaseToolsetController):
    tool_type = MouseMode
    clicked = pyqtSignal(object)

    def __init__(self, tools):
        super(MouseModeToolset, self).__init__(tools)
        self._select(MouseMode.Pointer)

    @pyqtSlot(MouseMode)
    def _select(self, mouse_mode):
        """Sets the selected mouse mode and uncheck any previously tool"""
        if (self.current_tool is None
                or self.current_tool is mouse_mode):
            # set default mouse mouse

            if self.current_tool is mouse_mode:
                # uncheck current button
                self.buttons[mouse_mode].setChecked(False)

            mouse_mode = MouseMode.Pointer
            self.buttons[mouse_mode].setChecked(True)
        else:
            # Uncheck previous button
            self.buttons[self.current_tool].setChecked(False)

        super(MouseModeToolset, self)._select(mouse_mode)

    def _button_factory(self, mouse_mode):
        """Add optional buttons"""
        button = None
        if mouse_mode is MouseMode.Pointer:
            button = create_tool_button(
                icon=icons.pointer,
                checkable=True,
                tooltip="Pointer Mode",
                on_clicked=partial(self._select, MouseMode.Pointer)
            )
        elif mouse_mode is MouseMode.Zoom:
            button = create_tool_button(
                icon=icons.zoomImage,
                checkable=True,
                tooltip="Zoom Mode",
                on_clicked=partial(self._select, MouseMode.Zoom)
            )
        elif mouse_mode is MouseMode.Move:
            button = create_tool_button(
                icon=icons.move,
                checkable=True,
                tooltip="Move Mode",
                on_clicked=partial(self._select, MouseMode.Move)
            )
        elif mouse_mode is MouseMode.Picker:
            self.buttons[MouseMode.Picker] = button = create_tool_button(
                icon=icons.picker,
                checkable=True,
                tooltip="Picker Mode",
                on_clicked=partial(self._select, MouseMode.Picker))

        return button

    def _default_buttons(self):
        return [MouseMode.Pointer, MouseMode.Zoom, MouseMode.Move]


class ExportToolset(BaseToolsetController):
    tool_type = ExportTool

    def _button_factory(self, tool):
        button = None
        if tool is ExportTool.Image:
            button = create_tool_button(
                icon=icons.camera,
                checkable=False,
                tooltip="Save Snapshot",
                on_clicked=partial(self._select, ExportTool.Image),
            )
        elif tool is ExportTool.Data:
            button = create_tool_button(
                icon=icons.export,
                checkable=False,
                tooltip="Export Data",
                on_clicked=partial(self._select, ExportTool.Data)
            )
        return button

    def _default_buttons(self):
        return [ExportTool.Image, ExportTool.Data]


class KaraboToolBar(QToolBar):
    def __init__(self, orientation=Qt.Vertical, parent=None):
        super(KaraboToolBar, self).__init__(parent)
        # Setup toolbar settings
        self.setOrientation(orientation)
        self.setStyleSheet("QToolBar { border: 0px }")
        self.toolset = {}
        self.buttons = {}
        self._separators = {}
        # Setup default toolset
        self.add_toolset(MouseModeToolset)

    def add_toolset(self, klass, tools=None):
        """Add toolset"""
        if klass.tool_type in self.toolset:
            return
        controller = klass(tools)
        for name, button in controller.buttons.items():
            self._add_button(button)
            self.buttons[name] = button

        self.toolset[klass.tool_type] = controller
        # Add and bookkeep separators for inserting buttons
        self._separators[klass.tool_type] = self.addSeparator()

        return controller

    def add_tool(self, tool):
        """For tools with existing toolset"""

        # Check if tool has existing toolset controller
        toolset = tool.__class__
        if toolset not in self.toolset:
            return

        # Add tool
        controller = self.toolset[tool.__class__]
        button = controller.add_button(tool)

        # Add button to toolbar
        action = WidgetAction(button, parent=self)
        self.insertAction(self._separators[toolset], action)
        return controller

    def add_button(self, name, button):
        """Add button to the toolbar"""
        self._add_button(button)
        self.buttons[name] = button

    def __getitem__(self, item):
        return self.buttons[item]

    def _add_button(self, button):
        action = WidgetAction(button, parent=self)
        self.addAction(action)


def create_tool_button(*, checkable, icon, tooltip, on_clicked=None):
    button = QToolButton()
    button.setCheckable(checkable)
    button.setIcon(icon)
    button.setAutoRaise(True)
    button.setToolTip(tooltip)
    button.setIconSize(QSize(ICON_SIZE, ICON_SIZE))
    button.setFixedSize(BUTTON_SIZE, BUTTON_SIZE)
    if on_clicked is not None:
        button.clicked.connect(on_clicked)

    return button


def create_action(text, icon, data=None, on_clicked=None, checkable=False):
    action = QAction(text, None)
    action.setIcon(icon)
    action.setCheckable(checkable)
    if data is not None:
        action.setData(data)
    if on_clicked is not None:
        action.triggered.connect(on_clicked)

    return action


def create_dropdown_button(actions, checkable, icon, tooltip,
                           on_clicked=None):
    def check_button(checked):
        current = button.defaultAction()
        for action in actions:
            if action is not current:
                action.setChecked(False)
        current.setChecked(checked)

    def set_default_action(action, activate=True):
        """Callback when select"""
        # Reset action check state, will be rechecked by button click.
        if checkable:
            action.setChecked(False)

        button.setDefaultAction(action)

        # Click immediately upon selected
        if activate:
            button.click()

    # create menu
    menu = DropDownMenu()
    for action in actions:
        # Action should be checkable to accept button check state
        action.setParent(menu)
        menu.addAction(action)

    button = create_tool_button(checkable=checkable,
                                icon=icon,
                                tooltip=tooltip,
                                on_clicked=on_clicked)
    button.setMenu(menu)
    button.setPopupMode(QToolButton.MenuButtonPopup)
    set_default_action(actions[0], activate=False)
    if checkable:
        button.toggled.connect(check_button)

    # connect events
    menu.triggered.connect(set_default_action)
    return button


class DropDownMenu(QMenu):
    triggered = pyqtSignal(QAction)

    def mouseReleaseEvent(self, event):
        super(DropDownMenu, self).mouseReleaseEvent(event)
        if event.button() == Qt.LeftButton:
            action = self.actionAt(event.pos())
            if action:
                self.triggered.emit(action)


class WidgetAction(QWidgetAction):
    def __init__(self, button, parent=None):
        super(WidgetAction, self).__init__(parent)
        button.setParent(parent)
        self._toolbar_button = button
        self._menu_button = None
        self._actions = {}

    def createWidget(self, parent):
        button = self._toolbar_button
        if parent is self.parent():
            # The parent is the toolbar. Return the original button, which
            # is to be added on the toolbar itself.
            return button

        # The parent is the extension menu. We create a tool button
        # that emulates the toolbar button

        # Distinguish what to create (normal button vs. dropdown button)
        menu = button.menu()
        if menu:
            # Copy all the actions from the dropdown menu and bookkeep them
            actions = []
            for action in menu.actions():
                new_action = create_action(text=action.text(),
                                           icon=action.icon(),
                                           data=action.data(),
                                           checkable=action.isCheckable())
                if action.isCheckable():
                    new_action.setChecked(action.isChecked())

                actions.append(new_action)
                self._actions[action] = new_action

            # Create a dropdown button using the information from the
            # toolbar button
            new = create_dropdown_button(
                actions=actions,
                icon=button.icon(),
                tooltip=button.toolTip(),
                checkable=button.isCheckable(),
                on_clicked=self._on_menu_button_clicked)

            # Set the corresponding copied default action
            new.setDefaultAction(self._actions[button.defaultAction()])

            # Intercept the menu signal, we update the copied button according
            # to the logic of the original button
            parent.aboutToShow.connect(self._update_menu_button)
        else:
            # Create a simple button. Not as fancy as the other one.
            new = create_tool_button(
                icon=button.icon(),
                checkable=button.isCheckable(),
                tooltip=button.toolTip(),
                on_clicked=self._on_menu_button_clicked)

        # Proper parenting is required.
        new.setParent(parent)

        # Set checked state
        if button.isCheckable():
            new.setChecked(button.isChecked())

        # We hide the menu every when button is clicked.
        new.clicked.connect(parent.hide)

        self._menu_button = new
        return new

    def deleteWidget(self, widget):
        # Clean the menu signal
        if widget.menu():
            widget.parent().aboutToShow.disconnect(self._update_menu_button)
            self._actions.clear()

        self._menu_button = None
        super(WidgetAction, self).deleteWidget(widget)

    @pyqtSlot()
    def _on_menu_button_clicked(self):
        # Change the default action according to the selected action
        for orig_action, new_action in self._actions.items():
            if new_action is self._menu_button.defaultAction():
                self._toolbar_button.setDefaultAction(orig_action)

        self._toolbar_button.click()

    def _update_menu_button(self):
        orig_action = self._toolbar_button.defaultAction()
        new_action = self._actions[orig_action]
        self._menu_button.setDefaultAction(new_action)
        new_action.setChecked(orig_action.isChecked())


class ROIToolset(BaseToolsetController):
    tool_type = ROITool

    @pyqtSlot(ROITool)
    def _select(self, tool):
        if set(self._default_buttons()) == set(self.buttons.keys()):
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

        super(ROIToolset, self)._select(tool)

    def _button_factory(self, tool):
        button = None
        if tool is ROITool.Rect:
            # Create Rect ROI dropdown button
            rect_actions = [create_action(text="Show Rect ROI",
                                          icon=icons.roi,
                                          data=ROITool.Rect,
                                          checkable=True),
                            create_action(text="Draw Rect ROI",
                                          icon=icons.drawROI,
                                          data=ROITool.DrawRect,
                                          checkable=True)]

            button = create_dropdown_button(
                actions=rect_actions,
                icon=icons.roi,
                tooltip="Rect ROI",
                checkable=True,
                on_clicked=partial(self._select_dropdown_button,
                                   ROITool.Rect)
            )
        elif tool is ROITool.Crosshair:
            # Create Crosshair ROI dropdown button
            crosshair_actions = [create_action(text="Show Crosshair ROI",
                                               icon=icons.crosshair,
                                               data=ROITool.Crosshair,
                                               checkable=True),
                                 create_action(text="Draw Crosshair ROI",
                                               icon=icons.drawCrosshair,
                                               data=ROITool.DrawCrosshair,
                                               checkable=True)]

            button = create_dropdown_button(
                actions=crosshair_actions,
                icon=icons.crosshair,
                tooltip="Rect ROI",
                checkable=True,
                on_clicked=partial(self._select_dropdown_button,
                                   ROITool.Crosshair)
            )
        return button

    def _select_dropdown_button(self, tool):
        """Get the ROI tool mode (show or draw) stored in the button
           default action"""
        mode = self.buttons[tool].defaultAction().data()
        self._select(mode)

    def check_button(self, tool):
        """Reimplementing since we want to change the default action"""

        # Uncheck all if no ROI is selected
        if tool is ROITool.NoROI:
            self.uncheck_all()

        # Do nothing if no button is associated with the tool
        if tool not in self.buttons:
            return

        button = self.buttons[tool]
        for action in button.menu().actions():
            if action.data() is tool:
                button.setDefaultAction(action)
            action.setChecked(action.data() is tool)

        super(ROIToolset, self).check_button(tool)

    def _default_buttons(self):
        return [ROITool.Rect, ROITool.Crosshair]
