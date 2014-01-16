#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the configuration panel on the
   right of the MainWindow which is un/dockable.
"""

__all__ = ["ConfigurationPanel"]


import const

from parametertreewidget import ParameterTreeWidget
from docktabwindow import DockTabWindow
from documentationpanel import DocumentationPanel
from enums import ConfigChangeTypes
from enums import NavigationItemTypes
from manager import Manager
from navigationtreeview import NavigationTreeView

from schemareader import SchemaReader 

from karabo.karathon import *
from PyQt4.QtCore import SIGNAL, Qt, QTimer
from PyQt4.QtGui import QAction, QHBoxLayout, QIcon, QMenu, QPushButton, QSplitter, \
                        QStackedWidget, QVBoxLayout, QWidget


class ConfigurationPanel(QWidget):
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


    def __init__(self, treemodel):
        super(ConfigurationPanel, self).__init__()
        
        self.__toolBar = None
        # map = { deviceInternalKey, swIndex }
        self.__navItemInternalKeyIndexMap = dict()
        # map = { deviceInternalKey, bool }
        self.__internalKeySchemaLoadedMap = dict()
        
        self.__schemaReader = SchemaReader()
        
        title = "Configuration Editor"
        self.setWindowTitle(title)
        
        # map = { deviceId, timer }
        self.__changingTimerDeviceIdMap = dict()

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
        Manager().notifier.signalSystemTopologyChanged.connect(self.onSystemTopologyChanged)
        
        Manager().notifier.signalGlobalAccessLevelChanged.connect(self.onGlobalAccessLevelChanged)
        
        Manager().notifier.signalNewNavigationItem.connect(self.onNewNavigationItem)
        Manager().notifier.signalSelectNewNavigationItem.connect(self.onSelectNewNavigationItem)
        Manager().notifier.signalSchemaAvailable.connect(self.onSchemaAvailable)
        Manager().notifier.signalDeviceSchemaUpdated.connect(self.onDeviceSchemaUpdated)
        
        Manager().notifier.signalNavigationItemChanged.connect(self.onNavigationItemChanged)
        Manager().notifier.signalNavigationItemSelectionChanged.connect(self.onNavigationItemSelectionChanged)

        Manager().notifier.signalProjectItemChanged.connect(self.onProjectItemChanged)

        Manager().notifier.signalInstanceGone.connect(self.onInstanceGone)
        
        Manager().notifier.signalDeviceStateChanged.connect(self.onDeviceStateChanged)
        Manager().notifier.signalConflictStateChanged.connect(self.onConflictStateChanged)
        Manager().notifier.signalChangingState.connect(self.onChangingState)
        Manager().notifier.signalErrorState.connect(self.onErrorState)

        self.__prevDevicePath = str() # previous selected DEVICE_INSTANCE internalKey
        self.__swParameterEditor = QStackedWidget(splitTopPanes)
        # Initial page
        twInitalParameterEditorPage = ParameterTreeWidget(self)
        twInitalParameterEditorPage.setHeaderLabels(["Parameter", "Value"])
        self.__swParameterEditor.addWidget(twInitalParameterEditorPage)
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

        #mainSplitter.setSizes([1,1])

        mainSplitter.setStretchFactor(0, 6)
        mainSplitter.setStretchFactor(1, 1)

        self.setupActions()
        self.setLayout(mainLayout)


    def setupActions(self):
        text = "Open configuration (*.xml)"
        self.__acFileOpen = QAction(QIcon(":open"), "&Open configuration", self)
        #self.__acFileOpen = QAction(QIcon(":filein"), "&Open configuration", self)
        self.__acFileOpen.setStatusTip(text)
        self.__acFileOpen.setToolTip(text)
        self.__acFileOpen.setVisible(False)
        self.__acFileOpen.triggered.connect(self.onFileOpen)

        text = "Save configuration as (*.xml)"
        self.__acFileSaveAs = QAction(QIcon(":save-as"), "Save &As...", self)
        #self.__acFileSaveAs = QAction(QIcon(":fileout"), "Save &As...", self)
        self.__acFileSaveAs.setStatusTip(text)
        self.__acFileSaveAs.setToolTip(text)
        self.__acFileSaveAs.setVisible(False)
        self.__acFileSaveAs.triggered.connect(self.onFileSaveAs)


    def setupToolBars(self, standardToolBar, parent):
        standardToolBar.addAction(self.__acFileOpen)
        standardToolBar.addAction(self.__acFileSaveAs)
        

    def applyAllAsHash(self, key, config):
        Manager().onDeviceChangedAsHash(key, config)


    def getParameterTreeWidgetItemByKey(self, key):
        return self._r_getParameterTreeWidgetItemByKey(self._getCurrentParameterEditor().invisibleRootItem(), key)


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
        nbSelected = self._getCurrentParameterEditor().nbSelectedApplyEnabledItems()
        if (self.__pbApplyAll.isEnabled() is True) and (nbSelected > 0):
            if nbSelected == 1:
                text = "Apply selected"
            else:
                text = "Apply ({}) selected".format(nbSelected)
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
        nbSelected = self._getCurrentParameterEditor().nbSelectedApplyEnabledItems()
        if (self.__pbResetAll.isEnabled() is True) and (nbSelected > 0):
            if nbSelected == 1:
                text = "Reset selected"
            else:
                text = "Reset ({}) selected".format(nbSelected)
        else:
            text = "Reset all"
        
        self.__pbResetAll.setText(text)
        self.__pbResetAll.setStatusTip(text)
        self.__pbResetAll.setToolTip(text)

        self.__acResetAll.setText(text)
        self.__acResetAll.setStatusTip(text)
        self.__acResetAll.setToolTip(text)


    def clearParameterEditorContent(self):
        self._setParameterEditorIndex(0)
        # Reset map
        self.__navItemInternalKeyIndexMap = dict()
        
        while self.__swParameterEditor.count() > 1:
            twParameterEditorPage = self.__swParameterEditor.widget(self.__swParameterEditor.count()-1)
            # Unregister all widgets of TreeWidget from DataNotifier in Manager before clearing..
            self._r_unregisterEditableComponent(twParameterEditorPage.invisibleRootItem())
            twParameterEditorPage.clear()
            
            # Remove widget completely
            self.__swParameterEditor.removeWidget(twParameterEditorPage)
            del twParameterEditorPage


    def _parseSchema(self, itemInfo, twParameterEditorPage):
        path = itemInfo.get('key')
        schema = itemInfo.get('schema')
        
        # Distinguish between DEVICE_CLASS and DEVICE_INSTANCE
        deviceType = itemInfo.get('type')
        self.__schemaReader.setDeviceType(deviceType)

        if not self.__schemaReader.readSchema(path, schema, twParameterEditorPage):
            return False
        
        return True


    def _createNewParameterPage(self, itemInfo):
        classId = itemInfo.get('classId')
        
        path = itemInfo.get('key')
        type = itemInfo.get('type')

        twParameterEditorPage = ParameterTreeWidget(self, path, classId)
        twParameterEditorPage.setHeaderLabels([
            "Parameter", "Current value on device", "Value"])
        
        twParameterEditorPage.addContextAction(self.__acFileOpen)
        twParameterEditorPage.addContextAction(self.__acFileSaveAs)
        twParameterEditorPage.addContextSeparator()
        twParameterEditorPage.addContextAction(self.__acKillInstance)
        twParameterEditorPage.addContextAction(self.__acApplyAll)
        twParameterEditorPage.addContextAction(self.__acResetAll)
        
        if type is NavigationItemTypes.CLASS:
            twParameterEditorPage.hideColumn(1)
        
        index = self.__swParameterEditor.addWidget(twParameterEditorPage)
        self._parseSchema(itemInfo, twParameterEditorPage)
        return index


### getter functions ###
    def _navigationTreeWidget(self):
        return self.__twNavigation
    navigationTreeWidget = property(fget=_navigationTreeWidget)


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


    def _setParameterEditorIndex(self, index):
        self.__swParameterEditor.blockSignals(True)
        self.__swParameterEditor.setCurrentIndex(index)
        self.__swParameterEditor.blockSignals(False)

        show = index != 0
        self.__acFileOpen.setVisible(show)
        self.__acFileSaveAs.setVisible(show)


    def _hideAllButtons(self):
        # Hide buttons and actions
        self.__pbInitDevice.setVisible(False)
        
        self.__pbKillInstance.setVisible(False)
        self.__acKillInstance.setVisible(False)
        self.__pbApplyAll.setVisible(False)
        self.__acApplyAll.setVisible(False)
        self.__pbResetAll.setVisible(False)
        self.__acResetAll.setVisible(False)


    def _getCurrentParameterEditor(self):
        return self.__swParameterEditor.currentWidget()


    def _updateButtonsVisibility(self, visible):
        self.__pbInitDevice.setVisible(visible)
        
        self.__pbKillInstance.setVisible(not visible)
        self.__pbApplyAll.setVisible(not visible)
        self.__pbResetAll.setVisible(not visible)
        
        self.__acKillInstance.setVisible(not visible)
        self.__acApplyAll.setVisible(not visible)
        self.__acResetAll.setVisible(not visible)
    updateButtonsVisibility = property(fset=_updateButtonsVisibility)


    def _r_getParameterTreeWidgetItemByKey(self, item, key):
        for i in range(item.childCount()):
            childItem = item.child(i)
            result = self._r_getParameterTreeWidgetItemByKey(childItem, key)
            if (result is not None):
                return result
            
        if item.internalKey == key:
            return item
        return None


    def _applyAllChanges(self):
        config = Hash()
        self._getCurrentParameterEditor().onApplyAll(config)
        self.applyAllAsHash(self._getCurrentParameterEditor().instanceKey, config)
        self._setApplyAllEnabled(False)


    def _applySelectedChanges(self):
        selectedItems = self._getCurrentParameterEditor().selectedItems()
        config = Hash()
        for item in selectedItems:
            self._getCurrentParameterEditor().addItemDataToHash(item, config)
        
        self.applyAllAsHash(self._getCurrentParameterEditor().instanceKey, config)
        self._setApplyAllEnabled(self._getCurrentParameterEditor().checkApplyButtonsEnabled()[0])


    def _applyAllRemoteChanges(self):
        self._getCurrentParameterEditor().onApplyAllRemoteChanges()


    def _applySelectedRemoteChanges(self):
        selectedItems = self._getCurrentParameterEditor().selectedItems()
        for item in selectedItems:
            self._getCurrentParameterEditor().applyRemoteChanges(item)


    def _r_unregisterEditableComponent(self, item):
        # Go recursively through tree and unregister Widgets in Manager
        for i in range(item.childCount()):
            childItem = item.child(i)
            childItem.unregisterEditableComponent()
            childItem.unregisterDisplayComponent()
            self._r_unregisterEditableComponent(childItem)


    def showParameterPage(self, type, path):
        # Show correct parameters
        index = self.__navItemInternalKeyIndexMap.get(path)
        if index:
            self._setParameterEditorIndex(index)
            
            if (type is NavigationItemTypes.DEVICE) and (self.__prevDevicePath != path):
                # Visible deviceId has changed
                Manager().newVisibleDevice(path)
                                            
                self.__prevDevicePath = path
        else:
            self._setParameterEditorIndex(0)
            
            if type is NavigationItemTypes.SERVER:
                return
            
            # Hide buttons and actions
            self._hideAllButtons()


### slots ###
    def onNewNavigationItem(self, itemInfo):
        # itemInfo: id, name, type, (status), (refType), (refId), (schema)
        self.__twNavigation.createNewItem(itemInfo)


    def onSelectNewNavigationItem(self, devicePath):
        self.__twNavigation.selectItem(devicePath)


    def onSchemaAvailable(self, itemInfo):
        # Update map deviceId = swIndex
        key = itemInfo.get('key')
        
        if (key in self.__navItemInternalKeyIndexMap) and (key in self.__internalKeySchemaLoadedMap):
            index = self.__navItemInternalKeyIndexMap.get(key)
            if index:
                twParameterEditorPage = self.__swParameterEditor.widget(index)
                # Parsing of schema necessary?
                schemaLoaded = self.__internalKeySchemaLoadedMap.get(key)
                if not schemaLoaded:
                    # Unregister all widgets of TreeWidget from DataNotifier in Manager before clearing..
                    self._r_unregisterEditableComponent(twParameterEditorPage.invisibleRootItem())
                    twParameterEditorPage.clear()

                    if self._parseSchema(itemInfo, twParameterEditorPage):
                        self.__internalKeySchemaLoadedMap[key] = True
        else:
            self.__navItemInternalKeyIndexMap[key] = self._createNewParameterPage(itemInfo)
            # Schema might not be there yet...
            schema = itemInfo.get('schema')
            
            if schema:
                self.__internalKeySchemaLoadedMap[key] = True
            else:
                self.__internalKeySchemaLoadedMap[key] = False


    # signal from Manager NavigationTreeWidgetItem clicked (NavigationPanel)
    def onNavigationItemClicked(self):
        #print "ConfigurationPanel.itemClicked"
        type = self.__twNavigation.itemClicked()
        if type is NavigationItemTypes.UNDEFINED:
            return
        
        if type is NavigationItemTypes.CLASS:
            self.updateButtonsVisibility = True
        else:
            self.updateButtonsVisibility = False


    def onNavigationItemChanged(self, itemInfo):
        #print "ConfigurationPanel.itemChanged", itemInfo
        type = itemInfo.get('type')
        path = itemInfo.get('key')
        
        if type is NavigationItemTypes.CLASS:
            self.updateButtonsVisibility = True
        elif (type is NavigationItemTypes.SERVER) or (type is NavigationItemTypes.DEVICE):
            self.updateButtonsVisibility = False
        
        if self.__prevDevicePath != "":
            Manager().removeVisibleDevice(self.__prevDevicePath)
            self.__prevDevicePath = str()
        
        self.__twNavigation.itemChanged(itemInfo)
        
        self.showParameterPage(type, path)


    def onNavigationItemSelectionChanged(self, path):
        self.__twNavigation.selectItem(path)


    def onProjectItemChanged(self, itemInfo):
        type = itemInfo.get('type')
        path = itemInfo.get('key')

        self.showParameterPage(type, str(path))


    def onInstanceGone(self, path, parentPath):
        path = str(path)
        # New schema can be in plugins of instance
        keys = self.__internalKeySchemaLoadedMap.keys()
        for key in keys:
            if path in key:
                self.__internalKeySchemaLoadedMap[key] = False

        # Check whether path of the gone instance were selected
        path = self.__twNavigation.lastSelectionPath
        if len(path) < 1:
            index = None
        else:
            index = self.__twNavigation.findIndex(path)
        
        if index and index.isValid():
            self.__twNavigation.lastSelectionPath = str()
            self.__twNavigation.selectItem(path)
        else:
            self._setParameterEditorIndex(0)
            self.__twNavigation.selectItem(str(parentPath))
            # Hide buttons and actions
            self._hideAllButtons()


    def onDeviceStateChanged(self, internalKey, state):
        index = self.__navItemInternalKeyIndexMap.get(internalKey)
        if index is None:
            index = self.__navItemInternalKeyIndexMap.get(str(internalKey))
        if index:
            twParameterEditorPage = self.__swParameterEditor.widget(index)
            twParameterEditorPage.stateUpdated(state)


    def onConflictStateChanged(self, hasConflict):
        result = self._getCurrentParameterEditor().checkApplyButtonsEnabled()
        if result[1] == hasConflict:
            self.hasConflicts = hasConflict


    def onChangingState(self, deviceId, isChanging):
        if deviceId in self.__changingTimerDeviceIdMap:
            timer = self.__changingTimerDeviceIdMap[deviceId]
        else:
            timer = QTimer(self)
            timer.timeout.connect(self.onTimeOut)
            self.__changingTimerDeviceIdMap[deviceId] = timer
        
        if isChanging is True:
            if timer.isActive() is False:
                timer.start(200)
        else:
            timer.stop()
            
            parameterWidget = self._getCurrentParameterEditor()
            if deviceId == parameterWidget.instanceKey:
                self._getCurrentParameterEditor().setReadOnly(False)

 
    def onErrorState(self, inErrorState):
        self._getCurrentParameterEditor().setErrorState(inErrorState)


    def onTimeOut(self):
        timer = self.sender()
        timer.stop()
        
        # Check deviceId against deviceId of current parameter editor
        mapValues = self.__changingTimerDeviceIdMap.values()
        for i in xrange(len(mapValues)):
            if timer == mapValues[i]:
                deviceId = self.__changingTimerDeviceIdMap.keys()[i]
                
                parameterWidget = self._getCurrentParameterEditor()
                if deviceId == parameterWidget.instanceKey:
                    parameterWidget.setReadOnly(True)
                break


    def onApplyChanged(self, enable, hasConflicts=False):
        # called when apply button of attributewidget changed
        self._setApplyAllEnabled(enable)
        self._setResetAllEnabled(enable)
        self.hasConflicts = hasConflicts


    def onApplyAll(self):
        if self._getCurrentParameterEditor().nbSelectedApplyEnabledItems() > 0:
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
        self._setApplyAllEnabled(self._getCurrentParameterEditor().checkApplyButtonsEnabled()[0])


    def onResetAll(self):
        if self._getCurrentParameterEditor().nbSelectedApplyEnabledItems() > 0:
            self._applySelectedRemoteChanges()
        else:
            self._applyAllRemoteChanges()


    def onKillInstance(self):
        itemInfo = self.__twNavigation.indexInfo()
        
        type = itemInfo.get('type')
        
        if type is NavigationItemTypes.DEVICE:
            deviceId = itemInfo.get('deviceId')
            Manager().killDevice(deviceId)
        elif type is NavigationItemTypes.SERVER:
            serverId = itemInfo.get('serverId')
            Manager().killServer(serverId)


    def onInitDevice(self):
        itemInfo = self.__twNavigation.indexInfo()
        if len(itemInfo) == 0:
            return
        
        serverId = itemInfo.get('serverId')
        classId = itemInfo.get('classId')
        path = itemInfo.get('key')
        
        Manager().initDevice(str(serverId), str(classId), str(path))


    def onDeviceSchemaUpdated(self, key):
        key = str(key)
        self.__internalKeySchemaLoadedMap[key] = False


    def onSystemTopologyChanged(self, config):
        # Already done in NavigationPanel which uses the same model
        #self.__twNavigation.updateTreeModel(config) 
        self.__twNavigation.expandAll()
        
        path = self.__twNavigation.lastSelectionPath
        if len(path) < 1:
            return
        self.__twNavigation.lastSelectionPath = str()
        self.__twNavigation.selectItem(path)


    def onGlobalAccessLevelChanged(self):
        for index in xrange(self.__swParameterEditor.count()):
            twParameterEditorPage = self.__swParameterEditor.widget(index)
            twParameterEditorPage.globalAccessLevelChanged()


    def onFileOpen(self):
        info = self.__twNavigation.indexInfo()
        
        path = info.get('key')
        type = info.get('type')
        classId = info.get('classId')
        deviceId = info.get('deviceId')
        
        configChangeType = None
        if type is NavigationItemTypes.CLASS:
            configChangeType = ConfigChangeTypes.DEVICE_CLASS_CONFIG_CHANGED
        elif type is NavigationItemTypes.DEVICE:
            configChangeType = ConfigChangeTypes.DEVICE_INSTANCE_CONFIG_CHANGED
        
        if classId is None:
            print "onFileOpen classId not set"
        
        # TODO: Remove dirty hack for scientific computing again!!!
        croppedClassId = classId.split("-")
        classId = croppedClassId[0]
        
        Manager().onFileOpen(configChangeType, str(path), str(classId))


    def onFileSaveAs(self):
        info = self.__twNavigation.indexInfo()
        
        path = info.get('key')
        type = info.get('type')
        classId = info.get('classId')
        deviceId = info.get('deviceId')

        Manager().onSaveAsXml(str(classId), str(path))


    # virtual function
    def onUndock(self):
        self.__twNavigation.show()


    # virtual function
    def onDock(self):
        self.__twNavigation.hide()

