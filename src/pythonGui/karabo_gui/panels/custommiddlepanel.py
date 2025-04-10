#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the custom view panel in the middle
   of the MainWindow which is un/dockable.
"""

__all__ = ["CustomMiddlePanel"]


from karabo_gui.docktabwindow import Dockable
import karabo_gui.icons as icons
from karabo_gui.toolbar import ToolBar

from PyQt4.QtCore import pyqtSignal, Qt
from PyQt4.QtGui import (QAction, QApplication, QPalette, QSizePolicy,
                         QScrollArea, QWidget)


class CustomMiddlePanel(Dockable, QScrollArea):
    signalClosed = pyqtSignal()


    def __init__(self, scene, isConnectedToServer):
        super(CustomMiddlePanel, self).__init__()

        # Reference to underlying scene object
        self.scene = scene
        self.setWidget(self.scene)
        
        self.setupActions(isConnectedToServer)
        self.setBackgroundRole(QPalette.Dark)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        self.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)


    def designModeText(self, isDesignMode):
        if isDesignMode:
            return "Change to control mode"

        return "Change to design mode"

### virtual functions ###
    def closeEvent(self, event):
        if self.scene.aboutToClose():
            event.accept()
        else:
            event.ignore()
        # Send signal to mainwindow
        self.signalClosed.emit()


    def setupActions(self, isConnectedToServer):
        text = self.designModeText(self.scene.designMode)
        self.__acDesignMode = QAction(icons.transform, text, self)
        self.__acDesignMode.setToolTip(text)
        self.__acDesignMode.setStatusTip(text)
        self.__acDesignMode.setCheckable(True)
        self.__acDesignMode.setChecked(self.scene.designMode)
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


    def onDesignModeChanged(self, isChecked):
        self.drawingToolBar.setVisible(isChecked)
        text = self.designModeText(isChecked)
        self.__acDesignMode.setToolTip(text)
        self.__acDesignMode.setStatusTip(text)
        self.scene.designMode = isChecked

    def notifyTabVisible(self, visible):
        self.scene.setTabVisible(visible)

    def onUndock(self):
        self.scene.setTabVisible(True)
        osize = self.scene.size()
        screen_rect = QApplication.desktop().screenGeometry()
        if osize.width() < screen_rect.width() and osize.height() < screen_rect.height():
            # Enlarge the scene widget to its actual size
            self.setWidgetResizable(True)
            # Resize parent
            self.parent().resize(osize - self.scene.size() + self.parent().size())

    def onDock(self):
        self.setWidgetResizable(False)
