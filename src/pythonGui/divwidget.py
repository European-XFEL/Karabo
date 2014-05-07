#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the un/docked widget."""

__all__ = ["DivWidget"]

from toolbar import ToolBar

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class DivWidget(QFrame):
    # define signals...
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

        self.docked.connect(self.dockableWidget.onDock)
        self.undocked.connect(self.dockableWidget.onUndock)

#        self.setStyleSheet("QWidget {border-style: solid;"
#                                    "border: 1px solid gray;"
#                                    "}")


    def forceClose(self):
        """
        This function sets the member variable to False which decides whether
        this widget should be closed on dockonevent and calls closeEvent.
        """
        self.doesDockOnClose = False
        return self.close()


### virtual functions ###
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

