#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the configuration panel on the
   right of the MainWindow which is un/dockable.
   
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

__all__ = ["ConfigurationPanel"]


import const

from attributetreewidget import AttributeTreeWidget
from docktabwindow import DockTabWindow
from documentationpanel import DocumentationPanel
from enums import NavigationItemTypes
from manager import Manager
from navigationtreeview import NavigationTreeView
from xsdreader import XsdReader

from libkarabo import *
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtXmlPatterns import *

class ConfigurationPanel(QWidget):
    
    def __init__(self, treemodel):
        super(ConfigurationPanel, self).__init__()
        
        self.__toolBar = None
        # map = { deviceInternalKey, swIndex }
        self.__navItemInternalKeyIndexMap = dict()
        # map = { deviceInternalKey, bool }
        self.__internalKeySchemaLoadedMap = dict()
        
        # TODO: removed because is is not supported yet for SL6 (introduced with Qt 4.6)
        #self.__xmlSchema = QXmlSchema()
        self.__xsdReader = XsdReader()
        
        title = "Configuration Editor"
        self.setWindowTitle(title)
        
        self.__changingStateTimer = QTimer(self)
        self.__changingStateTimer.timeout.connect(self.onTimeOut)

        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainSplitter = QSplitter(Qt.Vertical)

        # widget with navigation/attributeeditor-splitter and button-layout
        topWidget = QWidget(mainSplitter)
        vLayout = QVBoxLayout(topWidget)
        vLayout.setContentsMargins(0,0,0,0)
        
        # splitter for navigation + attributeeditor
        splitTopPanes = QSplitter(Qt.Horizontal, topWidget)
        vLayout.addWidget(splitTopPanes)
                
        self.__twNavigation = NavigationTreeView(splitTopPanes, treemodel)
        self.connect(self.__twNavigation.selectionModel(),
                    SIGNAL('selectionChanged(const QItemSelection &, const QItemSelection &)'),
                    self.onNavigationItemClicked)
        self.__twNavigation.hide()

        splitTopPanes.setStretchFactor(0, 1)
        
        # Make connects
        Manager().notifier.signalNewNavigationItem.connect(self.onNewNavigationItem)
        Manager().notifier.signalSelectNewNavigationItem.connect(self.onSelectNewNavigationItem)
        Manager().notifier.signalSchemaAvailable.connect(self.onSchemaAvailable)
        Manager().notifier.signalDeviceInstanceSchemaUpdated.connect(self.onDeviceInstanceSchemaUpdated)
        
        Manager().notifier.signalNavigationItemChanged.connect(self.onNavigationItemChanged)

        Manager().notifier.signalUpdateDeviceServerInstance.connect(self.onUpdateDeviceServerInstance)
        Manager().notifier.signalUpdateDeviceInstance.connect(self.onUpdateDeviceInstance)
        Manager().notifier.signalDeviceInstanceStateChanged.connect(self.onDeviceInstanceStateChanged)
        Manager().notifier.signalConflictStateChanged.connect(self.onConflictStateChanged)
        Manager().notifier.signalChangingState.connect(self.onChangingState)
        Manager().notifier.signalErrorState.connect(self.onErrorState)

        self.__prevDevInsKey = str() # previous selected DEVICE_INSTANCE internalKey
        self.__swAttributeEditor = QStackedWidget(splitTopPanes)
        # Initial page
        twInitalAttributeEditorPage = AttributeTreeWidget(self)
        twInitalAttributeEditorPage.setHeaderLabels(QStringList() << "Attribute" << "Value")
        self.__swAttributeEditor.addWidget(twInitalAttributeEditorPage)
        splitTopPanes.setStretchFactor(1, 3)

        hLayout = QHBoxLayout()
        hLayout.setContentsMargins(0,5,5,5)
        
        text = "Initiate device"
        self.__pbInitDevice = QPushButton(QIcon(":start"), text)
        self.__pbInitDevice.setToolTip(text)
        self.__pbInitDevice.setStatusTip(text)
        self.__pbInitDevice.setVisible(False)
        self.__pbInitDevice.setMinimumSize(140,32)
        self.__pbInitDevice.clicked.connect(self.onInitDevice)
        hLayout.addWidget(self.__pbInitDevice)

        text = "Kill instance"
        self.__pbKillInstance = QPushButton(QIcon(":delete"), text)
        self.__pbKillInstance.setStatusTip(text)
        self.__pbKillInstance.setToolTip(text)
        self.__pbKillInstance.setVisible(False)
        self.__pbKillInstance.setMinimumSize(140,32)
        # use action for button to reuse
        self.__acKillInstance = QAction(QIcon(":delete"), text, self)
        self.__acKillInstance.setStatusTip(text)
        self.__acKillInstance.setToolTip(text)
        self.__acKillInstance.triggered.connect(self.onKillInstance)
        self.__pbKillInstance.clicked.connect(self.__acKillInstance.triggered)
        hLayout.addWidget(self.__pbKillInstance)
        
        self.__hasConflicts = False

        text = "Apply all"
        self.__pbApplyAll = QPushButton(QIcon(":apply"), text)
        self.__pbApplyAll.setToolTip(text)
        self.__pbApplyAll.setStatusTip(text)
        self.__pbApplyAll.setVisible(False)
        self.__pbApplyAll.setEnabled(False)
        self.__pbApplyAll.setMinimumSize(140,32)
        # use action for button to reuse
        self.__acApplyAll = QAction(QIcon(":apply"), text, self)
        self.__acApplyAll.setStatusTip(text)
        self.__acApplyAll.setToolTip(text)
        self.__acApplyAll.setEnabled(False)
        self.__acApplyAll.triggered.connect(self.onApplyAll)
        self.__pbApplyAll.clicked.connect(self.__acApplyAll.triggered)

        text = "Accept all local changes"
        self.__acApplyLocalChanges = QAction(text, self)
        self.__acApplyLocalChanges.setStatusTip(text)
        self.__acApplyLocalChanges.setToolTip(text)
        self.__acApplyLocalChanges.triggered.connect(self.onApplyAll)

        text = "Accept all remote changes"
        self.__acApplyRemoteChanges = QAction(text, self)
        self.__acApplyRemoteChanges.setStatusTip(text)
        self.__acApplyRemoteChanges.setToolTip(text)
        self.__acApplyRemoteChanges.triggered.connect(self.onApplyAllRemoteChanges)

        text = "Apply selected local changes"
        self.__acApplySelectedChanges = QAction(text, self)
        self.__acApplySelectedChanges.setStatusTip(text)
        self.__acApplySelectedChanges.setToolTip(text)
        self.__acApplySelectedChanges.triggered.connect(self.onApplySelected)

        text = "Accept selected remote changes"
        self.__acApplySelectedRemoteChanges = QAction(text, self)
        self.__acApplySelectedRemoteChanges.setStatusTip(text)
        self.__acApplySelectedRemoteChanges.setToolTip(text)
        self.__acApplySelectedRemoteChanges.triggered.connect(self.onApplySelectedRemoteChanges)
        
        # add menu to toolbutton
        self.__mApply = QMenu(self.__pbApplyAll)
        self.__mApply.addAction(self.__acApplyLocalChanges)
        self.__mApply.addAction(self.__acApplyRemoteChanges)
        self.__mApply.addSeparator()
        self.__mApply.addAction(self.__acApplySelectedChanges)
        self.__mApply.addAction(self.__acApplySelectedRemoteChanges)
        
        hLayout.addWidget(self.__pbApplyAll)
        
        text = "Reset all"
        self.__pbResetAll = QPushButton(QIcon(":no"), text)
        self.__pbResetAll.setToolTip(text)
        self.__pbResetAll.setStatusTip(text)
        self.__pbResetAll.setVisible(False)
        self.__pbResetAll.setEnabled(False)
        self.__pbResetAll.setMinimumSize(140,32)
        # use action for button to reuse
        self.__acResetAll = QAction(QIcon(":no"), text, self)
        self.__acResetAll.setStatusTip(text)
        self.__acResetAll.setToolTip(text)
        self.__acResetAll.setEnabled(False)
        self.__acResetAll.triggered.connect(self.onResetAll)
        self.__pbResetAll.clicked.connect(self.__acResetAll.triggered)

        hLayout.addWidget(self.__pbResetAll)
        hLayout.addStretch()
        vLayout.addLayout(hLayout)
        
        self.__documentationPanel = DocumentationPanel()
        documentation = DockTabWindow("Documentation", mainSplitter)
        documentation.addDockableTab(self.__documentationPanel, "Documentation")
        
        mainLayout.addWidget(mainSplitter)

        mainSplitter.setSizes([1,1])

        mainSplitter.setStretchFactor(0, 1)
        mainSplitter.setStretchFactor(1, 0)

        self.setupActions()
        self.setLayout(mainLayout)


    def setupActions(self):
        pass


    def setupToolBar(self, toolBar):
        # this is done per AttributeTreeWidget
        self.__toolBar = toolBar
        

    def applyAllAsHash(self, key, config):
        Manager().onDeviceInstanceChangedAsHash(key, config)


    def getAttributeTreeWidgetItemByKey(self, key):
        return self._r_getAttributeTreeWidgetItemByKey(self._getCurrentAttributeEditor().invisibleRootItem(), key)


    def getNavigationItemType(self):
        return self.__twNavigation.currentIndexType()


    def addActionToToolBar(self, action):
        if self.__toolBar is None: return
        self.__toolBar.addAction(action)


    def removeActionFromToolBar(self, action):
        if self.__toolBar is None: return
        self.__toolBar.removeAction(action)


    def updateButtonActions(self):
        self.updateApplyAllActions()
        self.updateResetAllActions()


    def updateApplyAllActions(self):
        nbSelected = self._getCurrentAttributeEditor().nbSelectedApplyEnabledItems()
        if (self.__pbApplyAll.isEnabled() is True) and (nbSelected > 0):
            if nbSelected == 1:
                text = "Apply selected"
            else:
                text = QString("Apply (%1) selected").arg(nbSelected)
            self.__acApplyLocalChanges.setVisible(False)
            self.__acApplyRemoteChanges.setVisible(False)
            self.__acApplySelectedChanges.setVisible(True)
            self.__acApplySelectedRemoteChanges.setVisible(True)
        else:
            text = "Apply all"
            self.__acApplyLocalChanges.setVisible(True)
            self.__acApplyRemoteChanges.setVisible(True)
            self.__acApplySelectedChanges.setVisible(False)
            self.__acApplySelectedRemoteChanges.setVisible(False)

        self.__pbApplyAll.setText(text)
        self.__pbApplyAll.setStatusTip(text)
        self.__pbApplyAll.setToolTip(text)

        self.__acApplyAll.setText(text)
        self.__acApplyAll.setStatusTip(text)
        self.__acApplyAll.setToolTip(text)
        
        if self.hasConflicts is True:
            text = "Resolve conflicts"
            self.__pbApplyAll.setStatusTip(text)
            self.__pbApplyAll.setToolTip(text)
            self.__pbApplyAll.setMenu(self.__mApply)
            
            self.__acApplyAll.setStatusTip(text)
            self.__acApplyAll.setToolTip(text)
            self.__acApplyAll.setMenu(self.__mApply)
        else:
            self.__pbApplyAll.setMenu(None)
            self.__acApplyAll.setMenu(None)


    def updateResetAllActions(self):
        nbSelected = self._getCurrentAttributeEditor().nbSelectedApplyEnabledItems()
        if (self.__pbResetAll.isEnabled() is True) and (nbSelected > 0):
            if nbSelected == 1:
                text = "Reset selected"
            else:
                text = QString("Reset (%1) selected").arg(nbSelected)
        else:
            text = "Reset all"
        
        self.__pbResetAll.setText(text)
        self.__pbResetAll.setStatusTip(text)
        self.__pbResetAll.setToolTip(text)

        self.__acResetAll.setText(text)
        self.__acResetAll.setStatusTip(text)
        self.__acResetAll.setToolTip(text)


    def _createNewAttributePage(self, itemInfo):
        devClaId = itemInfo.get(QString('devClaId'))
        if devClaId is None:
            devClaId = itemInfo.get('devClaId')
        internalKey = itemInfo.get(QString('key'))
        if internalKey is None:
            internalKey = itemInfo.get('key')
        type = itemInfo.get(QString('type'))
        if type is None:
            type = itemInfo.get('type')
        schema = itemInfo.get(QString('schema'))
        if schema is None:
            schema = itemInfo.get('schema')
        
        # TODO: removed because it is not supported yet for SL6 (introduced with Qt 4.6)
        #self.__xmlSchema.load(schema.toLatin1())
        #if self.__xmlSchema.isValid() == False :
        #    QMessageBox.critical(None, "XML Schema", "The given XML schema file is invalid")
        #    return

        twAttributeEditorPage = AttributeTreeWidget(self, internalKey, devClaId)
        twAttributeEditorPage.setHeaderLabels(QStringList() << "Attribute" << "Current value on device" << "Value")
        twAttributeEditorPage.addConfigAction(self.__acKillInstance)
        twAttributeEditorPage.addConfigAction(self.__acApplyAll)
        #twAttributeEditorPage.addConfigMenu(self.__mApply)
        twAttributeEditorPage.addConfigAction(self.__acResetAll)
        
        if type is NavigationItemTypes.DEVICE_CLASS:
            twAttributeEditorPage.hideColumn(1)
        
        index = self.__swAttributeEditor.addWidget(twAttributeEditorPage)

        if self.__xsdReader.parseContent(twAttributeEditorPage, self.__documentationPanel, itemInfo) == False:
            QMessageBox.critical(self, "XSD Schema", "The given XSD schema file is invalid")
            return

        return index


### getter functions ###
    def _navigationTreeWidget(self):
        return self.__twNavigation
    navigationTreeWidget = property(fget=_navigationTreeWidget)


    def _attributeStackedWidget(self):
        return self.__swAttributeEditor
    attributeStackedWidget = property(fget=_attributeStackedWidget)


    def _hasConflicts(self):
        return self.__hasConflicts
    def _setHasConflicts(self, hasConflicts):
        self.__hasConflicts = hasConflicts
        
        if hasConflicts is True:
            icon = QIcon(":apply-conflict")
            text = "Resolve conflict"
            self.__pbApplyAll.setIcon(icon)
            self.__pbApplyAll.setStatusTip(text)
            self.__pbApplyAll.setToolTip(text)
            self.__pbApplyAll.setMenu(self.__mApply)

            self.__acApplyAll.setIcon(icon)
            self.__acApplyAll.setStatusTip(text)
            self.__acApplyAll.setToolTip(text)
            self.__acApplyAll.setMenu(self.__mApply)
            
            self.__pbResetAll.setEnabled(False)
            self.__acResetAll.setEnabled(False)
        else:
            icon = QIcon(":apply")
            text = "Apply all"
            self.__pbApplyAll.setIcon(icon)
            self.__pbApplyAll.setStatusTip(text)
            self.__pbApplyAll.setToolTip(text)
            self.__pbApplyAll.setMenu(None)
            
            self.__acApplyAll.setIcon(icon)
            self.__acApplyAll.setStatusTip(text)
            self.__acApplyAll.setToolTip(text)
            self.__acApplyAll.setMenu(None)
        self.__acApplyLocalChanges.setVisible(hasConflicts)
        self.__acApplyRemoteChanges.setVisible(hasConflicts)

        self.__acApplySelectedChanges.setVisible(not hasConflicts)
        self.__acApplySelectedRemoteChanges.setVisible(not hasConflicts)
    hasConflicts = property(fget=_hasConflicts, fset=_setHasConflicts)


    def _setApplyAllEnabled(self, enable):
        self.__pbApplyAll.setEnabled(enable)
        self.__acApplyAll.setEnabled(enable)
        self.updateApplyAllActions()
        self.updateResetAllActions()


    def _setResetAllEnabled(self, enable):
        self.__pbResetAll.setEnabled(enable)
        self.__acResetAll.setEnabled(enable)
        self.updateResetAllActions()


    def _getCurrentAttributeEditor(self):
        return self.__swAttributeEditor.currentWidget()


    def _updateButtonsVisibility(self, visible):
        self.__pbInitDevice.setVisible(visible)
        
        self.__pbKillInstance.setVisible(not visible)
        self.__pbApplyAll.setVisible(not visible)
        self.__pbResetAll.setVisible(not visible)
        
        self.__acKillInstance.setVisible(not visible)
        self.__acApplyAll.setVisible(not visible)
        self.__acResetAll.setVisible(not visible)
    updateButtons = property(fset=_updateButtonsVisibility)


    def _r_getAttributeTreeWidgetItemByKey(self, item, key):
        for i in range(item.childCount()):
            childItem = item.child(i)
            result = self._r_getAttributeTreeWidgetItemByKey(childItem, key)
            if (result is not None):
                return result
            
        if item.internalKey == key:
            return item
        return None


    def _applyAllChanges(self):
        config = Hash()
        self._getCurrentAttributeEditor().onApplyAll(config)
        self.applyAllAsHash(self._getCurrentAttributeEditor().instanceKey, config)
        self._setApplyAllEnabled(False)


    def _applySelectedChanges(self):
        selectedItems = self._getCurrentAttributeEditor().selectedItems()
        config = Hash()
        for item in selectedItems:
            self._getCurrentAttributeEditor().addItemDataToHash(item, config)
        
        self.applyAllAsHash(self._getCurrentAttributeEditor().instanceKey, config)
        self._setApplyAllEnabled(self._getCurrentAttributeEditor().checkApplyButtonsEnabled()[0])


    def _applyAllRemoteChanges(self):
        self._getCurrentAttributeEditor().onApplyAllRemoteChanges()


    def _applySelectedRemoteChanges(self):
        selectedItems = self._getCurrentAttributeEditor().selectedItems()
        for item in selectedItems:
            self._getCurrentAttributeEditor().applyRemoteChanges(item)


    def _r_unregisterEditableComponent(self, item):
        # Go recursively through tree and unregister Widgets in Manager
        for i in range(item.childCount()):
            childItem = item.child(i)
            childItem.unregisterEditableComponent()
            childItem.unregisterDisplayComponent()
            self._r_unregisterEditableComponent(childItem)


