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
from qtpy.QtCore import QSize, Qt, Signal, Slot
from qtpy.QtWidgets import QAction, QMenu, QToolBar, QToolButton, QWidgetAction

from ..const import BUTTON_SIZE, ICON_SIZE


class ToolBar(QToolBar):

    def __init__(self, orientation=Qt.Vertical, parent=None):
        super().__init__(parent)
        # Setup toolbar settings
        self.setOrientation(orientation)
        self.setStyleSheet("QToolBar { border: 0px }")
        self._separators = {}

    def add_button(self, button):
        """Wraps the button into an action to have seamless integration
           with the toolbar"""
        action = WidgetAction(button, parent=self)

        # Check separators
        self.addAction(action)

    def setBackground(self, color):
        sheet = "QToolBar {{ border: 0px; background-color: rgba{}; }}"
        self.setStyleSheet(sheet.format(color.getRgb()))


class WidgetAction(QWidgetAction):
    def __init__(self, button, parent=None):
        super().__init__(parent)
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
            new = create_button(
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
            parent = widget.parent()
            if isinstance(parent, QMenu):
                parent.aboutToShow.disconnect(self._update_menu_button)
            self._actions.clear()

        self._menu_button = None
        super().deleteWidget(widget)

    @Slot()
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


class DropDownMenu(QMenu):
    triggered = Signal(QAction)

    def mouseReleaseEvent(self, event):
        super().mouseReleaseEvent(event)
        if event.button() == Qt.LeftButton:
            action = self.actionAt(event.pos())
            if action:
                self.triggered.emit(action)


class DropDownButton(QToolButton):
    # A similar signal with `clicked` but contains the data from the actions.
    # Will return `None` if unchecked.
    triggered = Signal(object)

    def __init__(self, parent=None):
        super().__init__(parent=parent)

        # Setup custom dropdown menu
        menu = DropDownMenu()
        menu.triggered.connect(self._set_default_action)
        self.setMenu(menu)
        self.setPopupMode(QToolButton.MenuButtonPopup)

        # Connect signal
        self.clicked.connect(self._trigger_data)

    def set_actions(self, actions):
        menu = self.menu()
        for action in actions:
            # Action should be checkable to accept button check state
            action.setParent(menu)
            menu.addAction(action)
        self._set_default_action(actions[0], enabled=False)

    def check(self, data):
        for action in self.menu().actions():
            if data == action.data():
                self.setDefaultAction(action)
                self.setChecked(True)
                # Bail out if we have find the action
                return

        self.setChecked(False)

    def setCheckable(self, checkable):
        super().setCheckable(checkable)
        if checkable:
            self.toggled.connect(self._check_action)

    @Slot(bool)
    def _trigger_data(self, checked):
        data = None
        if checked:
            data = self.defaultAction().data()
        self.triggered.emit(data)

    @Slot(bool)
    def _check_action(self, checked):
        current = self.defaultAction()
        for action in self.menu().actions():
            if action is not current:
                action.setChecked(False)
        current.setChecked(checked)

    def _set_default_action(self, action, enabled=True):
        """Callback when select"""
        # Reset action check state, will be rechecked by button click.
        if self.isCheckable():
            action.setChecked(False)
        self.setDefaultAction(action)

        # Click immediately upon selected
        if enabled:
            self.click()


def create_button(*, checkable, icon, tooltip, on_clicked=None):
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


# XXX: Backward compatibility
create_tool_button = create_button


def create_dropdown_button(actions, checkable, icon, tooltip,
                           on_clicked=None):
    button = DropDownButton()
    button.setCheckable(checkable)
    button.setIcon(icon)
    button.setAutoRaise(True)
    button.setToolTip(tooltip)
    button.setIconSize(QSize(ICON_SIZE, ICON_SIZE))
    button.setFixedSize(BUTTON_SIZE, BUTTON_SIZE)
    if on_clicked is not None:
        button.clicked.connect(on_clicked)

    # Main difference from the normal tool button: actions as children
    button.set_actions(actions)

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
