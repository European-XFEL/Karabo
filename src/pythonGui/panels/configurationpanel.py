#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the configuration panel on the
   right of the MainWindow which is un/dockable.
"""

__all__ = ["ConfigurationPanel"]


from docktabwindow import DockTabWindow
from documentationpanel import DocumentationPanel
from enums import NavigationItemTypes
from manager import Manager
from navigationtreeview import NavigationTreeView
from parametertreewidget import ParameterTreeWidget

from schemareader import SchemaReader 

from PyQt4.QtCore import Qt, QTimer
from PyQt4.QtGui import (QAction, QHBoxLayout, QIcon, QMenu, QPushButton,
                         QSplitter, QStackedWidget, QVBoxLayout, QWidget)

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
        treemodel.signalItemChanged.connect(self.onNavigationItemChanged)
        treemodel.signalInstanceNewReset.connect(self.onInstanceNewReset)
        self.__twNavigation.hide()

        splitTopPanes.setStretchFactor(0, 1)
        
        Manager().signalGlobalAccessLevelChanged.connect(self.onGlobalAccessLevelChanged)
        
        Manager().signalNewNavigationItem.connect(self.onNewNavigationItem)
        Manager().signalSelectNewNavigationItem.connect(self.onSelectNewNavigationItem)
        Manager().signalSchemaAvailable.connect(self.onSchemaAvailable)
        Manager().signalDeviceSchemaUpdated.connect(self.onDeviceSchemaUpdated)
        
        Manager().signalProjectItemChanged.connect(self.onProjectItemChanged)

        Manager().signalInstanceGone.connect(self.onInstanceGone)
        
        Manager().signalDeviceStateChanged.connect(self.onDeviceStateChanged)
        Manager().signalConflictStateChanged.connect(self.onConflictStateChanged)
        Manager().signalChangingState.connect(self.onChangingState)
        Manager().signalErrorState.connect(self.onErrorState)
        Manager().signalReset.connect(self.onResetPanel)

        self.prevConfiguration = None
        self.__swParameterEditor = QStackedWidget(splitTopPanes)
        # Initial page
        twParameterEditorPage = ParameterTreeWidget()
        twParameterEditorPage.setHeaderLabels(["Parameter", "Value"])
        self.__swParameterEditor.addWidget(twParameterEditorPage)
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
        self.__acApplySelectedChanges.triggered.connect(self.onApplyAll)

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


    def setupToolBars(self, toolBar, parent):
        toolBar.addAction(self.__acFileOpen)
        toolBar.addAction(self.__acFileSaveAs)


    def updateApplyAllActions(self, box):
        twParameterEditor = self.__swParameterEditor.widget(
            box.configuration.index)

        nbSelected = twParameterEditor.nbSelectedApplyEnabledItems()
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


    def updateResetAllActions(self, box):
        twParameterEditor = self.__swParameterEditor.widget(
            box.configuration.index)

        nbSelected = twParameterEditor.nbSelectedApplyEnabledItems()
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


    def _parseSchema(self, itemInfo, twParameterEditor):
        path = itemInfo.get('key')
        conf = itemInfo["configuration"]
        
        # Distinguish between DEVICE_CLASS and DEVICE_INSTANCE
        deviceType = itemInfo.get('type')
        if conf is None:
            return False
        else:
            conf.fillWidget(twParameterEditor)
        return True


    def _createNewParameterPage(self, configuration):
        twParameterEditor = ParameterTreeWidget(configuration)
        twParameterEditor.setHeaderLabels([
            "Parameter", "Current value on device", "Value"])
        
        twParameterEditor.addContextAction(self.__acFileOpen)
        twParameterEditor.addContextAction(self.__acFileSaveAs)
        twParameterEditor.addContextSeparator()
        twParameterEditor.addContextAction(self.__acKillInstance)
        twParameterEditor.addContextAction(self.__acApplyAll)
        twParameterEditor.addContextAction(self.__acResetAll)
        twParameterEditor.signalApplyChanged.connect(self.onApplyChanged)
        twParameterEditor.signalItemSelectionChanged.connect(self.onSelectionChanged)
        
        if type is NavigationItemTypes.CLASS:
            twParameterEditor.hideColumn(1)

        if configuration.schema is not None:
            configuration.fillWidget(twParameterEditor)
        return self.__swParameterEditor.addWidget(twParameterEditor)


### getter functions ###
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


    def _setApplyAllEnabled(self, box, enable):
        self.__pbApplyAll.setEnabled(enable)
        self.__acApplyAll.setEnabled(enable)
        self.updateApplyAllActions(box)


    def _setResetAllEnabled(self, box, enable):
        self.__pbResetAll.setEnabled(enable)
        self.__acResetAll.setEnabled(enable)
        self.updateResetAllActions(box)


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
    
    
    def _getParameterEditorByPath(self, path):
        """
        Returns the parameterEditor-Treewidget with the given \path.
        If not found, return None.
        """
        for index in range(self.__swParameterEditor.count()):
            twParameterEditor = self.__swParameterEditor.widget(index)
            if twParameterEditor.path is None:
                continue
            if path.startswith(twParameterEditor.path):
                return twParameterEditor
        return None


    def _updateButtonsVisibility(self, visible):
        self.__pbInitDevice.setVisible(visible)
        
        self.__pbKillInstance.setVisible(not visible)
        self.__pbApplyAll.setVisible(not visible)
        self.__pbResetAll.setVisible(not visible)
        
        self.__acKillInstance.setVisible(not visible)
        self.__acApplyAll.setVisible(not visible)
        self.__acResetAll.setVisible(not visible)
    updateButtonsVisibility = property(fset=_updateButtonsVisibility)


    def _r_unregisterComponents(self, item):
        # Go recursively through tree and unregister Widgets in Manager
        for i in range(item.childCount()):
            childItem = item.child(i)
            childItem.unregisterEditableComponent()
            childItem.unregisterDisplayComponent()
            self._r_unregisterComponents(childItem)


    def showParameterPage(self, configuration):
        """ Show the parameters for configuration """
        if hasattr(configuration, 'index'):
            self._setParameterEditorIndex(configuration.index)

            if (configuration.type == "device" and
                    self.prevConfiguration is not configuration):
                configuration.addVisible()
                self.prevConfiguration = configuration
        else:
            self._setParameterEditorIndex(0)
            
            #if type is NavigationItemTypes.SERVER:
            #    return ########################### TODO
            
            # Hide buttons and actions
            self._hideAllButtons()


    def _removeParameterEditorPage(self, twParameterEditor):
        """
        The \twParameterEditor is remove from StackedWidget and all registered
        components get unregistered.
        """
        if twParameterEditor is None:
            return

        # Unregister all widgets of TreeWidget from DataNotifier in Manager before clearing..
        self._r_unregisterComponents(twParameterEditor.invisibleRootItem())
        # Clear page
        twParameterEditor.clear()
        # Remove widget completely
        self.__swParameterEditor.removeWidget(twParameterEditor)
        self._setParameterEditorIndex(0)
        self._hideAllButtons()


### slots ###
    def onResetPanel(self):
        """
        This slot is called when the configurator needs a reset which means all
        parameter editor pages need to be cleaned and removed.
        """
        # Reset map
        self.__navItemInternalKeyIndexMap = dict()

        while self.__swParameterEditor.count() > 1:
            self._removeParameterEditorPage(self.__swParameterEditor.widget(self.__swParameterEditor.count()-1))


    def onInstanceNewReset(self, path):
        """
        This slot is called when a new instance is available which means, if there
        was already a parameter editor for the given path created it needs to
        be cleaned and removed.
        """
        # Remove \path from map
        if path in self.__navItemInternalKeyIndexMap:
            del self.__navItemInternalKeyIndexMap[path]
        self._removeParameterEditorPage(self._getParameterEditorByPath(path))


    def onNewNavigationItem(self, itemInfo):
        # itemInfo: id, name, type, (status), (refType), (refId), (schema)
        self.__twNavigation.createNewItem(itemInfo)


    def onSelectNewNavigationItem(self, devicePath):
        self.__twNavigation.selectItem(devicePath)


    def onSchemaAvailable(self, configuration):
        if hasattr(configuration, 'index'):
            twParameterEditor = self.__swParameterEditor.widget(
                configuration.index)
            if configuration.schema is None:
                self._r_unregisterComponents(twParameterEditor.invisibleRootItem())
                twParameterEditor.clear()
            else:
                configuration.fillWidget(twParameterEditor)
        else:
            configuration.index = self._createNewParameterPage(configuration)


    def onNavigationItemChanged(self, configuration):
        self.updateButtonsVisibility = configuration.type == 'class'

        if self.prevConfiguration not in (None, configuration):
            configuration.removeVisible()
            self.prevConfiguration = None
        
        self.showParameterPage(configuration)


    def onProjectItemChanged(self, itemInfo):
        type = itemInfo.get('type')
        path = itemInfo.get('key')

        self.showParameterPage(type, str(path))


    def onInstanceGone(self, instanceId, parentPath):
        # New schema can be in plugins of instance
        keys = self.__internalKeySchemaLoadedMap.keys()
        for key in keys:
            if instanceId in key:
                self.__internalKeySchemaLoadedMap[key] = False

        self._setParameterEditorIndex(0)
        self._hideAllButtons()
        self.__twNavigation.selectItem(parentPath)


    def onDeviceStateChanged(self, conf, state):
        if hasattr(conf, 'index'):
            twParameterEditor = self.__swParameterEditor.widget(conf.index)
            twParameterEditor.stateUpdated(state)


    def onConflictStateChanged(self, deviceId, hasConflict):
        parameterEditor = Manager().deviceData[deviceId].parameterEditor

        result = parameterEditor.checkApplyButtonsEnabled()
        self.__pbApplyAll.setEnabled(result[0])
        self.__pbResetAll.setEnabled(result[0])
        if result[1] == hasConflict:
            self.hasConflicts = hasConflict


    def onChangingState(self, conf, isChanging):
        if not hasattr(conf, "timer"):
            conf.timer = QTimer()
            conf.timer.timeout.connect(self.onTimeOut)
        timer = conf.timer

        if isChanging:
            if not timer.isActive():
                timer.start(200)
        else:
            timer.stop()

            if hasattr(conf, 'index'):
                parameterEditor = self.__swParameterEditor.widget(conf.index)
                parameterEditor.setReadOnly(False)

 
    def onErrorState(self, conf, inErrorState):
        if hasattr(conf, 'index'):
            parameterEditor = self.__swParameterEditor.widget(conf.index)
            parameterEditor.setErrorState(inErrorState)


    def onTimeOut(self):
        timer = self.sender()
        timer.stop()
        
        # Check path against path of current parameter editor
        mapValues = self.__changingTimerDeviceIdMap.values()
        for i in xrange(len(mapValues)):
            if timer == mapValues[i]:
                path = self.__changingTimerDeviceIdMap.keys()[i]
                
                parameterEditor = self._getParameterEditorByPath(path)
                if parameterEditor:
                    parameterEditor.setReadOnly(True)
                break


    def onApplyChanged(self, box, enable, hasConflicts=False):
        # Called when apply button of ParameterPage changed
        self._setApplyAllEnabled(box, enable)
        self._setResetAllEnabled(box, enable)
        self.hasConflicts = hasConflicts


    def onSelectionChanged(self, path):
        """
        This function is call from the current parameterEditor whenever the
        selection of this widget changed and this selection includes an apply
        enabled.
        
        \path The path of the current parameterEditor.
        """
        self.updateApplyAllActions(path)
        self.updateResetAllActions(path)


    def onApplyAll(self):
        self._getCurrentParameterEditor().onApplyAll()


    def onApplyAllRemoteChanges(self):
        self._getCurrentParameterEditor().onApplyAllRemoteChanges()


    def onApplySelectedRemoteChanges(self):
        twParameterEditor = self._getCurrentParameterEditor()
        selectedItems = twParameterEditor.selectedItems()
        for item in selectedItems:
            twParameterEditor.applyRemoteChanges(item)


    def onResetAll(self):
        self._getCurrentParameterEditor().resetAll()


    def onKillInstance(self):
        self.__twNavigation.onKillInstance()


    def onInitDevice(self):
        itemInfo = self.__twNavigation.indexInfo()
        if len(itemInfo) == 0:
            return
        
        serverId = itemInfo.get('serverId')
        classId = itemInfo.get('classId')

        Manager().initDevice(serverId, classId)


    def onDeviceSchemaUpdated(self, key):
        key = str(key)
        self.__internalKeySchemaLoadedMap[key] = False


    def onGlobalAccessLevelChanged(self):
        for index in xrange(self.__swParameterEditor.count()):
            twParameterEditor = self.__swParameterEditor.widget(index)
            twParameterEditor.globalAccessLevelChanged()


    def onFileSaveAs(self):
        self.__twNavigation.onFileSaveAs()


    def onFileOpen(self):
        self.__twNavigation.onFileOpen()


    # virtual function
    def onUndock(self):
        self.__twNavigation.show()


    # virtual function
    def onDock(self):
        self.__twNavigation.hide()

