#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the custom view panel in the middle
   of the MainWindow which is un/dockable.
   
   As a dockable widget class used in DivWidget, it needs the following interfaces
   implemented:
   
    def setupActions(self):
        pass
    def setupToolBar(self, toolBar):
        pass
    def onUndock(self):
        pass
    def onDock(self):
        pass
"""

__all__ = ["CustomMiddlePanel"]


from customwidget import CustomWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class CustomMiddlePanel(QWidget):

    def __init__(self):
        super(CustomMiddlePanel, self).__init__()

        self.__customWidget = CustomWidget(self)
        self.__customWidget.lineInserted.connect(self.onLineInserted)
        self.__customWidget.rectInserted.connect(self.onRectInserted)
        self.__customWidget.sceneSelectionChanged.connect(self.updateActions)
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(3,3,3,3)
        mainLayout.addWidget(self.__customWidget)
        
        self.setupActions()
        self.updateActions()


    def setupActions(self):
        text = "Edit mode"
        self.__acEditableMode = QAction(QIcon(":transform"), text, self)
        self.__acEditableMode.setToolTip(text)
        self.__acEditableMode.setStatusTip(text)
        self.__acEditableMode.setCheckable(True)
        self.__acEditableMode.setChecked(False)
        self.__acEditableMode.toggled.connect(self.onEditableModeChanged)

        text = "Open"
        self.__acOpen = QAction(QIcon(":open"), text, self)
        self.__acOpen.setStatusTip(text)
        self.__acOpen.setToolTip(text)
        self.__acOpen.triggered.connect(self.onOpen)

        text = "Save as"
        self.__acSaveAs = QAction(QIcon(":save-as"), text, self)
        self.__acSaveAs.setStatusTip(text)
        self.__acSaveAs.setToolTip(text)
        self.__acSaveAs.triggered.connect(self.onSaveAs)

        #text = "Add shape"
        #self.__tbAddShape = QToolButton(self)
        #self.__tbAddShape.setIcon(QIcon(":shape"))
        #self.__tbAddShape.setToolTip(text)
        #self.__tbAddShape.setStatusTip(text)
        #self.__tbAddShape.setPopupMode(QToolButton.InstantPopup)
        
        text = "Add text"
        self.__acAddText = QAction(QIcon(":text"), text, self)
        #self.__acAddText.setCheckable(True)
        #self.__acAddText.setChecked(False)
        self.__acAddText.setStatusTip(text)
        self.__acAddText.setToolTip(text)
        self.__acAddText.triggered.connect(self.onAddText)
        
        text = "Add line"
        self.__acAddLine = QAction(QIcon(":line"), text, self)
        self.__acAddLine.setCheckable(True)
        self.__acAddLine.setChecked(False)
        self.__acAddLine.setStatusTip(text)
        self.__acAddLine.setToolTip(text)
        self.__acAddLine.toggled.connect(self.onAddLine)
        
        text = "Add rectangle"
        self.__acAddRect = QAction(QIcon(":rect"), text, self)
        self.__acAddRect.setCheckable(True)
        self.__acAddRect.setChecked(False)
        self.__acAddRect.setStatusTip(text)
        self.__acAddRect.setToolTip(text)
        self.__acAddRect.toggled.connect(self.onAddRect)
        
        #self.__mAddShape = QMenu()
        #self.__mAddShape.addAction(self.__acAddText)
        #self.__mAddShape.addAction(self.__acAddLine)
        #self.__mAddShape.addAction(self.__acAddRect)
        #self.__tbAddShape.setMenu(self.__mAddShape)
        
        text = "Add simple link"
        self.__acAddSimpleLink = QAction(QIcon(":link"), text, self)
        self.__acAddSimpleLink.setStatusTip(text)
        self.__acAddSimpleLink.setToolTip(text)
        self.__acAddSimpleLink.triggered.connect(self.onAddSimpleLink)
        
        text = "Add arrow link"
        self.__acAddArrow = QAction(QIcon(":arrow"), text, self)
        self.__acAddArrow.setStatusTip(text)
        self.__acAddArrow.setToolTip(text)
        self.__acAddArrow.triggered.connect(self.onAddArrowLink)
        
        text = "Cut"
        self.__acCut = QAction(QIcon(":edit-cut"), text, self)
        self.__acCut.setStatusTip(text)
        self.__acCut.setToolTip(text)
        self.__acCut.triggered.connect(self.onCut)
        
        text = "Copy"
        self.__acCopy = QAction(QIcon(":edit-copy"), text, self)
        self.__acCopy.setStatusTip(text)
        self.__acCopy.setToolTip(text)
        self.__acCopy.triggered.connect(self.onCopy)
        
        text = "Paste"
        self.__acPaste = QAction(QIcon(":edit-paste"), text, self)
        self.__acPaste.setStatusTip(text)
        self.__acPaste.setToolTip(text)
        self.__acPaste.triggered.connect(self.onPaste)
        
        text = "Remove"
        self.__acRemove = QAction(QIcon(":edit-remove"), text, self)
        self.__acRemove.setStatusTip(text)
        self.__acRemove.setToolTip(text)
        self.__acRemove.triggered.connect(self.onRemove)
        
        text = "Rotate"
        self.__acRotate = QAction(QIcon(":transform-rotate"), text, self)
        self.__acRotate.setStatusTip(text)
        self.__acRotate.setToolTip(text)
        self.__acRotate.triggered.connect(self.onRotate)
        text = "Scale"
        self.__acScale = QAction(QIcon(":transform-scale"), text, self)
        self.__acScale.setStatusTip(text)
        self.__acScale.setToolTip(text)
        self.__acScale.triggered.connect(self.onScale)
        
        text = "Group"
        self.__tbGroup = QToolButton(self)
        self.__tbGroup.setIcon(QIcon(":group"))
        self.__tbGroup.setToolTip(text)
        self.__tbGroup.setStatusTip(text)
        self.__tbGroup.setPopupMode(QToolButton.InstantPopup)
        
        text = "Group selected items"
        self.__acGroupItems = QAction(QIcon(":group"), text, self)
        self.__acGroupItems.setStatusTip(text)
        self.__acGroupItems.setToolTip(text)
        self.__acGroupItems.triggered.connect(self.onGroupItems)
        
        text = "Ungroup selected items"
        self.__acUnGroupItems = QAction(QIcon(":ungroup"), text, self)
        self.__acUnGroupItems.setStatusTip(text)
        self.__acUnGroupItems.setToolTip(text)
        self.__acUnGroupItems.triggered.connect(self.onUnGroupItems)
        
        self.__mGroup = QMenu()
        self.__mGroup.addAction(self.__acGroupItems)
        self.__mGroup.addAction(self.__acUnGroupItems)
        self.__tbGroup.setMenu(self.__mGroup)
        
        text = "Bring to front"
        self.__acBringToFront = QAction(QIcon(":bringtofront"), text, self)
        self.__acBringToFront.setStatusTip(text)
        self.__acBringToFront.setToolTip(text)
        self.__acBringToFront.triggered.connect(self.onBringToFront)
        text = "Send to back"
        self.__acSendToBack = QAction(QIcon(":sendtoback"), text, self)
        self.__acSendToBack.setStatusTip(text)
        self.__acSendToBack.setToolTip(text)
        self.__acSendToBack.triggered.connect(self.onSendToBack)


    def setupToolBar(self, toolBar):
        toolBar.addAction(self.__acEditableMode)
        
        toolBar.addSeparator()
        toolBar.addAction(self.__acOpen)
        toolBar.addAction(self.__acSaveAs)
        
        toolBar.addSeparator()
        #toolBar.addWidget(self.__tbAddShape)
        toolBar.addAction(self.__acAddText)
        toolBar.addAction(self.__acAddLine)
        toolBar.addAction(self.__acAddRect)
        
        toolBar.addSeparator()
        toolBar.addAction(self.__acAddSimpleLink)
        toolBar.addAction(self.__acAddArrow)
        
        toolBar.addSeparator()
        toolBar.addAction(self.__acCut)
        toolBar.addAction(self.__acCopy)
        toolBar.addAction(self.__acPaste)
        toolBar.addAction(self.__acRemove)
        
        toolBar.addSeparator()
        toolBar.addAction(self.__acRotate)
        toolBar.addAction(self.__acScale)
        
        toolBar.addSeparator()
        toolBar.addWidget(self.__tbGroup)
        
        toolBar.addSeparator()
        toolBar.addAction(self.__acBringToFront)
        toolBar.addAction(self.__acSendToBack)


    # Depending on the (non-)selected items the actions are enabled/disabled
    def updateActions(self):
        hasSelection = len(self.__customWidget.selectedItems()) > 0
        isLink = self.__customWidget.selectedLinks() is not None
        isItemPair = self.__customWidget.selectedItemPair() is not None
        isItemGroup = self.__customWidget.selectedItemGroup() is not None
        
        if hasSelection:
            self.__acAddSimpleLink.setDisabled(not isItemPair)
            self.__acAddArrow.setDisabled(not isItemPair)
            
            self.__acCut.setDisabled(isLink)
            self.__acCopy.setDisabled(isLink)

            self.__acRotate.setDisabled(isLink)
            self.__acScale.setDisabled(isLink)

            self.__acBringToFront.setDisabled(isLink)
            self.__acSendToBack.setDisabled(isLink)
        else:
            self.__acAddSimpleLink.setDisabled(True)
            self.__acAddArrow.setDisabled(True)
            
            self.__acCut.setDisabled(True)
            self.__acCopy.setDisabled(True)

            self.__acRotate.setDisabled(True)
            self.__acScale.setDisabled(True)

            self.__acBringToFront.setDisabled(True)
            self.__acSendToBack.setDisabled(True)
        
        self.__acPaste.setDisabled(not self.__customWidget.hasCopy())
        self.__acRemove.setDisabled(not hasSelection)
        
        self.__tbGroup.setDisabled(not isItemGroup and len(self.__customWidget.selectedItems()) < 2)
        self.__acGroupItems.setDisabled(isItemGroup)
        self.__acUnGroupItems.setDisabled(not isItemGroup)


    def onLineInserted(self):
        self.__acAddLine.setChecked(False)


    def onRectInserted(self):
        self.__acAddRect.setChecked(False)


### slots ###
    def onEditableModeChanged(self, isChecked):
        if isChecked:
            text = "Edit mode"
        else:
            text = "View mode"
        
        self.__acEditableMode.setToolTip(text)
        self.__acEditableMode.setStatusTip(text)
        self.__customWidget.setEditableMode(isChecked)


    def onOpen(self):
        self.__customWidget.openSceneFromFile()


    def onSaveAs(self):
        self.__customWidget.saveSceneToFile()


    def onAddText(self):
        self.__customWidget.addText()


    def onAddLine(self, checked):
        self.__customWidget.aboutToInsertLine(checked)


    def onAddRect(self, checked):
        self.__customWidget.aboutToInsertRect(checked)


    def onAddSimpleLink(self):
        self.__customWidget.addLink()

    
    def onAddArrowLink(self):
        self.__customWidget.addArrowLink()


    def onCut(self):
        self.__customWidget.cut()
        self.updateActions()


    def onCopy(self):
        self.__customWidget.copy()
        self.updateActions()


    def onPaste(self):
        self.__customWidget.paste()


    def onRemove(self):
        self.__customWidget.remove()
        

    def onRotate(self):
        self.__customWidget.rotate()


    def onScale(self):
        self.__customWidget.scale()


    def onGroupItems(self):
        self.__customWidget.groupItems()


    def onUnGroupItems(self):
        self.__customWidget.unGroupItems()


    def onBringToFront(self):
        self.__customWidget.bringToFront()

    
    def onSendToBack(self):
        self.__customWidget.sendToBack()


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass

