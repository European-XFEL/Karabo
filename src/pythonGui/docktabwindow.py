#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 4, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a tab widget for un/dockable
   widgets.
"""

__all__ = ["DockTabWindow"]


from toolbar import ToolBar

from PyQt4.QtCore import pyqtSignal
from PyQt4.QtGui import (QAction, QCursor, QFrame, QHBoxLayout, QIcon,
                         QTabWidget, QVBoxLayout)

class DockTabWindow(QTabWidget):

    def __init__(self, title, parent):
        super(DockTabWindow, self).__init__()
        
        self.setParent(parent)
        self.setWindowTitle(title)
        self.divWidgetList = []

#        self.setStyleSheet("QTabWidget {border-style: solid;"
#                                       "border: 1px solid gray;"
#                                       "border-radius: 3px;"
#                                       "}")


    def addDockableTab(self, dockWidget, label, icon=None):
        """
        This function gets a DockTabWindow, a label and optionally an icon.
        """
        divWidget = DivWidget(dockWidget, label, icon)

        divWidget.docked.connect(dockWidget.onDock)
        divWidget.docked.connect(self.onDock)
        divWidget.undocked.connect(self.onUndock)
        divWidget.undocked.connect(dockWidget.onUndock)

        index = self.addTab(divWidget, label)
        divWidget.index = index

        # Store divWidget in list to keep it alive for un/dock event!!!
        self.divWidgetList.append(divWidget)


    def removeDockableTab(self, widget):
        divWidget = widget.parent()
        self.removeTab(self.indexOf(divWidget))
        self.updateTabsClosable()


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
        if (currentDivWidget is not None) and (currentDivWidget.parent() is not None):
            self.removeTab(currentDivWidget.index)
            currentDivWidget.setParent(None)
            currentDivWidget.move(QCursor.pos())
            currentDivWidget.show()

            if self.count() == 0:
                self.hide()


    def onDock(self):
        divWidget = self.sender()
        if (divWidget is not None) and (divWidget.parent() is None):
            if divWidget.hasIcon() == True:
                index = self.insertTab(divWidget.index, divWidget, divWidget.icon, divWidget.label)
            else:
                index = self.insertTab(divWidget.index, divWidget, divWidget.label)

            for i in range(self.count()):
                if self.widget(i) is not None:
                    self.widget(i).index = i

            self.setCurrentIndex(index)
            self.show()


class DivWidget(QFrame):
    docked = pyqtSignal()
    undocked = pyqtSignal()

    def __init__(self, dockableWidget, label, icon=None):
        super(DivWidget, self).__init__()

        self.setFrameStyle(QFrame.Box | QFrame.Plain)
        self.setLineWidth(1)

        self.index = -1
        self.label = label
        self.doesDockOnClose = True
        self.dockableWidget = dockableWidget

        self.icon = icon

        self.setupActions()
        self.setupToolBar()

        self.toolBarLayout = QHBoxLayout()
        self.toolBarLayout.setContentsMargins(0, 0, 0, 0)
        self.toolBarLayout.setSpacing(0)
        self.addToolBar(self.toolBar)

        vLayout = QVBoxLayout(self)
        vLayout.setContentsMargins(0,0,0,0)
        vLayout.addLayout(self.toolBarLayout)
        vLayout.addWidget(self.dockableWidget)

        # Add custom actions to toolbar
        self.dockableWidget.setupToolBars(self.toolBar, self)


    def forceClose(self):
        """
        This function sets the member variable to False which decides whether
        this widget should be closed on dockonevent and calls closeEvent.
        """
        self.doesDockOnClose = False
        return self.close()


    def closeEvent(self, event):
        if self.doesDockOnClose:
            self.onDock()
            event.ignore()
        else:
            if self.dockableWidget.close():
                event.accept()
            else:
                event.ignore()


    def setupActions(self):
        text = "Unpin as individual window"
        self.acUndock = QAction(QIcon(":undock"), "&Undock", self)
        self.acUndock.setToolTip(text)
        self.acUndock.setStatusTip(text)
        self.acUndock.triggered.connect(self.onUndock)

        text = "Pin this window to main program"
        self.acDock = QAction(QIcon(":dock"), "&Dock", self)
        self.acDock.setToolTip(text)
        self.acDock.setStatusTip(text)
        self.acDock.triggered.connect(self.onDock)
        self.acDock.setVisible(False)


    def setupToolBar(self):
        self.toolBar = ToolBar("Standard")
        self.toolBar.addAction(self.acUndock)
        self.toolBar.addAction(self.acDock)


    def addToolBar(self, toolBar):
        self.toolBarLayout.addWidget(toolBar)


    def onUndock(self):
        self.acDock.setVisible(True)
        self.acUndock.setVisible(False)
        if self.icon is not None:
            self.setWindowIcon(self.icon)
        self.setWindowTitle(self.label)
        self.undocked.emit()


    def onDock(self):
        self.acDock.setVisible(False)
        self.acUndock.setVisible(True)
        self.docked.emit()


    def onIndexChanged(self, index):
        #TODO: change index, if tab is removed
        print "onIndexChanged", index
        self.index = index


    def hasIcon(self):
        if self.icon is None:
            return False

        return True
