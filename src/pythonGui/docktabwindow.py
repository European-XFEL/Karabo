#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 4, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a tab widget for un/dockable
   widgets.
"""

__all__ = ["DockTabWindow", "Dockable"]


import icons
from toolbar import ToolBar

from PyQt4.QtCore import pyqtSignal, pyqtSlot
from PyQt4.QtGui import (QAction, QCursor, QFrame, QHBoxLayout,
                         QTabWidget, QVBoxLayout)


class Dockable:
    def onDock(self):
        pass

    def onUndock(self):
        pass

    def setupToolBars(self, toolbar, widget):
        pass

    def notifyTabVisible(self, visible):
        pass


class DockTabWindow(QTabWidget):

    def __init__(self, title, parent):
        super(DockTabWindow, self).__init__(parent)
        self.setWindowTitle(title)
        self.divWidgetList = []
        self.lastWidget = None
        self.currentChanged.connect(self.onCurrentChanged)

#        self.setStyleSheet("QTabWidget {border-style: solid;"
#                                       "border: 1px solid gray;"
#                                       "border-radius: 3px;"
#                                       "}")


    def addDockableTab(self, dockWidget, label, icon=None):
        """
        This function gets a DockTabWindow, a label and optionally an icon.
        """
        divWidget = DivWidget(self, dockWidget, label, icon)

        index = self.addTab(divWidget, label)
        divWidget.index = index

        # Store divWidget in list to keep it alive for un/dock event!!!
        self.divWidgetList.append(divWidget)


    def removeDockableTab(self, widget):
        divWidget = widget.parent()
        self.removeTab(self.indexOf(divWidget))
        del divWidget
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


    def onUndock(self, div):
        if div.parent() is not None:
            self.removeTab(div.index)
            div.setParent(None)
            div.move(QCursor.pos())
            div.show()

            if self.count() == 0:
                self.hide()

    def onDock(self, div):
        if div.parent() is None:
            if div.hasIcon():
                index = self.insertTab(div.index, div, div.icon, div.label)
            else:
                index = self.insertTab(div.index, div, div.label)

            for i in range(self.count()):
                if self.widget(i) is not None:
                    self.widget(i).index = i

            self.setCurrentIndex(index)
            self.show()

    @pyqtSlot(int)
    def onCurrentChanged(self, index):
        cw = self.currentWidget()
        if cw is self.lastWidget:
            return

        if self.lastWidget is not None:
            self.lastWidget.dockableWidget.notifyTabVisible(False)
        if cw is not None:
            cw.dockableWidget.notifyTabVisible(True)
        self.lastWidget = cw


class DivWidget(QFrame):
    docked = pyqtSignal()
    undocked = pyqtSignal()

    def __init__(self, dockWindow, dockableWidget, label, icon=None):
        super(DivWidget, self).__init__()

        self.setFrameStyle(QFrame.Box | QFrame.Plain)
        self.setLineWidth(1)

        self.index = -1
        self.label = label
        self.doesDockOnClose = True
        self.dockableWidget = dockableWidget
        self.dockWindow = dockWindow

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
        self.acUndock = QAction(icons.undock, "&Undock", self)
        self.acUndock.setToolTip(text)
        self.acUndock.setStatusTip(text)
        self.acUndock.triggered.connect(self.onUndock)

        text = "Pin this window to main program"
        self.acDock = QAction(icons.dock, "&Dock", self)
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
        self.dockWindow.onUndock(self)
        self.dockableWidget.onUndock()

    def onDock(self):
        self.acDock.setVisible(False)
        self.acUndock.setVisible(True)
        self.dockWindow.onDock(self)
        self.dockableWidget.onDock()

    def hasIcon(self):
        return self.icon is not None
