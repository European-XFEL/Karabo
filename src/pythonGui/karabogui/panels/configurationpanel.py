#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import Qt
from PyQt4.QtGui import (
    QAction, QHBoxLayout, QMenu, QPalette, QPushButton, QScrollArea,
    QStackedWidget, QToolButton, QVBoxLayout, QWidget)

from karabogui import icons
from karabogui.binding.api import (
    DeviceProxy, ProjectDeviceProxy, extract_configuration)
from karabogui.configurator.api import ConfigurationTreeView
from karabogui.events import KaraboEventSender, register_for_broadcasts
from karabogui.singletons.api import get_manager
from karabogui.util import (
    get_spin_widget, loadConfigurationFromFile, saveConfigurationToFile
)
from karabogui.widgets.toolbar import ToolBar
from .base import BasePanelWidget

BLANK_PAGE = 0
WAITING_PAGE = 1
CONFIGURATION_PAGE = 2


class ConfigurationPanel(BasePanelWidget):
    def __init__(self):
        super(ConfigurationPanel, self).__init__("Configuration Editor")

        self.prevConfiguration = None
        self._awaitingSchema = None
        self.__hasConflicts = False

        # Register for broadcast events.
        # This object lives as long as the app. No need to unregister.
        register_for_broadcasts(self)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(self)
        mainLayout = QVBoxLayout(widget)
        mainLayout.setContentsMargins(0, 0, 0, 0)

        # Stacked widget for configuration parameters
        self.__swParameterEditor = QStackedWidget(widget)
        # BLANK_PAGE
        self.__swParameterEditor.addWidget(ConfigurationTreeView(widget))

        # WAITING_PAGE
        wait_widget = get_spin_widget(parent=widget)
        wait_widget.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        wait_widget.setAutoFillBackground(True)
        wait_widget.setBackgroundRole(QPalette.Base)
        self.__swParameterEditor.addWidget(wait_widget)

        # CONFIGURATION_PAGE
        self.__swParameterEditor.addWidget(ConfigurationTreeView(widget))

        hLayout = QHBoxLayout()
        hLayout.setContentsMargins(0, 5, 5, 5)

        text = "Instantiate device"
        self.pbInitDevice = QPushButton(icons.start, text)
        self.pbInitDevice.setToolTip(text)
        self.pbInitDevice.setStatusTip(text)
        self.pbInitDevice.setVisible(False)
        self.pbInitDevice.setMinimumWidth(140)
        self.pbInitDevice.clicked.connect(self.onInitDevice)
        hLayout.addWidget(self.pbInitDevice)

        text = "Shutdown instance"
        self.pbKillInstance = QPushButton(icons.kill, text)
        self.pbKillInstance.setStatusTip(text)
        self.pbKillInstance.setToolTip(text)
        self.pbKillInstance.setVisible(False)
        self.pbKillInstance.setMinimumWidth(140)
        # use action for button to reuse
        self.acKillInstance = QAction(icons.kill, text, widget)
        self.acKillInstance.setStatusTip(text)
        self.acKillInstance.setToolTip(text)
        self.acKillInstance.triggered.connect(self.onKillInstance)
        self.pbKillInstance.clicked.connect(self.acKillInstance.triggered)
        hLayout.addWidget(self.pbKillInstance)

        text = "Apply all"
        description = "Apply all property changes in one go"
        self.pbApplyAll = QPushButton(icons.apply, text)
        self.pbApplyAll.setToolTip(description)
        self.pbApplyAll.setStatusTip(description)
        self.pbApplyAll.setVisible(False)
        self.pbApplyAll.setEnabled(False)
        self.pbApplyAll.setMinimumWidth(140)
        # use action for button to reuse
        self.acApplyAll = QAction(icons.apply, text, widget)
        self.acApplyAll.setStatusTip(text)
        self.acApplyAll.setToolTip(text)
        self.acApplyAll.setEnabled(False)
        self.acApplyAll.triggered.connect(self.onApplyAll)
        self.pbApplyAll.clicked.connect(self.acApplyAll.triggered)

        text = "Apply all my changes"
        self.acApplyLocalChanges = QAction(text, widget)
        self.acApplyLocalChanges.setStatusTip(text)
        self.acApplyLocalChanges.setToolTip(text)
        self.acApplyLocalChanges.triggered.connect(self.onApplyAll)

        text = "Adjust to current values on device"
        self.acApplyRemoteChanges = QAction(text, widget)
        self.acApplyRemoteChanges.setStatusTip(text)
        self.acApplyRemoteChanges.setToolTip(text)
        self.acApplyRemoteChanges.triggered.connect(
            self.onApplyAllRemoteChanges)

        text = "Apply selected local changes"
        self.acApplySelectedChanges = QAction(text, widget)
        self.acApplySelectedChanges.setStatusTip(text)
        self.acApplySelectedChanges.setToolTip(text)
        self.acApplySelectedChanges.triggered.connect(self.onApplyAll)

        text = "Accept selected remote changes"
        self.acApplySelectedRemoteChanges = QAction(text, widget)
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
        self.pbResetAll.setMinimumWidth(140)
        # use action for button to reuse
        self.acResetAll = QAction(icons.no, text, widget)
        self.acResetAll.setStatusTip(text)
        self.acResetAll.setToolTip(text)
        self.acResetAll.setEnabled(False)
        self.acResetAll.triggered.connect(self.onResetAll)
        self.pbResetAll.clicked.connect(self.acResetAll.triggered)

        hLayout.addWidget(self.pbResetAll)
        hLayout.addStretch()

        self.rightScrollArea = QScrollArea(widget)
        self.rightScrollArea.setWidgetResizable(True)
        rightWidget = QWidget(widget)
        vLayout = QVBoxLayout(rightWidget)
        vLayout.setContentsMargins(0, 0, 0, 0)
        vLayout.addWidget(self.__swParameterEditor)
        vLayout.addLayout(hLayout)
        self.rightScrollArea.setWidget(rightWidget)
        mainLayout.addWidget(self.rightScrollArea)

        return widget

    def toolbars(self):
        """This should create and return one or more `ToolBar` instances needed
        by this panel.
        """
        toolbar = ToolBar(parent=self)

        text = "Open configuration from file (*.xml)"
        self.acOpenFromFile = QAction(icons.load, text, toolbar)
        self.acOpenFromFile.setStatusTip(text)
        self.acOpenFromFile.setToolTip(text)
        self.acOpenFromFile.triggered.connect(self.onOpenFromFile)

        self.openMenu = QMenu(toolbar)
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
        self.acSaveToFile = QAction(icons.saveAs, text, toolbar)
        self.acSaveToFile.setStatusTip(text)
        self.acSaveToFile.setToolTip(text)
        self.acSaveToFile.triggered.connect(self.onSaveToFile)

        self.saveMenu = QMenu(toolbar)
        self.saveMenu.addAction(self.acSaveToFile)
        text = "Save configuration"
        self.tbSaveConfig = QToolButton()
        self.tbSaveConfig.setIcon(icons.saveAs)
        self.tbSaveConfig.setStatusTip(text)
        self.tbSaveConfig.setToolTip(text)
        self.tbSaveConfig.setVisible(False)
        self.tbSaveConfig.setPopupMode(QToolButton.InstantPopup)
        self.tbSaveConfig.setMenu(self.saveMenu)

        self.acOpenConfig = toolbar.addWidget(self.tbOpenConfig)
        self.acSaveConfig = toolbar.addWidget(self.tbSaveConfig)

        return [toolbar]

    def karaboBroadcastEvent(self, event):
        """ Router for incoming broadcasts
        """
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
        elif event.sender is KaraboEventSender.NetworkConnectStatus:
            connected = data['status']
            if not connected:
                self._resetPanel()

        return False

    def updateApplyAllActions(self, configuration):
        editor = self.__swParameterEditor.widget(CONFIGURATION_PAGE)

        nbSelected = editor.nbSelectedApplyEnabledItems()
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
        editor = self.__swParameterEditor.widget(CONFIGURATION_PAGE)

        nbSelected = editor.nbSelectedApplyEnabledItems()
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

    def removeDepartedConfiguration(self, device_id):
        """Clear the configuration panel when a device goes away
        """
        config = self.prevConfiguration
        if config is None:
            return

        if isinstance(config, DeviceProxy) and device_id == config.device_id:
            self.showConfiguration(None)

    def showConfiguration(self, configuration):
        """Show a Configuration object in the panel
        """
        if configuration is None:
            # There is no configuration to show
            index = BLANK_PAGE
        elif len(configuration.binding.value) == 0:
            # The configuration is not ready to be shown (no Schema)
            index = WAITING_PAGE
        else:
            # The configuration is OK to show
            index = CONFIGURATION_PAGE
            tree_widget = self.__swParameterEditor.widget(index)
            self._set_tree_widget_configuration(tree_widget, configuration)

        # This is the configuration we're viewing now
        self._setParameterEditorIndex(index)
        self._setConfiguration(configuration)

    def updateDisplayedConfiguration(self, configuration):
        if self.prevConfiguration is None:
            return

        def _get_ids(conf):
            class_id = conf.binding.class_id
            server_id = conf.server_id
            device_id = ('' if not hasattr(conf, 'device_id')
                         else conf.device_id)
            return class_id, server_id, device_id

        previous = self.prevConfiguration
        cur_class_id, cur_server_id, cur_dev_id = _get_ids(previous)
        class_id, server_id, dev_id = _get_ids(configuration)
        if (server_id == cur_server_id and class_id == cur_class_id and
                dev_id == cur_dev_id):
            # FIXME: Maybe what really needs to be done here is to just
            # update the view?
            # Maybe this method is obsolete?
            self.showConfiguration(configuration)

    def _set_tree_widget_configuration(self, tree_widget, configuration):
        tree_widget.assign_proxy(configuration)
        tree_widget.resizeColumnToContents(0)
        tree_widget.resizeColumnToContents(1)

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

    def _updateButtonsVisibility(self, visible):
        self.pbInitDevice.setVisible(visible)

        self.pbKillInstance.setVisible(not visible)
        self.pbApplyAll.setVisible(not visible)
        self.pbResetAll.setVisible(not visible)

        self.acKillInstance.setVisible(not visible)
        self.acApplyAll.setVisible(not visible)
        self.acResetAll.setVisible(not visible)
    updateButtonsVisibility = property(fset=_updateButtonsVisibility)

    def _resetPanel(self):
        """This is called when the configurator needs a reset which means all
        parameter editor pages need to be cleaned and removed.
        """
        self.prevConfiguration = None

        page = self.__swParameterEditor.widget(CONFIGURATION_PAGE)
        page.clear()

        self._setParameterEditorIndex(BLANK_PAGE)
        self._hideAllButtons()

    def _setConfiguration(self, conf):
        """Adapt to the Configuration which is currently showing
        """
        # Update buttons
        if conf is None or len(conf.binding.value) == 0:
            self._hideAllButtons()
        else:
            self.updateButtonsVisibility = (conf is not None and
                                            not isinstance(conf, DeviceProxy))

        # Toggle device updates
        if (self.prevConfiguration not in (None, conf) and
                isinstance(self.prevConfiguration, DeviceProxy)):
            self.prevConfiguration.remove_monitor()

        if (conf not in (None, self.prevConfiguration) and
                isinstance(conf, DeviceProxy)):
            conf.add_monitor()

        # Cancel notification of arriving schema
        if self._awaitingSchema is not None:
            awaited = self._awaitingSchema
            awaited.on_trait_change(self.onSchemaArrival, 'schema_update',
                                    remove=True)
            self._awaitingSchema = None

        # Handle configurations which await a schema
        if conf is not None and len(conf.binding.value) == 0:
            self._awaitingSchema = conf
            conf.on_trait_change(self.onSchemaArrival, 'schema_update')

        # Finally, set prevConfiguration
        self.prevConfiguration = conf

    # -----------------------------------------------------------------------
    # slots

    def onSchemaArrival(self, configuration, name, value):
        # A schema we wanted arrived
        assert configuration is self._awaitingSchema
        self._awaitingSchema = None
        configuration.on_trait_change(self.onSchemaArrival, 'schema_update',
                                      remove=True)

        if self.prevConfiguration is configuration:
            self.showConfiguration(configuration)

    def onOpenFromFile(self):
        if self.prevConfiguration is not None:
            loadConfigurationFromFile(self.prevConfiguration)

    def onSaveToFile(self):
        if self.prevConfiguration is not None:
            saveConfigurationToFile(self.prevConfiguration)

    def onApplyAll(self):
        self._getCurrentParameterEditor().apply_all()

    def onApplyAllRemoteChanges(self):
        self._getCurrentParameterEditor().decline_all_changes()

    def onApplySelectedRemoteChanges(self):
        twParameterEditor = self._getCurrentParameterEditor()
        selectedItems = twParameterEditor.selectedItems()
        for item in selectedItems:
            twParameterEditor.decline_item_changes(item)

    def onResetAll(self):
        self._getCurrentParameterEditor().decline_all()

    def onKillInstance(self):
        if not isinstance(self.prevConfiguration, DeviceProxy):
            return
        get_manager().shutdownDevice(self.prevConfiguration.device_id)

    def onInitDevice(self):
        config = None
        proxy = self.prevConfiguration
        server_id = proxy.server_id
        class_id = proxy.binding.class_id
        device_id = ''

        if isinstance(proxy, ProjectDeviceProxy):
            config = extract_configuration(proxy.binding)
            device_id = proxy.device_id

        get_manager().initDevice(server_id, class_id, device_id, config=config)
