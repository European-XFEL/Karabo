#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on December 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QByteArray, Qt
from PyQt4.QtGui import (QPixmap, QDialog, QListView, QStandardItemModel,
                         QStandardItem, QIcon)
from PyQt4.QtSvg import QSvgWidget

from karabo.middlelayer import State, String
from karabo_gui.widget import DisplayWidget
from karabo_gui.indicators import STATE_COLORS
from karabo_gui.icons.statefulicons import ICONS


class StatefulIconWidget(DisplayWidget):
    """
    The IconWidget handles SVG icons which are programmatically colored.
    """

    menu = "Change vacuum widget" # TODO: this should become its own menu
    category = String
    alias = "Standard Icon"
    icon = None
    value = None

    def __init__(self, box, icon, parent):
        #show a list to pick the icon from if it is not set
        super(StatefulIconWidget, self).__init__(box)
        if icon is None:
            self._showIconDialog()
        else:
            self._setIcon(icon)
        self.widget = QSvgWidget(parent)

    def _setIcon(self, icon):
        self.icon = icon

    def _showIconDialog(self):
        """
        The dialog used for selecting a standard icon

        """
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
            p.loadFromData(QByteArray(icon.with_color("#FFFFFF")))
            listItem.setData(QIcon(p),
                             Qt.DecorationRole)
            listItem.setData(icon, Qt.UserRole + 1)
            model.appendRow(listItem)
        iconlist.setModel(model)

        # double clicking an entry will select it and close the dialog
        def handleDoubleClick(index):
            self._setIcon(index.data(Qt.UserRole + 1))
            dialog.close()

        iconlist.doubleClicked.connect(handleDoubleClick)
        dialog.exec_()

    @classmethod
    def isCompatible(cls, box, readonly):
        super_comp = super(StatefulIconWidget, cls).isCompatible(box, readonly)
        return super_comp and box.path == ('state',)

    def _setSVG(self, svg):
        """
        :param svg: An XML for the icon
        """
        self.widget.load(QByteArray(svg))

    def valueChanged(self, box, value, timestamp=None):
        if State(value).isDerivedFrom(State.CHANGING):
            self._setSVG(self.icon.with_color(STATE_COLORS[State.CHANGING]))
        elif State(value).isDerivedFrom(State.RUNNING):
            self._setSVG(self.icon.with_color(STATE_COLORS[State.RUNNING]))
        elif State(value).isDerivedFrom(State.ACTIVE):
            self._setSVG(self.icon.with_color(STATE_COLORS[State.ACTIVE]))
        elif State(value).isDerivedFrom(State.PASSIVE):
            self._setSVG(self.icon.with_color(STATE_COLORS[State.PASSIVE]))
        elif State(value).isDerivedFrom(State.DISABLED):
            self._setSVG(self.icon.with_color(STATE_COLORS[State.DISABLED]))
        elif State(value) is State.STATIC:
            self._setSVG(self.icon.with_color(STATE_COLORS[State.STATIC]))
        elif State(value) is State.NORMAL:
            self._setSVG(self.icon.with_color(STATE_COLORS[State.NORMAL]))
        elif State(value) is State.ERROR:
            self._setSVG(self.icon.with_color(STATE_COLORS[State.ERROR]))
        elif State(value) is State.INIT:
            self._setSVG(self.icon.with_color(STATE_COLORS[State.INIT]))
        else:
            self._setSVG(self.icon.with_color(STATE_COLORS[State.UNKNOWN]))
