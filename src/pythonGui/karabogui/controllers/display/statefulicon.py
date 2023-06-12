#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on December 7, 2016
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
#############################################################################
from qtpy.QtCore import Qt
from qtpy.QtGui import QIcon, QPixmap, QStandardItem, QStandardItemModel
from qtpy.QtSvg import QSvgWidget
from qtpy.QtWidgets import QDialog, QListView
from traits.api import Instance

from karabo.common.scenemodel.api import StatefulIconWidgetModel
from karabogui.binding.api import StringBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller, with_display_type)
from karabogui.icons.statefulicons import ICONS
from karabogui.icons.statefulicons.color_change_icon import ColorChangeIcon
from karabogui.indicators import get_state_color

WHITE = '#ffffff'


@register_binding_controller(ui_name='Standard Icon',
                             klassname='StatefulIconWidget',
                             binding_type=StringBinding,
                             is_compatible=with_display_type('State'),
                             can_show_nothing=False)
class StatefulIconWidget(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(StatefulIconWidgetModel, args=())
    # Internal traits
    _icon = Instance(ColorChangeIcon)

    def create_widget(self, parent):
        # show a list to pick the icon from if it is not set
        widget = QSvgWidget(parent)
        if not self.model.icon_name:
            # This is the creation phase, and the picker will set the icon
            # on the model
            self._show_icon_picker(widget)
        else:
            # Note: We might have an icon that is not supported. In this case
            # we do not modify the traits model!
            icon = ICONS.get(self.model.icon_name, ICONS["icon_default"])
            self._icon = icon

        return widget

    def value_update(self, proxy):
        if self._icon is None:
            return  # We haven't been configured correctly!
        value = proxy.value
        color = get_state_color(value)

        svg = self._icon.with_color(color)
        self.widget.load(bytearray(svg, encoding='UTF-8'))

    def _show_icon_picker(self, widget):
        dialog = QDialog(widget)
        dialog.setWindowTitle("Select standard icon")
        # we do not allow to cancel this dialog. An icon must be selected
        dialog.setWindowFlags(Qt.Window |
                              Qt.WindowTitleHint |
                              Qt.CustomizeWindowHint)

        iconlist = QListView(dialog)
        iconlist.setMinimumSize(400, 600)
        model = QStandardItemModel(iconlist)

        non_selection = ICONS["icon_default"]
        # we add all items with a preview icon
        for key, icon in sorted(ICONS.items()):
            listItem = QStandardItem(icon.description)
            p = QPixmap()
            p.loadFromData(bytearray(icon.with_color(WHITE), encoding='UTF-8'))
            listItem.setData(QIcon(p),
                             Qt.DecorationRole)
            listItem.setData(icon, Qt.UserRole + 1)
            model.appendRow(listItem)
        iconlist.setModel(model)

        # double clicking an entry will select it and close the dialog
        def handleDoubleClick(index):
            self._icon = index.data(Qt.UserRole + 1)
            self.model.icon_name = self._icon.name
            dialog.close()

        iconlist.doubleClicked.connect(handleDoubleClick)
        dialog.exec()
        if self._icon is None:
            # Operator bailed out of the dialog! Set a default icon!
            self._icon = non_selection
            self.model.icon_name = self._icon.name
