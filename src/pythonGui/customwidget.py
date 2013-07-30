#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 25, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the area in the middle where
   the user can drag and drop either items of the navigation panel or items of
   the configuration panel to get a user specific view of certain properties.
   
   This widget is embedded in the CustomMiddlePanel.
"""

__all__ = ["CustomWidget"]

from graphicsview import GraphicsView

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class CustomWidget(QWidget):
    # Signals
    lineInserted = pyqtSignal()
    rectInserted = pyqtSignal()
    sceneSelectionChanged = pyqtSignal()
    
    def __init__(self, parent):
        super(CustomWidget, self).__init__(parent)
        
        self.__view = GraphicsView()
        self.__view.lineInserted.connect(self.onLineInserted)
        self.__view.rectInserted.connect(self.onRectInserted)
        self.__view.sceneSelectionChanged.connect(self.onSceneSelectionChanged)
        
        self.__slZoom = QSlider(Qt.Horizontal)
        self.__slZoom.setRange(5, 200)
        self.__slZoom.setValue(100)
        self.__slZoom.valueChanged.connect(self.onZoom)
        hZoomLayout = QHBoxLayout()
        hZoomLayout.addStretch()
        hZoomLayout.addWidget(self.__slZoom)
        
        vLayout = QVBoxLayout(self)
        vLayout.setContentsMargins(0,0,0,0)
        vLayout.addWidget(self.__view)
        vLayout.addLayout(hZoomLayout)


### public functions ###
    def setDesignMode(self, isDesignMode):
        self.__view.setDesignMode(isDesignMode)


    # Returns true, when items has been copied; otherwise false
    def hasCopy(self):
        return self.__view.hasCopy()


    # All selected items of the scene are returned
    def selectedItems(self):
        return self.__view.selectedItems()


    # If there are exactely 2 selected items (not of type Link) they are returned
    # as a pair; otherwise None is returned
    def selectedItemPair(self):
        return self.__view.selectedItemPair()


    # All selected items of type Text are returned
    def selectedTextItems(self):
        return self.__view.selectedTextItems()
        

    # All selected items of type Link are returned
    def selectedLinks(self):
        return self.__view.selectedLinks()


    def selectedItemGroup(self):
        return self.__view.selectedItemGroup()


    def openSceneLayout(self):
        self.__view.openSceneLayoutFromFile()


    def openSceneConfigurations(self):
        self.__view.openSceneConfigurationsFromFile()


    def openSceneLayoutConfigurations(self):
        self.__view.openSceneLayoutConfigurationsFromFile()


    def saveSceneAsLayout(self):
        self.__view.saveSceneLayoutToFile()


    def saveSceneAsConfigurations(self):
        self.__view.saveSceneConfigurationsToFile()


    def saveSceneAsLayoutConfigurations(self):
        self.__view.saveSceneLayoutConfigurationsToFile()


    # Save active view including the configurations to folder/files
    def saveSceneAndConfigurationsToFile(self):
        self.__view.saveSceneAndConfigurationsToFile()


    # A new instance of a text is created and passed to the _setupItem function
    # to position and select
    def addText(self):
        self.__view.addText()


    # Add a line
    def aboutToInsertLine(self, checked):
        if checked:
            self.__view.mode = GraphicsView.InsertLine
        else:
            self.__view.mode = GraphicsView.MoveItem


    # Add a rectangle
    def aboutToInsertRect(self, checked):
        if checked:
            self.__view.mode = GraphicsView.InsertRect
        else:
            self.__view.mode = GraphicsView.MoveItem


    # Add a link, if exactely 2 items are selected
    def addLink(self):
        self.__view.addLink()

    
    # Add an arrow, if exactely 2 items are selected
    def addArrowLink(self):
        self.__view.addArrowLink()


    # Cut is two-part process: copy selected items and remove item from scene
    def cut(self):
        self.__view.cut()


    # The selected items are stored as binary data inside the application
    def copy(self):
        self.__view.copy()


    # The copied item data is extracted and the items are instantiated with the
    # refered binary data
    def paste(self):
        self.__view.paste()


    # All selected items are removed; when an item (not type Link) is removed its
    # destructor deletes any links that are associated with it
    # To avoid double-deleting links, the Link-items are removed before deleting the other items
    def remove(self):
        self.__view.remove()
        

    # Rotates all selected items around 30 degrees
    def rotate(self):
        self.__view.rotate()


    # Scales all selected items up (so far) : TODO use value
    def scaleUp(self): # value
        self.__view.scaleSelectedItemsUp()


    def scaleDown(self): # value
        self.__view.scaleSelectedItemsDown()


    def groupItems(self):
        self.__view.groupItems()


    def horizontalLayout(self):
        self.__view.horizontalLayout()


    def verticalLayout(self):
        self.__view.verticalLayout()


    def breakLayout(self):
        self.__view.breakLayout()


    def unGroupItems(self):
        self.__view.unGroupItems()


    # Increments self.__maxZ value, and then sets the currently selected item's z
    # value to self.__maxZ
    def bringToFront(self):
        self.__view.bringToFront()

    
    # Decrements self.__minZ value, and then sets the currently selected item's z
    # value to self.__minZ
    def sendToBack(self):
        self.__view.sendToBack()


### private slots ###
    # Called whenever the user rolls the mouse wheel, and it will cause the view
    # to scale smaller or larger depending on which way the wheel is rolled
    def onZoom(self, value):
        factor = value / 100.0
        matrix = self.__view.matrix()
        matrix.reset()
        matrix.scale(factor, factor)
        self.__view.setMatrix(matrix)


    # Called whenever an item of the scene is un-/selected
    def onSceneSelectionChanged(self):
        self.sceneSelectionChanged.emit()


    # Called whenever a new line item was inserted in the scene
    def onLineInserted(self):
        self.lineInserted.emit()


    # Called whenever a new line item was inserted in the scene
    def onRectInserted(self):
        self.rectInserted.emit()

