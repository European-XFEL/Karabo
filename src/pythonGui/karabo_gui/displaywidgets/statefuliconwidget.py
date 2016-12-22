#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on December 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QByteArray, Qt
from PyQt4.QtGui import (QLabel, QPixmap, QDialog, QListView,
                         QStandardItemModel, QStandardItem, QIcon)
from PyQt4.QtSvg import QSvgWidget

from karabo.common.states import State
from karabo.middlelayer import String
from karabo_gui.const import OK_COLOR, ERROR_COLOR_ALPHA
from karabo_gui.util import generateObjectName
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
        objectName = generateObjectName(self)
        self.widget.setObjectName(objectName)
        self.setErrorState(False)

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
        Use the SVG code
        :param svg:
        :return:
        """
        self.widget.load(QByteArray(svg))

    def setErrorState(self, isError):
        if isError:
            self._setSVG(self.icon.with_color(ERROR_COLOR_ALPHA))
        else:
            self._setSVG(self.icon.with_color(OK_COLOR))

    def valueChanged(self, box, value, timestamp=None):
        if State(value).isDerivedFrom(State.CHANGING):
            self._setSVG(self.icon.with_color(STATE_COLORS[State.CHANGING]))
        elif State(value).isDerivedFrom(State.ACTIVE):
            self._setSVG(self.icon.with_color(STATE_COLORS[State.ACTIVE]))
        elif State(value).isDerivedFrom(State.PASSIVE):
            self._setSVG(self.icon.with_color(STATE_COLORS[State.PASSIVE]))
        elif State(value) is State.ERROR:
            self._setSVG(self.icon.with_color(STATE_COLORS[State.ERROR]))
        elif State(value) is State.INIT:
            self._setSVG(self.icon.with_color(STATE_COLORS[State.INIT]))
        elif State(value) is State.DISABLED:
            self._setSVG(self.icon.with_color(STATE_COLORS[State.DISABLED]))
        elif State(value) is State.NORMAL:
            self._setSVG(self.icon.with_color(OK_COLOR))
        else:
            self._setSVG(self.icon.with_color(STATE_COLORS[State.UNKNOWN]))