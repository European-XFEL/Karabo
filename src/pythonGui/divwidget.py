#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the un/docked widget."""

__all__ = ["DivWidget"]


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

        self.dockableWidget  = dockableWidget
        self.index           = -1
        self.label           = label
        self.doesDockOnClose = True

        self.icon = icon

        self.setupActions()
        self.setupToolBar()

        # Add custom actions to toolbar
        self.dockableWidget.setupToolBar(self.toolBar)
        self.docked.connect(dockableWidget.onDock)
        self.undocked.connect(dockableWidget.onUndock)
        
        self.vLayout = QVBoxLayout(self)
        self.vLayout.setContentsMargins(0,0,0,0)
        self.vLayout.addWidget(self.toolBar)
        self.vLayout.addWidget(self.dockableWidget)

#        self.setStyleSheet("QWidget {border-style: solid;"
#                                    "border: 1px solid gray;"
#                                    "}")


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
        self.toolBar = QToolBar("Standard")
        self.toolBar.setStyleSheet("QToolBar {"
                                   "background-color: rgb(180,180,180);"
                                   "margin-bottom: 0px;"
                                   "}")
        self.toolBar.setIconSize(QSize(32,32))

        self.toolBar.setObjectName("DivWidgetToolBar")
        self.toolBar.addAction(self.acUndock)
        self.toolBar.addAction(self.acDock)

        iconSize = self.toolBar.iconSize()
        iconSize *= 0.6
        self.toolBar.setIconSize(iconSize)


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
        self.index = index


    def closeEvent(self, event):
        if self.doesDockOnClose:
            self.onDock()
            event.ignore()
        else:
            event.accept()


    def getIndex(self):
        return self.index


    def setIndex(self, index):
        self.index = index


    def getLabel(self):
        return self.label


    def setLabel(self, label):
        self.label = label


    def hasIcon(self):
        if self.icon is None :
            return False
        else :
            return True

