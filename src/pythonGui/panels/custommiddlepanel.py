#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the custom view panel in the middle
   of the MainWindow which is un/dockable.
"""

__all__ = ["CustomMiddlePanel"]


from network import Network
from toolbar import ToolBar

from PyQt4.QtCore import Qt
from PyQt4.QtGui import (QAction, QIcon, QKeySequence, QMenu, QSizePolicy,
                         QToolButton, QVBoxLayout, QWidget)


class CustomMiddlePanel(QWidget):
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


    def __init__(self, scene, isConnectedToServer):
        super(CustomMiddlePanel, self).__init__()

        # Reference to underlying scene object
        self.scene = scene

        self.graphicsview = scene.view
        self.graphicsview.setParent(self)
        self.graphicsview.designMode = isConnectedToServer
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(3,3,3,3)
        mainLayout.addWidget(self.graphicsview)
        
        #Manager().signalReset.connect(self.graphicsview.reset)
        Network().signalServerConnectionChanged.connect(self.onServerConnectionChanged)
        
        self.setupActions(isConnectedToServer)


### virtual functions ###
    def closeEvent(self, event):
        if self.graphicsview.close():
            event.accept()
        else:
            event.ignore()


    def setupActions(self, isConnectedToServer):
        text = "Change to control mode"
        self.__acDesignMode = QAction(QIcon(":transform"), text, self)
        self.__acDesignMode.setToolTip(text)
        self.__acDesignMode.setStatusTip(text)
        self.__acDesignMode.setCheckable(True)
        self.__acDesignMode.setChecked(isConnectedToServer)
        self.__acDesignMode.setEnabled(isConnectedToServer)
        self.__acDesignMode.toggled.connect(self.onDesignModeChanged)
       
        text = "Open"
        self.__tbOpen = QToolButton(self)
        self.__tbOpen.setIcon(QIcon(":open"))
        self.__tbOpen.setToolTip(text)
        self.__tbOpen.setStatusTip(text)
        self.__tbOpen.setPopupMode(QToolButton.InstantPopup)
        
        text = "Open layout"
        self.__acOpenLayout = QAction(QIcon(":open"), text, self)
        self.__acOpenLayout.setStatusTip(text)
        self.__acOpenLayout.setToolTip(text)
        self.__acOpenLayout.triggered.connect(
            self.graphicsview.openSceneLayoutFromFile)
        
        text = "Open configurations"
        self.__acOpenConfigurations = QAction(QIcon(":open"), text, self)
        self.__acOpenConfigurations.setStatusTip(text)
        self.__acOpenConfigurations.setToolTip(text)
        self.__acOpenConfigurations.setEnabled(False)
        self.__acOpenConfigurations.triggered.connect(
            self.graphicsview.openSceneConfigurationsFromFile)
        
        text = "Open layout and configurations"
        self.__acOpenLayoutConfigurations = QAction(QIcon(":open"), text, self)
        self.__acOpenLayoutConfigurations.setStatusTip(text)
        self.__acOpenLayoutConfigurations.setToolTip(text)
        self.__acOpenLayoutConfigurations.triggered.connect(
            self.graphicsview.openSceneLayoutConfigurationsFromFile)
        
        self.__mOpen = QMenu()
        self.__mOpen.addAction(self.__acOpenLayout)
        self.__mOpen.addAction(self.__acOpenConfigurations)
        self.__mOpen.addAction(self.__acOpenLayoutConfigurations)
        self.__tbOpen.setMenu(self.__mOpen)

        text = "Save as"
        self.__tbSaveAs = QToolButton(self)
        self.__tbSaveAs.setIcon(QIcon(":save-as"))
        self.__tbSaveAs.setToolTip(text)
        self.__tbSaveAs.setStatusTip(text)
        self.__tbSaveAs.setPopupMode(QToolButton.InstantPopup)
        
        text = "Save layout as"
        self.__acLayoutSaveAs = QAction(QIcon(":save-as"), text, self)
        self.__acLayoutSaveAs.setStatusTip(text)
        self.__acLayoutSaveAs.setToolTip(text)
        self.__acLayoutSaveAs.triggered.connect(
            self.graphicsview.saveSceneLayoutToFile)
        
        text = "Save configurations as"
        self.__acConfigurationsSaveAs = QAction(QIcon(":save-as"), text, self)
        self.__acConfigurationsSaveAs.setStatusTip(text)
        self.__acConfigurationsSaveAs.setToolTip(text)
        self.__acConfigurationsSaveAs.triggered.connect(
            self.graphicsview.saveSceneConfigurationsToFile)
        
        text = "Save layout and configurations as"
        self.__acLayoutConfigurationsSaveAs = QAction(QIcon(":save-as"), text, self)
        self.__acLayoutConfigurationsSaveAs.setStatusTip(text)
        self.__acLayoutConfigurationsSaveAs.setToolTip(text)
        self.__acLayoutConfigurationsSaveAs.triggered.connect(
            self.graphicsview.saveSceneLayoutConfigurationsToFile)
        
        self.__mSaveAs = QMenu()
        self.__mSaveAs.addAction(self.__acLayoutSaveAs)
        self.__mSaveAs.addAction(self.__acConfigurationsSaveAs)
        self.__mSaveAs.addAction(self.__acLayoutConfigurationsSaveAs)
        self.__tbSaveAs.setMenu(self.__mSaveAs)

        self.graphicsactions = list(self.graphicsview.add_actions(self))
        

    def setupToolBars(self, standardToolBar, parent):
        standardToolBar.addAction(self.__acDesignMode)

        toolBar = ToolBar('Drawing')
        toolBar.setVisible(self.graphicsview.designMode)
        parent.addToolBar(toolBar)
        self.graphicsview.setFocusProxy(toolBar)
        toolBar.setFocusPolicy(Qt.StrongFocus)
        
        toolBar.addSeparator()
        toolBar.addWidget(self.__tbOpen)
        toolBar.addWidget(self.__tbSaveAs)
        
        toolBar.addSeparator()
        #toolBar.addWidget(self.__tbAddShape)
        for a in self.graphicsactions:
            toolBar.addAction(a)
        self.drawingToolBar = toolBar

        # Add placeholder widget to toolbar
        widget = QWidget()
        widget.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        self.drawingToolBar.addWidget(widget)


    def openScene(self, filename):
        self.graphicsview.openScene(filename)


    def saveScene(self, filename):
        self.graphicsview.saveScene(filename)


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
        self.graphicsview.designMode = isChecked


    def onUndock(self):
        pass


    def onDock(self):
        pass
