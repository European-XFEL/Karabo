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
from manager import Manager
from navigationtreeview import NavigationTreeView
from parametertreewidget import ParameterTreeWidget
from projecttreeview import ProjectTreeView

from PyQt4.QtCore import pyqtSignal, Qt, QTimer
from PyQt4.QtGui import (QAction, QHBoxLayout, QIcon, QMenu, QPushButton,
                         QSplitter, QStackedWidget, QToolButton, QVBoxLayout,
                         QWidget)

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


    def __init__(self):
        super(ConfigurationPanel, self).__init__()
        
        self.__toolBar = None
        
        title = "Configuration Editor"
        self.setWindowTitle(title)
        
        # map = { deviceId, timer }
        self.__changingTimerDeviceIdMap = dict()

        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainSplitter = QSplitter(Qt.Vertical)

        # Layout for navigation and project tree
        navSplitter = QSplitter(Qt.Vertical)
        # Navigation tree
        self.twNavigation = NavigationTreeView(self)
        self.twNavigation.signalItemChanged.connect(self.onDeviceItemChanged)
        self.twNavigation.hide()
        navSplitter.addWidget(self.twNavigation)

        # Project tree
        self.twProject = ProjectTreeView(self)
        self.twProject.model().signalItemChanged.connect(self.onDeviceItemChanged)
        self.twProject.hide()
        navSplitter.addWidget(self.twProject)

        navSplitter.setStretchFactor(0, 1)
        navSplitter.setStretchFactor(1, 1)

        # Stacked widget for configuration parameters
        self.__swParameterEditor = QStackedWidget(None)
        # Initial page
        twInitalParameterEditorPage = ParameterTreeWidget()
        twInitalParameterEditorPage.setHeaderLabels(["Parameter", "Value"])
        self.__swParameterEditor.addWidget(twInitalParameterEditorPage)
        self.prevConfiguration = None

        topWidget = QWidget(mainSplitter)

        splitTopPanes = QSplitter(Qt.Horizontal, topWidget)
        splitTopPanes.addWidget(navSplitter)
        splitTopPanes.addWidget(self.__swParameterEditor)

        splitTopPanes.setStretchFactor(0, 1)
        splitTopPanes.setStretchFactor(1, 3)

        vLayout = QVBoxLayout(topWidget)
        vLayout.setContentsMargins(0,0,0,0)
        vLayout.addWidget(splitTopPanes)
        
        Manager().signalNewNavigationItem.connect(self.onNewNavigationItem)
        Manager().signalSelectNewNavigationItem.connect(self.onSelectNewNavigationItem)
        Manager().signalShowConfiguration.connect(self.onShowConfiguration)
        
        Manager().signalConflictStateChanged.connect(self.onConflictStateChanged)
        Manager().signalChangingState.connect(self.onChangingState)
        Manager().signalErrorState.connect(self.onErrorState)
        Manager().signalReset.connect(self.onResetPanel)
        Manager().signalClearParameterPage.connect(self.onClearParameterPage)

        hLayout = QHBoxLayout()
        hLayout.setContentsMargins(0,5,5,5)
        
        text = "Initiate device"
        self.pbInitDevice = QPushButton(QIcon(":start"), text)
        self.pbInitDevice.setToolTip(text)
        self.pbInitDevice.setStatusTip(text)
        self.pbInitDevice.setVisible(False)
        self.pbInitDevice.setMinimumSize(140,32)
        self.pbInitDevice.clicked.connect(self.onInitDevice)
        hLayout.addWidget(self.pbInitDevice)

        text = "Kill instance"
        self.pbKillInstance = QPushButton(QIcon(":delete"), text)
        self.pbKillInstance.setStatusTip(text)
        self.pbKillInstance.setToolTip(text)
        self.pbKillInstance.setVisible(False)
        self.pbKillInstance.setMinimumSize(140,32)
        # use action for button to reuse
        self.acKillInstance = QAction(QIcon(":delete"), text, self)
        self.acKillInstance.setStatusTip(text)
        self.acKillInstance.setToolTip(text)
        self.acKillInstance.triggered.connect(self.onKillInstance)
        self.pbKillInstance.clicked.connect(self.acKillInstance.triggered)
        hLayout.addWidget(self.pbKillInstance)
        
        self.__hasConflicts = False

        text = "Apply all"
        self.pbApplyAll = QPushButton(QIcon(":apply"), text)
        self.pbApplyAll.setToolTip(text)
        self.pbApplyAll.setStatusTip(text)
        self.pbApplyAll.setVisible(False)
        self.pbApplyAll.setEnabled(False)
        self.pbApplyAll.setMinimumSize(140,32)
        # use action for button to reuse
        self.acApplyAll = QAction(QIcon(":apply"), text, self)
        self.acApplyAll.setStatusTip(text)
        self.acApplyAll.setToolTip(text)
        self.acApplyAll.setEnabled(False)
        self.acApplyAll.triggered.connect(self.onApplyAll)
        self.pbApplyAll.clicked.connect(self.acApplyAll.triggered)

        text = "Accept all local changes"
        self.acApplyLocalChanges = QAction(text, self)
        self.acApplyLocalChanges.setStatusTip(text)
        self.acApplyLocalChanges.setToolTip(text)
        self.acApplyLocalChanges.triggered.connect(self.onApplyAll)

        text = "Accept all remote changes"
        self.acApplyRemoteChanges = QAction(text, self)
        self.acApplyRemoteChanges.setStatusTip(text)
        self.acApplyRemoteChanges.setToolTip(text)
        self.acApplyRemoteChanges.triggered.connect(self.onApplyAllRemoteChanges)

        text = "Apply selected local changes"
        self.acApplySelectedChanges = QAction(text, self)
        self.acApplySelectedChanges.setStatusTip(text)
        self.acApplySelectedChanges.setToolTip(text)
        self.acApplySelectedChanges.triggered.connect(self.onApplyAll)

        text = "Accept selected remote changes"
        self.acApplySelectedRemoteChanges = QAction(text, self)
        self.acApplySelectedRemoteChanges.setStatusTip(text)
        self.acApplySelectedRemoteChanges.setToolTip(text)
        self.acApplySelectedRemoteChanges.triggered.connect(self.onApplySelectedRemoteChanges)
        
        # add menu to toolbutton
        self.mApply = QMenu(self.pbApplyAll)
        self.mApply.addAction(self.acApplyLocalChanges)
        self.mApply.addAction(self.acApplyRemoteChanges)
        self.mApply.addSeparator()
        self.mApply.addAction(self.acApplySelectedChanges)
        self.mApply.addAction(self.acApplySelectedRemoteChanges)
        
        hLayout.addWidget(self.pbApplyAll)
        
        text = "Reset all"
        self.pbResetAll = QPushButton(QIcon(":no"), text)
        self.pbResetAll.setToolTip(text)
        self.pbResetAll.setStatusTip(text)
        self.pbResetAll.setVisible(False)
        self.pbResetAll.setEnabled(False)
        self.pbResetAll.setMinimumSize(140,32)
        # use action for button to reuse
        self.acResetAll = QAction(QIcon(":no"), text, self)
        self.acResetAll.setStatusTip(text)
        self.acResetAll.setToolTip(text)
        self.acResetAll.setEnabled(False)
        self.acResetAll.triggered.connect(self.onResetAll)
        self.pbResetAll.clicked.connect(self.acResetAll.triggered)

        hLayout.addWidget(self.pbResetAll)
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
        text = "Open configuration from file (*.xml)"
        self.acOpenFromFile = QAction(QIcon(":open"), text, self)
        self.acOpenFromFile.setStatusTip(text)
        self.acOpenFromFile.setToolTip(text)
        self.acOpenFromFile.triggered.connect(Manager().onOpenFromFile)

        text = "Open configuration from project"
        self.acOpenFromProject = QAction(QIcon(":open"), text, self)
        self.acOpenFromProject.setStatusTip(text)
        self.acOpenFromProject.setToolTip(text)
        self.acOpenFromProject.triggered.connect(Manager().onOpenFromProject)
        
        self.openMenu = QMenu()
        self.openMenu.addAction(self.acOpenFromFile)
        self.openMenu.addAction(self.acOpenFromProject)
        text = "Open configuration"
        self.tbOpenConfig = QToolButton()
        self.tbOpenConfig.setIcon(QIcon(":open"))
        self.tbOpenConfig.setStatusTip(text)
        self.tbOpenConfig.setToolTip(text)
        self.tbOpenConfig.setVisible(False)
        self.tbOpenConfig.setPopupMode(QToolButton.InstantPopup)
        self.tbOpenConfig.setMenu(self.openMenu)

        text = "Save configuration to file (*.xml)"
        self.acSaveToFile = QAction(QIcon(":save-as"), text, self)
        self.acSaveToFile.setStatusTip(text)
        self.acSaveToFile.setToolTip(text)
        self.acSaveToFile.triggered.connect(Manager().onSaveToFile)

        text = "Save configuration to project"
        self.acSaveToProject = QAction(QIcon(":save-as"), text, self)
        self.acSaveToProject.setStatusTip(text)
        self.acSaveToProject.setToolTip(text)
        self.acSaveToProject.triggered.connect(Manager().onSaveToProject)
        
        self.saveMenu = QMenu()
        self.saveMenu.addAction(self.acSaveToFile)
        self.saveMenu.addAction(self.acSaveToProject)
        text = "Save configuration"
        self.tbSaveConfig = QToolButton()
        self.tbSaveConfig.setIcon(QIcon(":save-as"))
        self.tbSaveConfig.setStatusTip(text)
        self.tbSaveConfig.setToolTip(text)
        self.tbSaveConfig.setVisible(False)
        self.tbSaveConfig.setPopupMode(QToolButton.InstantPopup)
        self.tbSaveConfig.setMenu(self.saveMenu)


    def setupToolBars(self, toolBar, parent):
        # Save action to member variables to make setVisible work later
        self.acOpenConfig = toolBar.addWidget(self.tbOpenConfig)
        self.acSaveConfig = toolBar.addWidget(self.tbSaveConfig)


    def updateApplyAllActions(self, configuration):
        twParameterEditor = self.__swParameterEditor.widget(configuration.index)

        nbSelected = twParameterEditor.nbSelectedApplyEnabledItems()
        if (self.pbApplyAll.isEnabled() is True) and (nbSelected > 0):
            if nbSelected == 1:
                text = "Apply selected"
            else:
                text = "Apply ({}) selected".format(nbSelected)
            self.acApplyLocalChanges.setVisible(False)
            self.acApplyRemoteChanges.setVisible(False)
            self.acApplySelectedChanges.setVisible(True)
            self.acApplySelectedRemoteChanges.setVisible(True)
        else:
            text = "Apply all"
            self.acApplyLocalChanges.setVisible(True)
            self.acApplyRemoteChanges.setVisible(True)
            self.acApplySelectedChanges.setVisible(False)
            self.acApplySelectedRemoteChanges.setVisible(False)

        self.pbApplyAll.setText(text)
        self.pbApplyAll.setStatusTip(text)
        self.pbApplyAll.setToolTip(text)

        self.acApplyAll.setText(text)
        self.acApplyAll.setStatusTip(text)
        self.acApplyAll.setToolTip(text)
        
        if self.hasConflicts is True:
            text = "Resolve conflicts"
            self.pbApplyAll.setStatusTip(text)
            self.pbApplyAll.setToolTip(text)
            self.pbApplyAll.setMenu(self.mApply)
            
            self.acApplyAll.setStatusTip(text)
            self.acApplyAll.setToolTip(text)
            self.acApplyAll.setMenu(self.mApply)
        else:
            self.pbApplyAll.setMenu(None)
            self.acApplyAll.setMenu(None)


    def updateResetAllActions(self, configuration):
        twParameterEditor = self.__swParameterEditor.widget(
            configuration.index)

        nbSelected = twParameterEditor.nbSelectedApplyEnabledItems()
        if (self.pbResetAll.isEnabled() is True) and (nbSelected > 0):
            if nbSelected == 1:
                text = "Reset selected"
            else:
                text = "Reset ({}) selected".format(nbSelected)
        else:
            text = "Reset all"
        
        self.pbResetAll.setText(text)
        self.pbResetAll.setStatusTip(text)
        self.pbResetAll.setToolTip(text)

        self.acResetAll.setText(text)
        self.acResetAll.setStatusTip(text)
        self.acResetAll.setToolTip(text)


    def _createNewParameterPage(self, configuration):
        twParameterEditor = ParameterTreeWidget(configuration)
        twParameterEditor.setHeaderLabels([
            "Parameter", "Current value on device", "Value"])
        
        twParameterEditor.addContextMenu(self.openMenu)
        twParameterEditor.addContextMenu(self.saveMenu)
        twParameterEditor.addContextSeparator()
        twParameterEditor.addContextAction(self.acKillInstance)
        twParameterEditor.addContextAction(self.acApplyAll)
        twParameterEditor.addContextAction(self.acResetAll)
        twParameterEditor.signalApplyChanged.connect(self.onApplyChanged)
        twParameterEditor.itemSelectionChanged.connect(self.onSelectionChanged)
        
        if configuration.type == "class" or configuration.type == "projectClass":
            twParameterEditor.hideColumn(1)

        if configuration is not None:
            configuration.fillWidget(twParameterEditor)
        
        index = self.__swParameterEditor.addWidget(twParameterEditor)
        return index


