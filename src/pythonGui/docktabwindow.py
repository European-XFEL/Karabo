#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 4, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a tab widget for un/dockable
   widgets.
"""

__all__ = ["DockTabWindow"]


from divwidget import DivWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class DockTabWindow(QTabWidget):

    def __init__(self, title, parent):
        super(DockTabWindow, self).__init__()
        
        self.setParent(parent)
        self.setWindowTitle(title)
        self.__divWidgetList = []

#        self.setStyleSheet("QTabWidget {border-style: solid;"
#                                       "border: 1px solid gray;"
#                                       "border-radius: 3px;"
#                                       "}")


    def addDockableTab(self, dockWidget, label, icon=None):
        divWidget = DivWidget(dockWidget, label, icon)

        divWidget.docked.connect(self.onDock)
        divWidget.undocked.connect(self.onUndock)

        index = self.addTab(divWidget, label)
        self.setCurrentIndex(index)
        divWidget.setIndex(index)

        # store divWidget in list to keep it alive for un/dock event!!!
        self.__divWidgetList.append(divWidget)


    def addCornerWidget(self, tbNewTab):
        self.setCornerWidget(tbNewTab)


    def updateTabsClosable(self):
        if self.count() > 1 and not self.tabsClosable():
            self.setTabsClosable(True)
            self.tabCloseRequested.connect(self.onCloseTab)
        elif self.count() == 1:
            self.setTabsClosable(False)
            self.tabCloseRequested.disconnect(self.onCloseTab)


### slots ###
    def onCloseTab(self, index):
        if self.count() == 1:
            return

        # Get widget, which is about to be closed
        widget = self.widget(index)
        # Close widget (if possible) before removing it from tab
        if not widget.forceClose():
            return
        # Remove widget from tab
        self.removeTab(index)
        self.updateTabsClosable()


    def onUndock(self):
        currentDivWidget = self.currentWidget()
        if (currentDivWidget is not None) and (currentDivWidget.parent() is not None) :
            self.removeTab(currentDivWidget.getIndex())
            currentDivWidget.setParent(None)
            currentDivWidget.move(QCursor.pos())
            currentDivWidget.show()

            if self.count() == 0 :
                self.hide()


    def onDock(self):
        divWidget = self.sender()
        if (divWidget is not None) and (divWidget.parent() is None):
            index = divWidget.getIndex()
            if divWidget.hasIcon() == True:
                index = self.insertTab(index, divWidget, divWidget.getIcon(), divWidget.getLabel())
            else:
                index = self.insertTab(index, divWidget, divWidget.getLabel())

            for i in range(self.count()):
                if self.widget(i) is not None:
                    self.widget(i).setIndex(i)

            self.setCurrentIndex(index)
            self.show()