### slots ###
    def onNewNavigationItem(self, itemInfo):
        # itemInfo: id, name, type, (status), (refType), (refId), (schema)
        self.__twNavigation.createNewItem(itemInfo)


    def onSelectNewNavigationItem(self, itemInfo):
        self.__twNavigation.selectItem(itemInfo)


    def onSchemaAvailable(self, itemInfo):
        # Update map deviceId = swIndex
        key = itemInfo.get(QString('key'))
        if key is None:
            key = itemInfo.get('key')
        
        if (key in self.__navItemInternalKeyIndexMap) and (key in self.__internalKeySchemaLoadedMap):
            index = self.__navItemInternalKeyIndexMap.get(key)
            if index is not None:
                twAttributeEditorPage = self.__swAttributeEditor.widget(index)
                
                # Parsing of schema necessary?
                schemaLoaded = self.__internalKeySchemaLoadedMap.get(key)
                if not schemaLoaded:
                    # Unregister all widgets of TreeWidget from DataNotifier in Manager before clearing..
                    self._r_unregisterEditableComponent(twAttributeEditorPage.invisibleRootItem())
                    twAttributeEditorPage.clear()
                    if self.__xsdReader.parseContent(twAttributeEditorPage, self.__documentationPanel, itemInfo) == False:
                        QMessageBox.critical(self, "XSD Schema", "The given XSD schema file is invalid")
        else:
            self.__navItemInternalKeyIndexMap[key] = self._createNewAttributePage(itemInfo)
            self.__internalKeySchemaLoadedMap[key] = True


    # signal from PanelManager NavigationTreeWidgetItem clicked (NavigationPanel)
    def onNavigationItemClicked(self):
        #print "ConfigurationPanel.itemClicked"
        type = self.__twNavigation.itemClicked()
        if type is NavigationItemTypes.UNDEFINED:
            return
        
        if type is NavigationItemTypes.DEVICE_CLASS:
            self.updateButtons = True
        else:
            self.updateButtons = False


    def onNavigationItemChanged(self, itemInfo):
        #print "ConfigurationPanel.itemChanged", itemInfo
        type = itemInfo.get(QString('type'))
        if type is None:
            type = itemInfo.get('type')
        key = itemInfo.get(QString('key'))
        if key is None:
            key = itemInfo.get('key')
        
        if type is NavigationItemTypes.DEVICE_CLASS:
            self.updateButtons = True
        else:
            self.updateButtons = False
        
        # hide apply button of current AttributeTreeWidget
        self._getCurrentAttributeEditor().setActionsVisible(False)
        if self.__prevDevInsKey != "":
            Manager().removeVisibleDeviceInstance(self.__prevDevInsKey)
            self.__prevDevInsKey = str()

        self.__twNavigation.itemChanged(itemInfo)
        
        # Show correct attributes
        index = self.__navItemInternalKeyIndexMap.get(QString(key))
        if index is not None:
            self.__swAttributeEditor.blockSignals(True)
            
            self.__swAttributeEditor.setCurrentIndex(index)
            
            # show apply button of current AttributeTreeWidget
            self._getCurrentAttributeEditor().setActionsVisible(True)
            self.__swAttributeEditor.blockSignals(False)
            
            if (type is NavigationItemTypes.DEVICE_INSTANCE) and (self.__prevDevInsKey != key):
                # visible DEVICE_INSTANCE has changed
                Manager().newVisibleDeviceInstance(key)
                self.__prevDevInsKey = key
        else:
            self.__swAttributeEditor.blockSignals(True)
            self.__swAttributeEditor.setCurrentIndex(0)
            self.__swAttributeEditor.blockSignals(False)


    def onUpdateDeviceServerInstance(self, itemInfo):
        # New schema can be in plugins of device server instance
        name = itemInfo.get(QString('name'))
        if name is None:
            name = itemInfo.get('name')
        keys = self.__internalKeySchemaLoadedMap.keys()
        for key in keys:
            if name in key:
                self.__internalKeySchemaLoadedMap[key] = False
        self.__twNavigation.updateDeviceServerInstance(itemInfo)


    def onUpdateDeviceInstance(self, itemInfo):
        name = itemInfo.get(QString('name'))
        if name is None:
            name = itemInfo.get('name')
        keys = self.__internalKeySchemaLoadedMap.keys()
        for key in keys:
            if name in key:
                self.__internalKeySchemaLoadedMap[key] = False
        self.__twNavigation.updateDeviceInstance(itemInfo)
        self._setApplyAllEnabled(False)


    def onDeviceInstanceStateChanged(self, internalKey, state):
        index = self.__navItemInternalKeyIndexMap.get(internalKey)
        if index is None:
            index = self.__navItemInternalKeyIndexMap.get(str(internalKey))
        if index is not None:
            twAttributeEditor = self.__swAttributeEditor.widget(index)
            twAttributeEditor.stateUpdated(state)


    def onConflictStateChanged(self, hasConflict):
        result = self._getCurrentAttributeEditor().checkApplyButtonsEnabled()
        if result[1] == hasConflict:
            self.hasConflicts = hasConflict


    def onChangingState(self, isChanging):
        if isChanging is True:
            if self.__changingStateTimer.isActive() is False:
                self.__changingStateTimer.start(200)
        else:
            self.__changingStateTimer.stop()
            self._getCurrentAttributeEditor().setReadOnly(False)
 
 
    def onErrorState(self, instanceId, hasError):
        self._getCurrentAttributeEditor().setErrorState(hasError)
        self.__twNavigation.setErrorState(instanceId, hasError)
 

    def onTimeOut(self):
        self.__changingStateTimer.stop()
        self._getCurrentAttributeEditor().setReadOnly(True)


    def onAttributeItemChanged(self, item):
        if item is None:
            self.__documentationPanel.setCurrentIndex(0)
        else:
            self.__documentationPanel.setCurrentIndex(item.descriptionIndex)


    def onApplyChanged(self, enable, hasConflicts=False):
        # called when apply button of attributewidget changed
        self._setApplyAllEnabled(enable)
        self._setResetAllEnabled(enable)
        self.hasConflicts = hasConflicts


    def onApplyAll(self):
        if self._getCurrentAttributeEditor().nbSelectedApplyEnabledItems() > 0:
            self._applySelectedChanges()
        else:
            self._applyAllChanges()


    def onApplyAllRemoteChanges(self):
        self._applyAllRemoteChanges()
        self._setApplyAllEnabled(False)


    def onApplySelected(self):
        self._applySelectedChanges()


    def onApplySelectedRemoteChanges(self):
        self._applySelectedRemoteChanges()
        self._setApplyAllEnabled(self._getCurrentAttributeEditor().checkApplyButtonsEnabled()[0])


    def onResetAll(self):
        if self._getCurrentAttributeEditor().nbSelectedApplyEnabledItems() > 0:
            self._applySelectedRemoteChanges()
        else:
            self._applyAllRemoteChanges()


    def onKillInstance(self):
        itemInfo = self.__twNavigation.currentIndexInfo()
        
        type = itemInfo.get(QString('type'))
        if type is None:
            type = itemInfo.get('type')
        
        devSerInsId = itemInfo.get(QString('devSerInsId'))
        if devSerInsId is None:
            devSerInsId = itemInfo.get('devSerInsId')
        
        internalKey = itemInfo.get(QString('internalKey'))
        if internalKey is None:
            internalKey = itemInfo.get('internalKey')
        
        if type is NavigationItemTypes.DEVICE_INSTANCE:
            Manager().killDeviceInstance(devSerInsId, internalKey)
        elif type is NavigationItemTypes.DEVICE_SERVER_INSTANCE:
            Manager().killDeviceServerInstance(internalKey)


    def onInitDevice(self):
        itemInfo = self.__twNavigation.currentIndexInfo()
        if len(itemInfo) == 0:
            return
        
        devSerInsId = itemInfo.get(QString('devSerInsId'))
        if devSerInsId is None:
            devSerInsId = itemInfo.get('devSerInsId')
        
        devClaId = itemInfo.get(QString('devClaId'))
        if devClaId is None:
            devClaId = itemInfo.get('devClaId')
        
        internalKey = itemInfo.get(QString('internalKey'))
        if internalKey is None:
            internalKey = itemInfo.get('internalKey')
        
        Manager().initDevice(str(devSerInsId), str(devClaId), str(internalKey))


    def onDeviceInstanceSchemaUpdated(self, instanceId, schema):
        self.__internalKeySchemaLoadedMap[instanceId] = False
        self.__twNavigation.updateDeviceInstanceSchema(instanceId, schema)


    # virtual function
    def onUndock(self):
        self.__twNavigation.show()


    # virtual function
    def onDock(self):
        self.__twNavigation.hide()

