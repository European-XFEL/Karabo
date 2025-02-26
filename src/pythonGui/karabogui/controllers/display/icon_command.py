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
from qtpy.QtCore import QModelIndex, QSize, Qt, Slot
from qtpy.QtGui import QIcon, QStandardItem, QStandardItemModel
from qtpy.QtWidgets import (
    QAction, QDialog, QHBoxLayout, QListView, QToolButton, QWidget)
from traits.api import Instance

import karabogui.icons as icons
from karabo.common.scenemodel.api import DisplayIconCommandModel
from karabogui.binding.api import SlotBinding
from karabogui.controllers.api import (
    BaseBindingController, is_proxy_allowed, register_binding_controller)

BUTTON_ICONS = {
    "Left": icons.arrowFancyLeft,
    "Right": icons.arrowFancyRight,
    "Up": icons.arrowFancyUp,
    "Down": icons.arrowFancyDown,
    "Start": icons.mediaStart,
    "Stop": icons.mediaStop,
    "Pause": icons.mediaPause,
    "Reset": icons.mediaBackward,
    "Acquire": icons.mediaRecord,
    "On": icons.on,
    "Off": icons.off,
    "Abort": icons.stop,
    "Halt": icons.halt
}

NO_SELECTION = icons.imageMissing
QPADDING = 2


class DynamicToolButton(QToolButton):

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setAutoRaise(True)
        self.setMinimumSize(20, 20)
        self.setEnabled(False)

    def resizeEvent(self, event):
        width, height = self.width(), self.height()
        self.setIconSize(QSize(width - QPADDING, height - QPADDING))
        super().resizeEvent(event)


class IconSelectionDialog(QDialog):

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setModal(False)
        self.setWindowTitle("Select icon")
        self.setWindowFlags(Qt.Window |
                            Qt.WindowTitleHint |
                            Qt.CustomizeWindowHint)

        # Variables
        self.icon = NO_SELECTION
        self.name = ""

        # Setup list view
        self.list_view = QListView(self)
        self.list_view.setMinimumSize(400, 600)
        model = QStandardItemModel(self.list_view)

        # we add all items with a preview icon
        for key, icon in sorted(BUTTON_ICONS.items()):
            listItem = QStandardItem(key)
            listItem.setData(icon, Qt.DecorationRole)
            listItem.setData(icon, Qt.UserRole + 1)
            model.appendRow(listItem)
        self.list_view.setModel(model)
        self.list_view.doubleClicked.connect(self.handleDoubleClick)

    @Slot(QModelIndex)
    def handleDoubleClick(self, index):
        icon = index.data(Qt.UserRole + 1)
        if icon is not None:
            self.icon = icon
            self.name = index.data(Qt.DisplayRole)
        self.close()


@register_binding_controller(ui_name="IconCommand",
                             klassname="DisplayIconCommand",
                             binding_type=SlotBinding,
                             priority=-10)
class DisplayIconCommand(BaseBindingController):
    # The scene model class for this controller
    model = Instance(DisplayIconCommandModel, args=())
    # Internal traits
    _button = Instance(QToolButton)
    _icon = Instance(QIcon)

    def create_widget(self, parent):
        # The ToolButton is affected with bounded actions, thus we need to
        # create another (parent) widget where we can bind the actions.
        widget = QWidget(parent)
        layout = QHBoxLayout(widget)

        self._button = DynamicToolButton(parent)
        self._button.clicked.connect(self.execute_action)
        layout.addWidget(self._button)

        change_icon = QAction("Change Icon", widget)
        change_icon.triggered.connect(self._change_icon)
        widget.addAction(change_icon)

        icon_name = self.model.icon_name
        self._icon = BUTTON_ICONS.get(icon_name, NO_SELECTION)
        self._button.setIcon(self._icon)

        return widget

    def execute_action(self):
        """Execute the action on the command proxy"""
        self.proxy.execute()

    def _change_icon(self):
        """Select the icon for the icon command widget"""
        dialog = IconSelectionDialog(parent=self.widget)
        dialog.exec()

        self._icon = dialog.icon
        self._button.setIcon(self._icon)
        self.model.icon_name = dialog.name

    def state_update(self, proxy):
        enable = is_proxy_allowed(proxy)
        self._button.setEnabled(enable)

    def setEnabled(self, enable):
        """Reimplemented to account for access level changes"""
        self._button.setEnabled(enable)
