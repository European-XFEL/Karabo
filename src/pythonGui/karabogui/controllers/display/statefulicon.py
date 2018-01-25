#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on December 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot, QByteArray, Qt
from PyQt4.QtGui import (QPixmap, QDialog, QListView, QStandardItemModel,
                         QStandardItem, QIcon)
from PyQt4.QtSvg import QSvgWidget
from traits.api import Instance

from karabo.common.api import State
from karabo.common.scenemodel.api import StatefulIconWidgetModel
from karabogui.binding.api import StringBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller, with_display_type)
from karabogui.indicators import STATE_COLORS
from karabogui.icons.statefulicons import ICONS
from karabogui.icons.statefulicons.color_change_icon import ColorChangeIcon

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
        icon = ICONS.get(self.model.icon_name, None)
        if icon is None:
            self._show_icon_picker()
        else:
            self._icon = icon
        return QSvgWidget(parent)

    def value_update(self, proxy):
        if self._icon is None:
            return  # We haven't been configured correctly!

        value = State(proxy.value)
        if value.isDerivedFrom(State.CHANGING):
            color = STATE_COLORS[State.CHANGING]
        elif value.isDerivedFrom(State.RUNNING):
            color = STATE_COLORS[State.RUNNING]
        elif value.isDerivedFrom(State.ACTIVE):
            color = STATE_COLORS[State.ACTIVE]
        elif value.isDerivedFrom(State.PASSIVE):
            color = STATE_COLORS[State.PASSIVE]
        elif value.isDerivedFrom(State.DISABLED):
            color = STATE_COLORS[State.DISABLED]
        elif value is State.STATIC:
            color = STATE_COLORS[State.STATIC]
        elif value is State.NORMAL:
            color = STATE_COLORS[State.NORMAL]
        elif value is State.ERROR:
            color = STATE_COLORS[State.ERROR]
        elif value is State.INIT:
            color = STATE_COLORS[State.INIT]
        else:
            color = STATE_COLORS[State.UNKNOWN]

        svg = self._icon.with_color(color)
        self.widget.load(QByteArray(svg))

    def __icon_changed(self, icon):
        """Update the scene model when the icon changes"""
        self.model.icon_name = icon.name

    def _show_icon_picker(self):
        dialog = QDialog()
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
        for key, icon in ICONS.items():
            listItem = QStandardItem(icon.description)
            p = QPixmap()
            p.loadFromData(QByteArray(icon.with_color(WHITE)))
            listItem.setData(QIcon(p),
                             Qt.DecorationRole)
            listItem.setData(icon, Qt.UserRole + 1)
            model.appendRow(listItem)
        iconlist.setModel(model)

        # double clicking an entry will select it and close the dialog
        @pyqtSlot(int)
        def handleDoubleClick(index):
            self._icon = index.data(Qt.UserRole + 1)
            dialog.close()

        iconlist.doubleClicked.connect(handleDoubleClick)
        dialog.exec_()
        if self._icon is None:
            self._icon = non_selection
