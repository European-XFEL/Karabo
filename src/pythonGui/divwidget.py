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

        self.__index = -1
        self.__label = label
        self.__doesDockOnClose = True

        self.__icon = icon

        self.setupActions()
        self.setupToolBar()

        self.toolBarLayout = QHBoxLayout()
        self.toolBarLayout.setContentsMargins(0, 0, 0, 0)
        self.toolBarLayout.setSpacing(0)
        self.addToolBar(self.toolBar)
        
        vLayout = QVBoxLayout(self)
        vLayout.setContentsMargins(0,0,0,0)
        vLayout.addLayout(self.toolBarLayout)
        vLayout.addWidget(dockableWidget)

        # Add custom actions to toolbar
        dockableWidget.setupToolBars(self.toolBar, self)

        self.docked.connect(dockableWidget.onDock)
        self.undocked.connect(dockableWidget.onUndock)

#        self.setStyleSheet("QWidget {border-style: solid;"
#                                    "border: 1px solid gray;"
#                                    "}")


    def setupActions(self):
        text = "Unpin as individual window"
        self.__acUndock = QAction(QIcon(":undock"), "&Undock", self)
        self.__acUndock.setToolTip(text)
        self.__acUndock.setStatusTip(text)
        self.__acUndock.triggered.connect(self.onUndock)

        text = "Pin this window to main program"
        self.__acDock = QAction(QIcon(":dock"), "&Dock", self)
        self.__acDock.setToolTip(text)
        self.__acDock.setStatusTip(text)
        self.__acDock.triggered.connect(self.onDock)
        self.__acDock.setVisible(False)


    def setupToolBar(self):
        self.toolBar = ToolBar("Standard")
        self.toolBar.addAction(self.__acUndock)
        self.toolBar.addAction(self.__acDock)


    def addToolBar(self, toolBar):
        self.toolBarLayout.addWidget(toolBar)


    def onUndock(self):
        self.__acDock.setVisible(True)
        self.__acUndock.setVisible(False)
        if self.__icon is not None:
            self.setWindowIcon(self.__icon)
        self.setWindowTitle(self.__label)
        self.undocked.emit()


    def onDock(self):
        self.__acDock.setVisible(False)
        self.__acUndock.setVisible(True)
        self.docked.emit()


    def onIndexChanged(self, index):
        self.__index = index


    def closeEvent(self, event):
        if self.__doesDockOnClose:
            self.onDock()
            event.ignore()
        else:
            event.accept()


    def getIndex(self):
        return self.__index


    def setIndex(self, index):
        self.__index = index


    def getLabel(self):
        return self.__label


    def setLabel(self, label):
        self.__label = label


    def hasIcon(self):
        if self.__icon:
            return True
        return False

