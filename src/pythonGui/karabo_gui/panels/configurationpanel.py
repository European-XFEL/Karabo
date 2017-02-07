#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import Qt, QTimer
from PyQt4.QtGui import (QAction, QHBoxLayout, QMenu, QPalette, QPushButton,
                         QSplitter, QStackedWidget, QToolButton, QVBoxLayout,
                         QWidget)

from karabo_gui.docktabwindow import Dockable
from karabo_gui.events import (
    register_for_broadcasts, KaraboBroadcastEvent, KaraboEventSender)
import karabo_gui.icons as icons
from karabo_gui.navigationtreeview import NavigationTreeView
from karabo_gui.parametertreewidget import ParameterTreeWidget
from karabo_gui.project.view import ProjectView
from karabo_gui.singletons.api import get_manager
from karabo_gui.util import (
    get_spin_widget, loadConfigurationFromFile, saveConfigurationToFile
)


class ConfigurationPanel(Dockable, QWidget):
    def __init__(self, parent=None):
        super(ConfigurationPanel, self).__init__(parent=parent)

        # Register for broadcast events.
        # This object lives as long as the app. No need to unregister.
        register_for_broadcasts(self)

        self.__toolBar = None

        title = "Configuration Editor"
        self.setWindowTitle(title)

        # map = { deviceId, timer }
        self.__changingTimerDeviceIdMap = dict()

        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5, 5, 5, 5)

        # Layout for navigation and project tree
        self.navSplitter = QSplitter(Qt.Vertical)
        # Navigation tree
        self.twNavigation = NavigationTreeView(self)
        self.twNavigation.model().signalItemChanged.connect(
            self.onDeviceItemChanged)
        self.twNavigation.hide()
        self.navSplitter.addWidget(self.twNavigation)

        # Project tree
        self.twProject = ProjectView(parent=self)
        self.twProject.hide()
        self.navSplitter.addWidget(self.twProject)

        self.navSplitter.setStretchFactor(0, 1)
        self.navSplitter.setStretchFactor(1, 1)

        # The navSplitter should only show when the panel is undocked.
        self.navSplitter.hide()

        # Stacked widget for configuration parameters
        self.__swParameterEditor = QStackedWidget(None)
        # Initial page
        twInitalParameterEditorPage = ParameterTreeWidget()
        twInitalParameterEditorPage.setHeaderLabels(["Parameter", "Value"])
        self.__swParameterEditor.addWidget(twInitalParameterEditorPage)

        # Wait page
        wait_widget = get_spin_widget(parent=self)
        wait_widget.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        wait_widget.setAutoFillBackground(True)
        wait_widget.setBackgroundRole(QPalette.Base)
        self.__swParameterEditor.addWidget(wait_widget)

        self.prevConfiguration = None
        self._awaitingSchema = None

        topWidget = QWidget(self)

        splitTopPanes = QSplitter(Qt.Horizontal, topWidget)
        splitTopPanes.addWidget(self.navSplitter)
        splitTopPanes.addWidget(self.__swParameterEditor)

        splitTopPanes.setStretchFactor(0, 1)
        splitTopPanes.setStretchFactor(1, 3)

        vLayout = QVBoxLayout(topWidget)
        vLayout.setContentsMargins(0, 0, 0, 0)
        vLayout.addWidget(splitTopPanes)

        hLayout = QHBoxLayout()
        hLayout.setContentsMargins(0, 5, 5, 5)

        text = "Instantiate device"
        self.pbInitDevice = QPushButton(icons.start, text)
        self.pbInitDevice.setToolTip(text)
        self.pbInitDevice.setStatusTip(text)
        self.pbInitDevice.setVisible(False)
        self.pbInitDevice.setMinimumSize(140, 32)
        self.pbInitDevice.clicked.connect(self.onInitDevice)
        hLayout.addWidget(self.pbInitDevice)

        text = "Shutdown instance"
        self.pbKillInstance = QPushButton(icons.kill, text)
        self.pbKillInstance.setStatusTip(text)
        self.pbKillInstance.setToolTip(text)
        self.pbKillInstance.setVisible(False)
        self.pbKillInstance.setMinimumSize(140, 32)
        # use action for button to reuse
        self.acKillInstance = QAction(icons.kill, text, self)
        self.acKillInstance.setStatusTip(text)
        self.acKillInstance.setToolTip(text)
        self.acKillInstance.triggered.connect(self.onKillInstance)
        self.pbKillInstance.clicked.connect(self.acKillInstance.triggered)
        hLayout.addWidget(self.pbKillInstance)

        self.__hasConflicts = False

        text = "Apply all"
        description = "Apply all property changes in one go"
        self.pbApplyAll = QPushButton(icons.apply, text)
        self.pbApplyAll.setToolTip(description)
        self.pbApplyAll.setStatusTip(description)
        self.pbApplyAll.setVisible(False)
        self.pbApplyAll.setEnabled(False)
        self.pbApplyAll.setMinimumSize(140, 32)
        # use action for button to reuse
        self.acApplyAll = QAction(icons.apply, text, self)
        self.acApplyAll.setStatusTip(text)
        self.acApplyAll.setToolTip(text)
        self.acApplyAll.setEnabled(False)
        self.acApplyAll.triggered.connect(self.onApplyAll)
        self.pbApplyAll.clicked.connect(self.acApplyAll.triggered)

        text = "Apply all my changes"
        self.acApplyLocalChanges = QAction(text, self)
        self.acApplyLocalChanges.setStatusTip(text)
        self.acApplyLocalChanges.setToolTip(text)
        self.acApplyLocalChanges.triggered.connect(self.onApplyAll)

        text = "Adjust to current values on device"
        self.acApplyRemoteChanges = QAction(text, self)
        self.acApplyRemoteChanges.setStatusTip(text)
        self.acApplyRemoteChanges.setToolTip(text)
        self.acApplyRemoteChanges.triggered.connect(
            self.onApplyAllRemoteChanges)

        text = "Apply selected local changes"
        self.acApplySelectedChanges = QAction(text, self)
        self.acApplySelectedChanges.setStatusTip(text)
        self.acApplySelectedChanges.setToolTip(text)
        self.acApplySelectedChanges.triggered.connect(self.onApplyAll)

        text = "Accept selected remote changes"
        self.acApplySelectedRemoteChanges = QAction(text, self)
        self.acApplySelectedRemoteChanges.setStatusTip(text)
        self.acApplySelectedRemoteChanges.setToolTip(text)
        self.acApplySelectedRemoteChanges.triggered.connect(
            self.onApplySelectedRemoteChanges)

        # add menu to toolbutton
        self.mApply = QMenu(self.pbApplyAll)
        self.mApply.addAction(self.acApplyLocalChanges)
        self.mApply.addAction(self.acApplyRemoteChanges)
        self.mApply.addSeparator()
        self.mApply.addAction(self.acApplySelectedChanges)
        self.mApply.addAction(self.acApplySelectedRemoteChanges)

        hLayout.addWidget(self.pbApplyAll)

        text = "Decline all"
        decription = ("Decline all property changes and reset them to value "
                      "on device")
        self.pbResetAll = QPushButton(icons.no, text)
        self.pbResetAll.setToolTip(decription)
        self.pbResetAll.setStatusTip(decription)
        self.pbResetAll.setVisible(False)
        self.pbResetAll.setEnabled(False)
        self.pbResetAll.setMinimumSize(140, 32)
        # use action for button to reuse
        self.acResetAll = QAction(icons.no, text, self)
        self.acResetAll.setStatusTip(text)
        self.acResetAll.setToolTip(text)
        self.acResetAll.setEnabled(False)
        self.acResetAll.triggered.connect(self.onResetAll)
        self.pbResetAll.clicked.connect(self.acResetAll.triggered)

        hLayout.addWidget(self.pbResetAll)
        hLayout.addStretch()
        vLayout.addLayout(hLayout)

        mainLayout.addWidget(topWidget)

        self.setupActions()
        self.setLayout(mainLayout)

    def eventFilter(self, obj, event):
        """ Router for incoming broadcasts
        """
        if isinstance(event, KaraboBroadcastEvent):
            data = event.data
            if event.sender is KaraboEventSender.ShowConfiguration:
                configuration = data.get('configuration')
                self.showConfiguration(configuration)
            elif event.sender is KaraboEventSender.UpdateDeviceConfigurator:
                configuration = data.get('configuration')
                self.updateDisplayedConfiguration(configuration)
            elif event.sender is KaraboEventSender.ClearConfigurator:
                deviceId = data.get('deviceId', '')
                self.removeDepartedConfiguration(deviceId)
            elif event.sender is KaraboEventSender.ShowNavigationItem:
                device_path = data.get('device_path')
                self.onSelectNewNavigationItem(device_path)
            elif event.sender is KaraboEventSender.DeviceStateChanged:
                configuration = data.get('configuration')
                is_changing = data.get('is_changing')
                self.onChangingState(configuration, is_changing)
            elif event.sender is KaraboEventSender.DeviceErrorChanged:
                configuration = data.get('configuration')
                is_changing = data.get('is_changing')
                self.onErrorState(configuration, is_changing)
            elif event.sender is KaraboEventSender.NetworkConnectStatus:
                if not data['status']:
                    self._resetPanel()
            return False
        return super(ConfigurationPanel, self).eventFilter(obj, event)

    def setupActions(self):
        text = "Open configuration from file (*.xml)"
        self.acOpenFromFile = QAction(icons.load, text, self)
        self.acOpenFromFile.setStatusTip(text)
        self.acOpenFromFile.setToolTip(text)
        self.acOpenFromFile.triggered.connect(self.onOpenFromFile)

        self.openMenu = QMenu()
        self.openMenu.addAction(self.acOpenFromFile)
        text = "Open configuration"
        self.tbOpenConfig = QToolButton()
        self.tbOpenConfig.setIcon(icons.load)
        self.tbOpenConfig.setStatusTip(text)
        self.tbOpenConfig.setToolTip(text)
        self.tbOpenConfig.setVisible(False)
        self.tbOpenConfig.setPopupMode(QToolButton.InstantPopup)
        self.tbOpenConfig.setMenu(self.openMenu)

        text = "Save configuration to file (*.xml)"
        self.acSaveToFile = QAction(icons.saveAs, text, self)
        self.acSaveToFile.setStatusTip(text)
        self.acSaveToFile.setToolTip(text)
        self.acSaveToFile.triggered.connect(self.onSaveToFile)

        self.saveMenu = QMenu()
        self.saveMenu.addAction(self.acSaveToFile)
        text = "Save configuration"
        self.tbSaveConfig = QToolButton()
        self.tbSaveConfig.setIcon(icons.saveAs)
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
        index = configuration.index
        twParameterEditor = self.__swParameterEditor.widget(index)

        nbSelected = twParameterEditor.nbSelectedApplyEnabledItems()
        if self.pbApplyAll.isEnabled() is True and nbSelected > 0:
            if nbSelected == 1:
                text = "Apply selected"
            else:
                text = "Apply ({}) selected".format(nbSelected)

            description = "Apply selected property changes in one go"
            self.acApplyLocalChanges.setVisible(False)
            self.acApplyRemoteChanges.setVisible(False)
            self.acApplySelectedChanges.setVisible(True)
            self.acApplySelectedRemoteChanges.setVisible(True)
        else:
            text = "Apply all"
            description = "Apply all property changes in one go"
            self.acApplyLocalChanges.setVisible(True)
            self.acApplyRemoteChanges.setVisible(True)
            self.acApplySelectedChanges.setVisible(False)
            self.acApplySelectedRemoteChanges.setVisible(False)

        self.pbApplyAll.setText(text)
        self.pbApplyAll.setStatusTip(description)
        self.pbApplyAll.setToolTip(description)

        self.acApplyAll.setText(text)
        self.acApplyAll.setStatusTip(description)
        self.acApplyAll.setToolTip(description)

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
        index = configuration.index
        twParameterEditor = self.__swParameterEditor.widget(index)

        nbSelected = twParameterEditor.nbSelectedApplyEnabledItems()
        if (self.pbResetAll.isEnabled() is True) and (nbSelected > 0):
            if nbSelected == 1:
                text = "Decline selected"
            else:
                text = "Decline ({}) selected".format(nbSelected)
            description = ("Decline all selected property changes and reset "
                           "them to value on device")
        else:
            text = "Decline all"
            description = ("Decline all property changes and reset them to "
                           "value on device")

        self.pbResetAll.setText(text)
        self.pbResetAll.setStatusTip(description)
        self.pbResetAll.setToolTip(description)

        self.acResetAll.setText(description)
        self.acResetAll.setStatusTip(description)
        self.acResetAll.setToolTip(text)

    def removeDepartedConfiguration(self, deviceId):
        """Clear the configuration panel when a device goes away
        """
        config = self.prevConfiguration
        if config is None:
            return

        acceptable_types = ('class', 'projectClass', 'deviceGroupClass')
        if deviceId == config.id and config.type not in acceptable_types:
            self.showConfiguration(None)

    def showConfiguration(self, configuration):
        """Show a Configuration object in the panel
        """
        if configuration is None:
            # There is no configuration to show
            index = 0
        elif configuration.descriptor is None:
            # The configuration is not ready to be shown (no Schema)
            index = 1
        elif configuration.index is None:
            # The configuration has never been shown before
            configuration.index = self._createNewParameterPage(configuration)
            index = configuration.index
        elif configuration.parameterEditor is None:
            # The configuration needs its editor rebuilt
            index = configuration.index
            twParameterEditor = self.__swParameterEditor.widget(index)
            twParameterEditor.clear()
            configuration.fillWidget(twParameterEditor)
        else:
            # Everything is ready to go!
            index = configuration.index

        # This is the configuration we're viewing now
        self._setParameterEditorIndex(index)
        self._setConfiguration(configuration)

    def updateDisplayedConfiguration(self, configuration):
        if self.prevConfiguration is None:
            return

        currentDeviceId = self.prevConfiguration.id
        if configuration.id == currentDeviceId:
            self.showConfiguration(configuration)

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

        if configuration.type in ("class", "projectClass", "deviceGroupClass"):
            twParameterEditor.hideColumn(1)

        if configuration is not None:
            configuration.fillWidget(twParameterEditor)

        index = self.__swParameterEditor.addWidget(twParameterEditor)
        return index

    # ----------------------------------------------------------------------
    # getter functions

    @property
    def hasConflicts(self):
        return self.__hasConflicts

    @hasConflicts.setter
    def hasConflicts(self, hasConflicts):
        self.__hasConflicts = hasConflicts

        if hasConflicts is True:
            icon = icons.applyConflict
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
            icon = icons.apply
            text = "Apply all"
            description = "Apply all property changes in one go"
            self.pbApplyAll.setIcon(icon)
            self.pbApplyAll.setStatusTip(description)
            self.pbApplyAll.setToolTip(description)
            self.pbApplyAll.setMenu(None)

            self.acApplyAll.setIcon(icon)
            self.acApplyAll.setStatusTip(text)
            self.acApplyAll.setToolTip(text)
            self.acApplyAll.setMenu(None)
        self.acApplyLocalChanges.setVisible(hasConflicts)
        self.acApplyRemoteChanges.setVisible(hasConflicts)

        self.acApplySelectedChanges.setVisible(not hasConflicts)
        self.acApplySelectedRemoteChanges.setVisible(not hasConflicts)

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

        show = (index != 0)
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
        """Returns the parameterEditor-Treewidget with the given ``path``.
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

    def _removeParameterEditorPage(self, twParameterEditor):
        """The ``twParameterEditor`` is remove from StackedWidget and all
        registered components get unregistered.
        """
        if twParameterEditor is None:
            return

        # Clear page
        twParameterEditor.clear()
        # Remove widget completely
        self.__swParameterEditor.removeWidget(twParameterEditor)
        self._showEmptyPage()

    def _showEmptyPage(self):
        """This function shows the default page of the parameter editor
        stacked widget and hides all buttons.
        """
        self._setParameterEditorIndex(0)
        self._hideAllButtons()

    def _resetPanel(self):
        """This is called when the configurator needs a reset which means all
        parameter editor pages need to be cleaned and removed.
        """
        self.prevConfiguration = None

        # Do not remove the first two widgets (empty page and waiting page)
        while self.__swParameterEditor.count() > 2:
            index = self.__swParameterEditor.count()-1
            page = self.__swParameterEditor.widget(index)
            self._removeParameterEditorPage(page)

    def _setConfiguration(self, configuration):
        """Adapt to the Configuration which is currently showing
        """
        # Update buttons
        if configuration is None or configuration.descriptor is None:
            self._hideAllButtons()
        else:
            vis_types = ('class', 'projectClass')
            self.updateButtonsVisibility = (configuration is not None and
                                            configuration.type in vis_types)

        # Toggle device updates
        visible_types = ('device', 'deviceGroup')
        if (self.prevConfiguration not in (None, configuration) and
                self.prevConfiguration.type in visible_types):
            self.prevConfiguration.removeVisible()

        if (configuration not in (None, self.prevConfiguration) and
                configuration.type in visible_types):
            configuration.addVisible()

        # Cancel notification of arriving schema
        if self._awaitingSchema is not None:
            conf = self._awaitingSchema
            conf.signalNewDescriptor.disconnect(self.onSchemaArrival)

        # Handle configurations which await a schema
        if configuration is not None and configuration.descriptor is None:
            self._awaitingSchema = configuration
            configuration.signalNewDescriptor.connect(self.onSchemaArrival)

        # Finally, set prevConfiguration
        self.prevConfiguration = configuration

    # -----------------------------------------------------------------------
    # slots

    def onSelectNewNavigationItem(self, devicePath):
        self.twNavigation.selectItem(devicePath)

    def onDeviceItemChanged(self, item_type, configuration):
        # Navigation view change. Show the configuration
        self.showConfiguration(configuration)

    def onSchemaArrival(self, configuration):
        # A schema we wanted arrived
        assert configuration is self._awaitingSchema
        self._awaitingSchema = None
        configuration.signalNewDescriptor.disconnect(self.onSchemaArrival)

        if self.prevConfiguration is configuration:
            self.showConfiguration(configuration)

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

            if conf.index is not None:
                parameterEditor = self.__swParameterEditor.widget(conf.index)
                parameterEditor.setReadOnly(False)

    def onErrorState(self, conf, inErrorState):
        if conf.index is not None:
            parameterEditor = self.__swParameterEditor.widget(conf.index)
            parameterEditor.setErrorState(inErrorState)

    def onTimeOut(self):
        timer = self.sender()
        timer.stop()

        # Check path against path of current parameter editor
        mapValues = list(self.__changingTimerDeviceIdMap.values())
        for i in range(len(mapValues)):
            if timer == mapValues[i]:
                path = list(self.__changingTimerDeviceIdMap.keys())[i]

                parameterEditor = self._getParameterEditorByPath(path)
                if parameterEditor:
                    parameterEditor.setReadOnly(True)
                break

    def onApplyChanged(self, box, enable, hasConflicts=False):
        """Called when apply button of ParameterPage changed
        """
        self._setApplyAllEnabled(box.configuration, enable)
        self._setResetAllEnabled(box.configuration, enable)
        self.hasConflicts = hasConflicts

    def onSelectionChanged(self):
        """Update the apply and reset buttons
        """
        conf = self.sender().conf
        self.updateApplyAllActions(conf)
        self.updateResetAllActions(conf)

    def onOpenFromFile(self):
        if self.prevConfiguration is not None:
            loadConfigurationFromFile(self.prevConfiguration)

    def onSaveToFile(self):
        if self.prevConfiguration is not None:
            saveConfigurationToFile(self.prevConfiguration)

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
        self.prevConfiguration.shutdown()

    def onInitDevice(self):
        conf_type = self.prevConfiguration.type
        if conf_type == 'projectClass':
            project_device = self.prevConfiguration
            hsh, _ = project_device.toHash()  # Ignore returned attributes
            get_manager().initDevice(project_device.serverId,
                                     project_device.classId,
                                     project_device.id,
                                     hsh)
            return

        if self.twNavigation.currentIndex().isValid():
            indexInfo = self.twNavigation.indexInfo()
        else:
            indexInfo = {}
            print("No device for initiation selected.")

        if len(indexInfo) == 0:
            return

        serverId = indexInfo.get('serverId')
        classId = indexInfo.get('classId')
        deviceId = indexInfo.get('deviceId')
        config = indexInfo.get('config')
        get_manager().initDevice(serverId, classId, deviceId, config)

    def onGlobalAccessLevelChanged(self):
        for index in range(self.__swParameterEditor.count()):
            twParameterEditor = self.__swParameterEditor.widget(index)
            if isinstance(twParameterEditor, ParameterTreeWidget):
                twParameterEditor.globalAccessLevelChanged()

    def undock(self, div):
        self.navSplitter.show()
        self.twNavigation.show()
        self.twProject.show()

    def dock(self, div):
        self.navSplitter.hide()
        self.twNavigation.hide()
        self.twProject.hide()
