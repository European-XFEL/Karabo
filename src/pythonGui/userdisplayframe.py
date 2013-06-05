#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a frame for userdisplaywidgets
   of the custom area. This class is implemented due to the needed context menu.
"""

__all__ = ["UserDisplayFrame"]


from PyQt4.QtCore import *
from PyQt4.QtGui import *


class UserDisplayFrame(QFrame):


    def __init__(self, widgetAliases, parent=None):
        super(UserDisplayFrame, self).__init__(parent)

        self.__layout = QHBoxLayout(self)
        
        self._setupContextMenu(widgetAliases)


    def insertWidget(self, widget, index=0):
        self.__layout.insertWidget(index, widget)


    def _setupContextMenu(self, widgetAliases):
        self.setContextMenuPolicy(Qt.DefaultContextMenu)#CustomContextMenu)
        
        # main menu
        self.__mMain = QMenu(self)
        
        text = "Remove widget"
        self.__acRemove = QAction(QIcon(":no"), text, self)
        self.__acRemove.setStatusTip(text)
        self.__acRemove.setToolTip(text)
        self.__acRemove.triggered.connect(self.onRemove)
        self.__mMain.addAction(self.__acRemove)
        
        # sub menu for widget type change
        self.__mChangeType = QMenu("Change widget type", self)
        
        for i in range(len(widgetAliases)):
            acChange = self.__mChangeType.addAction(widgetAliases[i])
            acChange.triggered.connect(self.onChangeWidgetType)

        self.__mMain.addMenu(self.__mChangeType)


    def contextMenuEvent(self, event):
        self.__mMain.move(event.globalPos())
        self.__mMain.show()
        QFrame.contextMenuEvent(self, event)


### slots ###
    def onRemove(self):
        while True:
            item = self.__layout.takeAt(0)
            if item is None:
                break
            item.widget().setVisible(False)


    def onChangeWidgetType(self):
        action = self.sender()
        print "change to:", action.text()

