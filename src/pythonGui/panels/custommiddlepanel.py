#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the custom view panel in the middle
   of the MainWindow which is un/dockable.
"""

__all__ = ["CustomMiddlePanel"]


import icons
from network import Network
from toolbar import ToolBar

from PyQt4.QtCore import pyqtSignal, Qt
from PyQt4.QtGui import (QAction, QPalette, QSizePolicy, QScrollArea, QWidget)


class CustomMiddlePanel(QScrollArea):
    ##########################################
    # Dockable widget class used in DivWidget
    # Requires following interface:
    #
    #def setupActions(self):
    #    pass
    #def setupToolBars(self, standardToolBar, parent):
    #    pass
    #def onUndock(self):
    #    pass
    #def onDock(self):
    #    pass
    ##########################################
    signalClosed = pyqtSignal()


    def __init__(self, scene, isConnectedToServer):
        super(CustomMiddlePanel, self).__init__()

        # Reference to underlying scene object
        self.scene = scene
        self.scene.setParent(self)
        self.scene.designMode = isConnectedToServer
        self.setWidget(self.scene)

        #Manager().signalReset.connect(self.scene.reset)
        Network().signalServerConnectionChanged.connect(self.onServerConnectionChanged)
        
        self.setupActions(isConnectedToServer)
        self.setBackgroundRole(QPalette.Dark)


### virtual functions ###
    def closeEvent(self, event):
        if self.scene.close():
            event.accept()
        else:
            event.ignore()
        # Send signal to mainwindow
        self.signalClosed.emit()


    def setupActions(self, isConnectedToServer):
        text = "Change to control mode"
        self.__acDesignMode = QAction(icons.transform, text, self)
        self.__acDesignMode.setToolTip(text)
        self.__acDesignMode.setStatusTip(text)
        self.__acDesignMode.setCheckable(True)
        self.__acDesignMode.setChecked(isConnectedToServer)
        self.__acDesignMode.setEnabled(isConnectedToServer)
        self.__acDesignMode.toggled.connect(self.onDesignModeChanged)
       
        self.graphicsactions = list(self.scene.add_actions(self))
        

    def setupToolBars(self, standardToolBar, parent):
        standardToolBar.addAction(self.__acDesignMode)

        toolBar = ToolBar('Drawing')
        toolBar.setVisible(self.scene.designMode)
        parent.addToolBar(toolBar)
        self.scene.setFocusProxy(toolBar)
        toolBar.setFocusPolicy(Qt.StrongFocus)
        
        toolBar.addSeparator()
        #toolBar.addWidget(self.__tbAddShape)
        for a in self.graphicsactions:
            toolBar.addAction(a)
        self.drawingToolBar = toolBar

        # Add placeholder widget to toolbar
        widget = QWidget()
        widget.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        self.drawingToolBar.addWidget(widget)


    def onServerConnectionChanged(self, isConnected):
        """
        This slot is called when the server connection has changed (connect/disconnect).
        In this case the design mode functionality of the customMiddlePanel changes.
        """
        self.__acDesignMode.setChecked(isConnected)
        self.__acDesignMode.setEnabled(isConnected)


    def onDesignModeChanged(self, isChecked):
        self.drawingToolBar.setVisible(isChecked)
        if isChecked:
            text = "Change to control mode"
        else:
            text = "Change to design mode"
        self.__acDesignMode.setToolTip(text)
        self.__acDesignMode.setStatusTip(text)
        self.scene.designMode = isChecked


    def onUndock(self):
        osize = self.scene.size()
        self.setWidgetResizable(True)
        self.parent().resize(osize - self.scene.size() + self.parent().size())
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)


    def onDock(self):
        self.setWidgetResizable(False)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        self.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)