### getter functions ###
    def _hasConflicts(self):
        return self.__hasConflicts
    def _setHasConflicts(self, hasConflicts):
        self.__hasConflicts = hasConflicts
        
        if hasConflicts is True:
            icon = QIcon(":apply-conflict")
            text = "Resolve conflict"
            self.pbApplyAll.setIcon(icon)
            self.pbApplyAll.setStatusTip(text)
            self.pbApplyAll.setToolTip(text)
            self.pbApplyAll.setMenu(self.mApply)

            self.acApplyAll.setIcon(icon)
            self.acApplyAll.setStatusTip(text)
            self.acApplyAll.setToolTip(text)
            self.acApplyAll.setMenu(self.mApply)
            
            self.pbResetAll.setEnabled(False)
            self.acResetAll.setEnabled(False)
        else:
            icon = QIcon(":apply")
            text = "Apply all"
            self.pbApplyAll.setIcon(icon)
            self.pbApplyAll.setStatusTip(text)
            self.pbApplyAll.setToolTip(text)
            self.pbApplyAll.setMenu(None)
            
            self.acApplyAll.setIcon(icon)
            self.acApplyAll.setStatusTip(text)
            self.acApplyAll.setToolTip(text)
            self.acApplyAll.setMenu(None)
        self.acApplyLocalChanges.setVisible(hasConflicts)
        self.acApplyRemoteChanges.setVisible(hasConflicts)

        self.acApplySelectedChanges.setVisible(not hasConflicts)
        self.acApplySelectedRemoteChanges.setVisible(not hasConflicts)
    hasConflicts = property(fget=_hasConflicts, fset=_setHasConflicts)


    def _setApplyAllEnabled(self, configuration, enable):
        self.pbApplyAll.setEnabled(enable)
        self.acApplyAll.setEnabled(enable)
        self.updateApplyAllActions(configuration)


    def _setResetAllEnabled(self, configuration, enable):
        self.pbResetAll.setEnabled(enable)
        self.acResetAll.setEnabled(enable)
        self.updateResetAllActions(configuration)


    def _setParameterEditorIndex(self, index):
        self.__swParameterEditor.blockSignals(True)
        self.__swParameterEditor.setCurrentIndex(index)
        self.__swParameterEditor.blockSignals(False)

        show = index != 0
        self.acOpenConfig.setVisible(show)
        self.acSaveConfig.setVisible(show)


    def _hideAllButtons(self):
        # Hide buttons and actions
        self.pbInitDevice.setVisible(False)
        
        self.pbKillInstance.setVisible(False)
        self.acKillInstance.setVisible(False)
        self.pbApplyAll.setVisible(False)
        self.acApplyAll.setVisible(False)
        self.pbResetAll.setVisible(False)
        self.acResetAll.setVisible(False)


    def _getCurrentParameterEditor(self):
        return self.__swParameterEditor.currentWidget()
    
    
    def _getParameterEditorByPath(self, path):
        """
        Returns the parameterEditor-Treewidget with the given \path.
        If not found, return None.
        """
        for index in range(self.__swParameterEditor.count()):
            twParameterEditor = self.__swParameterEditor.widget(index)
            if twParameterEditor.conf is None:
                continue
            if path.startswith(twParameterEditor.conf.id):
                return twParameterEditor
        return None


    def _updateButtonsVisibility(self, visible):
        self.pbInitDevice.setVisible(visible)
        
        self.pbKillInstance.setVisible(not visible)
        self.pbApplyAll.setVisible(not visible)
        self.pbResetAll.setVisible(not visible)
        
        self.acKillInstance.setVisible(not visible)
        self.acApplyAll.setVisible(not visible)
        self.acResetAll.setVisible(not visible)
    updateButtonsVisibility = property(fset=_updateButtonsVisibility)


    def showParameterPage(self, configuration):
        """ Show the parameters for configuration """
        if configuration is None:
            self._setParameterEditorIndex(0)
        else:
            if not hasattr(configuration, 'index'):
                configuration.index = self._createNewParameterPage(configuration)
            self._setParameterEditorIndex(configuration.index)
        
        if configuration not in (None, self.prevConfiguration) and (configuration.type == "device"):
            configuration.addVisible()
        
        self.prevConfiguration = configuration


    def _clearParameterEditorPage(self, twParameterEditor):
        """
        The \twParameterEditor is remove from StackedWidget and all registered
        components get unregistered.
        """
        if twParameterEditor is None:
            return

        # Clear page
        twParameterEditor.clear()
        # Remove widget completely
        #self.__swParameterEditor.removeWidget(twParameterEditor)
        self._setParameterEditorIndex(0)
        self._hideAllButtons()


