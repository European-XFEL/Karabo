#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the custom view panel in the middle
   of the MainWindow which is un/dockable.
"""

__all__ = ["CustomMiddlePanel"]


from graphicsview import GraphicsView
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


    def __init__(self):
        super(CustomMiddlePanel, self).__init__()

        self.graphicsview = GraphicsView(self)
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(3,3,3,3)
        mainLayout.addWidget(self.graphicsview)
        
        self.setupActions()
        self.updateActions()


    def setupActions(self):
        text = "Change to control mode"
        self.__acDesignMode = QAction(QIcon(":transform"), text, self)
        self.__acDesignMode.setToolTip(text)
        self.__acDesignMode.setStatusTip(text)
        self.__acDesignMode.setCheckable(True)
        self.__acDesignMode.setChecked(True)
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

        #text = "Add shape"
        #self.__tbAddShape = QToolButton(self)
        #self.__tbAddShape.setIcon(QIcon(":shape"))
        #self.__tbAddShape.setToolTip(text)
        #self.__tbAddShape.setStatusTip(text)
        #self.__tbAddShape.setPopupMode(QToolButton.InstantPopup)

        self.graphicsactions = list(self.graphicsview.add_actions(self))
        
        text = "Add text"
        self.__acAddText = QAction(QIcon(":text"), text, self)
        #self.__acAddText.setCheckable(True)
        #self.__acAddText.setChecked(False)
        self.__acAddText.setStatusTip(text)
        self.__acAddText.setToolTip(text)
        #self.__acAddText.triggered.connect(self.onAddText)
        
        text = "Add line"
        self.__acAddLine = QAction(QIcon(":line"), text, self)
        self.__acAddLine.setCheckable(True)
        self.__acAddLine.setChecked(False)
        self.__acAddLine.setStatusTip(text)
        self.__acAddLine.setToolTip(text)
        #self.__acAddLine.toggled.connect(self.onAddLine)
        
        text = "Add rectangle"
        self.__acAddRect = QAction(QIcon(":rect"), text, self)
        self.__acAddRect.setCheckable(True)
        self.__acAddRect.setChecked(False)
        self.__acAddRect.setStatusTip(text)
        self.__acAddRect.setToolTip(text)
        #self.__acAddRect.toggled.connect(self.onAddRect)
        
        #self.__mAddShape = QMenu()
        #self.__mAddShape.addAction(self.__acAddText)
        #self.__mAddShape.addAction(self.__acAddLine)
        #self.__mAddShape.addAction(self.__acAddRect)
        #self.__tbAddShape.setMenu(self.__mAddShape)
        
        text = "Connect two nodes"
        self.__acAddLink = QAction(QIcon(":link"), text, self)
        self.__acAddLink.setStatusTip(text)
        self.__acAddLink.setToolTip(text)
        #self.__acAddLink.triggered.connect(self.onAddLink)

        # TODO: temporary removed, because works not properly..
        #text = "Add arrow link"
        #self.__acAddArrow = QAction(QIcon(":arrow"), text, self)
        #self.__acAddArrow.setStatusTip(text)
        #self.__acAddArrow.setToolTip(text)
        #self.__acAddArrow.triggered.connect(self.onAddArrowLink)
        
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
        text = "Scale up"
        self.__acScaleUp = QAction(QIcon(":transform-scale-up"), text, self)
        self.__acScaleUp.setStatusTip(text)
        self.__acScaleUp.setToolTip(text)
        self.__acScaleUp.triggered.connect(self.onScaleUp)
        text = "Scale down"
        self.__acScaleDown = QAction(QIcon(":transform-scale-down"), text, self)
        self.__acScaleDown.setStatusTip(text)
        self.__acScaleDown.setToolTip(text)
        self.__acScaleDown.triggered.connect(self.onScaleDown)
        
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
        
        text = "Horizontal layout"
        self.__acHLayout = QAction(text, self)
        self.__acHLayout.setStatusTip(text)
        self.__acHLayout.setToolTip(text)
        self.__acHLayout.triggered.connect(self.onHorizontalLayout)
        
        text = "Vertical layout"
        self.__acVLayout = QAction(text, self)
        self.__acVLayout.setStatusTip(text)
        self.__acVLayout.setToolTip(text)
        self.__acVLayout.triggered.connect(self.onVerticalLayout)
        
        text = "Break layout"
        self.__acBreakLayout = QAction(text, self)
        self.__acBreakLayout.setStatusTip(text)
        self.__acBreakLayout.setToolTip(text)
        self.__acBreakLayout.triggered.connect(self.onBreakLayout)
        
        self.__mGroupInLayout = QMenu("Group selected items in")
        self.__mGroupInLayout.addAction(self.__acHLayout)
        self.__mGroupInLayout.addAction(self.__acVLayout)
        self.__mGroupInLayout.addSeparator()
        self.__mGroupInLayout.addAction(self.__acBreakLayout)
        
        text = "Ungroup selected items"
        self.__acUnGroupItems = QAction(QIcon(":ungroup"), text, self)
        self.__acUnGroupItems.setStatusTip(text)
        self.__acUnGroupItems.setToolTip(text)
        self.__acUnGroupItems.triggered.connect(self.onUnGroupItems)
        
        self.__mGroup = QMenu()
        self.__mGroup.addAction(self.__acGroupItems)
        self.__mGroup.addMenu(self.__mGroupInLayout)
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


    def setupToolBars(self, standardToolBar, parent):
        standardToolBar.addAction(self.__acDesignMode)

        toolBar = ToolBar('Drawing')
        parent.addToolBar(toolBar)
        
        toolBar.addSeparator()
        toolBar.addWidget(self.__tbOpen)
        toolBar.addWidget(self.__tbSaveAs)
        
        toolBar.addSeparator()
        #toolBar.addWidget(self.__tbAddShape)
        for a in self.graphicsactions:
            toolBar.addAction(a)
        self.drawingToolBar = toolBar
        
        toolBar.addSeparator()
        toolBar.addAction(self.__acAddLink)
        #toolBar.addAction(self.__acAddArrow)
        
        toolBar.addSeparator()
        toolBar.addAction(self.__acCut)
        toolBar.addAction(self.__acCopy)
        toolBar.addAction(self.__acPaste)
        toolBar.addAction(self.__acRemove)
        
        toolBar.addSeparator()
        toolBar.addAction(self.__acRotate)
        toolBar.addAction(self.__acScaleUp)
        toolBar.addAction(self.__acScaleDown)
        
        toolBar.addSeparator()
        toolBar.addWidget(self.__tbGroup)
        
        toolBar.addSeparator()
        toolBar.addAction(self.__acBringToFront)
        toolBar.addAction(self.__acSendToBack)


    # Depending on the (non-)selected items the actions are enabled/disabled
    def updateActions(self):
        self.__acAddText.setEnabled(True)
        self.__acAddLine.setEnabled(True)
        self.__acAddRect.setEnabled(True)
        
        hasSelection = len(self.graphicsview.selectedItems()) > 0
        isLink = self.graphicsview.selectedLinks() is not None
        isItemPair = self.graphicsview.selectedItemPair() is not None
        isItemGroup = self.graphicsview.selectedItemGroup() is not None
        
        if hasSelection:
            self.__acAddLink.setDisabled(not isItemPair)
            #self.__acAddArrow.setDisabled(not isItemPair)
            
            self.__acCut.setDisabled(isLink)
            self.__acCopy.setDisabled(isLink)

            self.__acRotate.setDisabled(isLink)
            self.__acScaleUp.setDisabled(isLink)
            self.__acScaleDown.setDisabled(isLink)

            self.__acBringToFront.setDisabled(isLink)
            self.__acSendToBack.setDisabled(isLink)
        else:
            self.__acAddLink.setDisabled(True)
            #self.__acAddArrow.setDisabled(True)
            
            self.__acCut.setDisabled(True)
            self.__acCopy.setDisabled(True)

            self.__acRotate.setDisabled(True)
            self.__acScaleUp.setDisabled(True)
            self.__acScaleDown.setDisabled(True)

            self.__acBringToFront.setDisabled(True)
            self.__acSendToBack.setDisabled(True)
        
        self.__acPaste.setDisabled(not self.graphicsview.hasCopy())
        self.__acRemove.setDisabled(not hasSelection)
        
        self.__tbGroup.setDisabled(not isItemGroup and len(self.graphicsview.selectedItems()) < 2)
        self.__acGroupItems.setDisabled(isItemGroup)
        self.__mGroupInLayout.setDisabled(isItemGroup)
        self.__acUnGroupItems.setDisabled(not isItemGroup)


    def enableActions(self, enable):
        self.__acAddText.setEnabled(enable)
        self.__acAddLine.setEnabled(enable)
        self.__acAddRect.setEnabled(enable)

        self.__acAddLink.setEnabled(enable)

        self.__acCut.setEnabled(enable)
        self.__acCopy.setEnabled(enable)
        self.__acPaste.setEnabled(enable)
        self.__acRemove.setEnabled(enable)

        self.__acRotate.setEnabled(enable)
        self.__acScaleUp.setEnabled(enable)
        self.__acScaleDown.setEnabled(enable)

        self.__tbGroup.setEnabled(enable)

        self.__acBringToFront.setEnabled(enable)
        self.__acSendToBack.setEnabled(enable)


    def onLineInserted(self):
        self.__acAddLine.setChecked(False)


    def onRectInserted(self):
        self.__acAddRect.setChecked(False)


### slots ###
    def onDesignModeChanged(self, isChecked):
        self.drawingToolBar.setVisible(isChecked)
        if isChecked:
            text = "Change to control mode"
        else:
            text = "Change to design mode"
        self.__acDesignMode.setToolTip(text)
        self.__acDesignMode.setStatusTip(text)
        self.graphicsview.setDesignMode(isChecked)


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


    def onScaleUp(self):
        self.__customWidget.scaleUp()


    def onScaleDown(self):
        self.__customWidget.scaleDown()


    def onGroupItems(self):
        self.__customWidget.groupItems()


    def onHorizontalLayout(self):
        self.__customWidget.horizontalLayout()


    def onVerticalLayout(self):
        self.__customWidget.verticalLayout()


    def onBreakLayout(self):
        self.__customWidget.breakLayout()


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


    def keyPressEvent(self, event):
        # TODO: cut and copy short cuts won't work, not quiet clear, why
        #if event.matches(QKeySequence.Cut):
        #    print "cut"
        #    self.onCut()
        #if event.matches(QKeySequence.Copy):
        #    print "copy"
        #    self.onCopy()
        if event.matches(QKeySequence.Paste):
            self.onPaste()
        if event.matches(QKeySequence.Delete):
            self.onRemove()

        QWidget.keyPressEvent(self, event)

