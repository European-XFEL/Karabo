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
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.registry import register_binding_controller
from karabogui.controllers.util import with_display_type
from karabogui.indicators import STATE_COLORS
from karabogui.icons.statefulicons import ICONS
from karabogui.icons.statefulicons.color_change_icon import ColorChangeIcon

WHITE = '#ffffff'


@register_binding_controller(ui_name='Standard Icon',
                             klassname='StatefulIconWidget',
                             binding_type=StringBinding,
                             is_compatible=with_display_type('State'))
class StatefulIconWidget(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(StatefulIconWidgetModel)
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
        value = proxy.value
        if State(value).isDerivedFrom(State.CHANGING):
            color = STATE_COLORS[State.CHANGING]
        elif State(value).isDerivedFrom(State.RUNNING):
            color = STATE_COLORS[State.RUNNING]
        elif State(value).isDerivedFrom(State.ACTIVE):
            color = STATE_COLORS[State.ACTIVE]
        elif State(value).isDerivedFrom(State.PASSIVE):
            color = STATE_COLORS[State.PASSIVE]
        elif State(value).isDerivedFrom(State.DISABLED):
            color = STATE_COLORS[State.DISABLED]
        elif State(value) is State.STATIC:
            color = STATE_COLORS[State.STATIC]
        elif State(value) is State.NORMAL:
            color = STATE_COLORS[State.NORMAL]
        elif State(value) is State.ERROR:
            color = STATE_COLORS[State.ERROR]
        elif State(value) is State.INIT:
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