### slots ###
    def onClearParameterPage(self, removePath, selectPath):
        """
        The parameter page with the following \path have to be removed.
        """
        for i in xrange(self.__swParameterEditor.count()):
            twParameterPage = self.__swParameterEditor.widget(i)
            if twParameterPage.conf is None: continue
            if removePath == twParameterPage.conf.id:
                self._clearParameterEditorPage(twParameterPage)
                break


    def onResetPanel(self):
        """
        This slot is called when the configurator needs a reset which means all
        parameter editor pages need to be cleaned and removed.
        """
        for i in xrange(self.__swParameterEditor.count()):
            self._clearParameterEditorPage(self.__swParameterEditor.widget(i))


    def onNewNavigationItem(self, itemInfo):
        # itemInfo: id, name, type, (status), (refType), (refId), (schema)
        self.twNavigation.createNewItem(itemInfo)


    def onSelectNewNavigationItem(self, devicePath):
        self.twNavigation.selectItem(devicePath)


    def onShowConfiguration(self, configuration):
        if hasattr(configuration, 'index'):
            twParameterEditor = self.__swParameterEditor.widget(
                configuration.index)
            twParameterEditor.clear()
            configuration.fillWidget(twParameterEditor)
        else:
            configuration.index = self._createNewParameterPage(configuration)
        

    def onDeviceItemChanged(self, configuration):
        self.updateButtonsVisibility = configuration is not None and \
                                       (configuration.type == 'class' or \
                                        configuration.type == 'projectClass')

        if self.prevConfiguration not in (None, configuration) and (self.prevConfiguration.type == "device"):
            self.prevConfiguration.removeVisible()
        
        self.showParameterPage(configuration)


    def onConflictStateChanged(self, deviceId, hasConflict):
        conf = Manager().deviceData.get(deviceId)
        if conf is None:
            return

        if conf.parameterEditor is None:
            return

        result = conf.parameterEditor.checkApplyButtonsEnabled()
        self.pbApplyAll.setEnabled(result[0])
        self.pbResetAll.setEnabled(result[0])
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
        self._setApplyAllEnabled(box.configuration, enable)
        self._setResetAllEnabled(box.configuration, enable)
        self.hasConflicts = hasConflicts


    def onSelectionChanged(self):
        """ Update the apply and reset buttons """
        conf = self.sender().conf
        self.updateApplyAllActions(conf)
        self.updateResetAllActions(conf)


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
        # Check whether an index of the Navigation or ProjectPanel is selected
        if self.twNavigation.currentIndex().isValid():
            self.twNavigation.onKillInstance()
        elif self.twProject.currentIndex().isValid():
            self.twProject.onKillDevice()


    def onInitDevice(self):
        # Check whether an index of the Navigation or ProjectPanel is selected
        if self.twNavigation.currentIndex().isValid():
            indexInfo = self.twNavigation.indexInfo()
        elif self.twProject.currentIndex().isValid():
            indexInfo = self.twProject.indexInfo()
        else:
            print "No device for initiation selected."

        if len(indexInfo) == 0:
            return
        
        serverId = indexInfo.get('serverId')
        classId = indexInfo.get('classId')
        config = indexInfo.get('config')

        Manager().initDevice(serverId, classId, config)


    def onGlobalAccessLevelChanged(self):
        for index in xrange(self.__swParameterEditor.count()):
            twParameterEditor = self.__swParameterEditor.widget(index)
            twParameterEditor.globalAccessLevelChanged()


    def onSaveToFile(self):
        self.twNavigation.onSaveToFile()


    def onSaveToProject(self):
        self.twNavigation.onSaveToProject()


    def onOpenFromFile(self):
        self.twNavigation.onOpenFromFile()


    def onOpenFromProject(self):
        self.twNavigation.onOpenFromProject()


    # virtual function
    def onUndock(self):
        self.twNavigation.show()
        self.twProject.show()


    # virtual function
    def onDock(self):
        self.twNavigation.hide()
        self.twProject.hide()

