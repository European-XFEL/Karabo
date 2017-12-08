#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import Qt, pyqtSlot
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
        self._showing_proxy = None
        self._awaiting_schema = None
        self._has_conflicts = False

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
        self._stacked_tree_widgets = QStackedWidget(widget)
        # BLANK_PAGE
        self._stacked_tree_widgets.addWidget(ConfigurationTreeView(widget))

        # WAITING_PAGE
        wait_widget = get_spin_widget(parent=widget)
        wait_widget.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        wait_widget.setAutoFillBackground(True)
        wait_widget.setBackgroundRole(QPalette.Base)
        self._stacked_tree_widgets.addWidget(wait_widget)

        # CONFIGURATION_PAGE
        self._stacked_tree_widgets.addWidget(ConfigurationTreeView(widget))

        hLayout = QHBoxLayout()
        hLayout.setContentsMargins(0, 5, 5, 5)

        text = "Instantiate device"
        self.pbInitDevice = QPushButton(icons.start, text)
        self.pbInitDevice.setToolTip(text)
        self.pbInitDevice.setStatusTip(text)
        self.pbInitDevice.setVisible(False)
        self.pbInitDevice.setMinimumWidth(140)
        self.pbInitDevice.clicked.connect(self._on_init_device)
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
        self.acKillInstance.triggered.connect(self._on_kill_device)
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
        self.acApplyAll.triggered.connect(self._on_apply_all)
        self.pbApplyAll.clicked.connect(self.acApplyAll.triggered)

        text = "Apply all my changes"
        self.acApplyLocalChanges = QAction(text, widget)
        self.acApplyLocalChanges.setStatusTip(text)
        self.acApplyLocalChanges.setToolTip(text)
        self.acApplyLocalChanges.triggered.connect(self._on_apply_all)

        text = "Adjust to current values on device"
        self.acApplyRemoteChanges = QAction(text, widget)
        self.acApplyRemoteChanges.setStatusTip(text)
        self.acApplyRemoteChanges.setToolTip(text)
        self.acApplyRemoteChanges.triggered.connect(
            self._on_apply_all_remote_changes)

        text = "Apply selected local changes"
        self.acApplySelectedChanges = QAction(text, widget)
        self.acApplySelectedChanges.setStatusTip(text)
        self.acApplySelectedChanges.setToolTip(text)
        self.acApplySelectedChanges.triggered.connect(self._on_apply_all)

        text = "Accept selected remote changes"
        self.acApplySelectedRemoteChanges = QAction(text, widget)
        self.acApplySelectedRemoteChanges.setStatusTip(text)
        self.acApplySelectedRemoteChanges.setToolTip(text)
        self.acApplySelectedRemoteChanges.triggered.connect(
            self._on_apply_selected_remote_changes)

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
        self.acResetAll.triggered.connect(self._on_reset_all)
        self.pbResetAll.clicked.connect(self.acResetAll.triggered)

        hLayout.addWidget(self.pbResetAll)
        hLayout.addStretch()

        self.rightScrollArea = QScrollArea(widget)
        self.rightScrollArea.setWidgetResizable(True)
        rightWidget = QWidget(widget)
        vLayout = QVBoxLayout(rightWidget)
        vLayout.setContentsMargins(0, 0, 0, 0)
        vLayout.addWidget(self._stacked_tree_widgets)
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
        self.acOpenFromFile.triggered.connect(self._on_open_from_file)

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
        self.acSaveToFile.triggered.connect(self._on_save_to_file)

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
            proxy = data.get('configuration')
            self._show_configuration(proxy)
        elif event.sender is KaraboEventSender.UpdateDeviceConfigurator:
            proxy = data.get('configuration')
            self._update_displayed_configuration(proxy)
        elif event.sender is KaraboEventSender.ClearConfigurator:
            deviceId = data.get('deviceId', '')
            self._remove_departed_device(deviceId)
        elif event.sender is KaraboEventSender.NetworkConnectStatus:
            connected = data['status']
            if not connected:
                self._reset_panel()

        return False

    # -----------------------------------------------------------------------
    # private methods

    def _hide_all_buttons(self):
        """Hide buttons and actions"""
        self.pbInitDevice.setVisible(False)

        self.pbKillInstance.setVisible(False)
        self.acKillInstance.setVisible(False)
        self.pbApplyAll.setVisible(False)
        self.acApplyAll.setVisible(False)
        self.pbResetAll.setVisible(False)
        self.acResetAll.setVisible(False)

    def _remove_departed_device(self, device_id):
        """Clear the configuration panel when a device goes away
        """
        proxy = self._showing_proxy
        if proxy is None:
            return

        if isinstance(proxy, DeviceProxy) and device_id == proxy.device_id:
            self._show_configuration(None)

    def _reset_panel(self):
        """This is called when the configurator needs a reset which means all
        parameter editor pages need to be cleaned and removed.
        """
        self._showing_proxy = None

        tree_widget = self._stacked_tree_widgets.widget(CONFIGURATION_PAGE)
        tree_widget.clear()

        self._set_stack_widget_index(BLANK_PAGE)
        self._hide_all_buttons()

    def _schema_update(self, proxy, name, value):
        """Trait notification handler for a schema_update event on the
        BaseDeviceProxy object referenced in `self._awaiting_schema`.
        """
        assert proxy is self._awaiting_schema
        self._awaiting_schema = None
        proxy.on_trait_change(self._schema_update, 'schema_update',
                              remove=True)

        if self._showing_proxy is proxy:
            self._show_configuration(proxy)

    def _set_stack_widget_index(self, index):
        """Pick between one of the pages in `self._stacked_tree_widgets`.
        """
        self._stacked_tree_widgets.blockSignals(True)
        self._stacked_tree_widgets.setCurrentIndex(index)
        self._stacked_tree_widgets.blockSignals(False)

        visible = (index != BLANK_PAGE)
        self.acOpenConfig.setVisible(visible)
        self.acSaveConfig.setVisible(visible)

    def _set_tree_widget_configuration(self, tree_widget, proxy):
        tree_widget.assign_proxy(proxy)
        tree_widget.resizeColumnToContents(0)
        tree_widget.resizeColumnToContents(1)

    def _set_configuration(self, proxy):
        """Adapt to the Configuration which is currently showing
        """
        # Update buttons
        if proxy is None or len(proxy.binding.value) == 0:
            self._hide_all_buttons()
        else:
            is_device = isinstance(proxy, DeviceProxy)
            self.update_buttons_visibility = not (proxy is None or is_device)

        # Toggle device updates
        if (self._showing_proxy not in (None, proxy) and
                isinstance(self._showing_proxy, DeviceProxy)):
            self._showing_proxy.remove_monitor()

        if (proxy not in (None, self._showing_proxy) and
                isinstance(proxy, DeviceProxy)):
            proxy.add_monitor()

        # Cancel notification of arriving schema
        if self._awaiting_schema is not None:
            awaited = self._awaiting_schema
            awaited.on_trait_change(self._schema_update, 'schema_update',
                                    remove=True)
            self._awaiting_schema = None

        # Handle configurations which await a schema
        if proxy is not None and len(proxy.binding.value) == 0:
            self._awaiting_schema = proxy
            proxy.on_trait_change(self._schema_update, 'schema_update')

        # Finally, set _showing_proxy
        self._showing_proxy = proxy

    def _show_configuration(self, proxy):
        """Show a Configuration object in the panel
        """
        if proxy is None:
            # There is no configuration to show
            index = BLANK_PAGE
        elif len(proxy.binding.value) == 0:
            # The configuration is not ready to be shown (no Schema)
            index = WAITING_PAGE
        else:
            # The configuration is OK to show
            index = CONFIGURATION_PAGE
            tree_widget = self._stacked_tree_widgets.widget(index)
            self._set_tree_widget_configuration(tree_widget, proxy)

        # This is the configuration we're viewing now
        self._set_stack_widget_index(index)
        self._set_configuration(proxy)

    def _update_displayed_configuration(self, proxy):
        if self._showing_proxy is None:
            return

        def _get_ids(conf):
            class_id = conf.binding.class_id
            server_id = conf.server_id
            device_id = ('' if not hasattr(conf, 'device_id')
                         else conf.device_id)
            return class_id, server_id, device_id

        previous = self._showing_proxy
        cur_class_id, cur_server_id, cur_dev_id = _get_ids(previous)
        class_id, server_id, dev_id = _get_ids(proxy)
        if (server_id == cur_server_id and class_id == cur_class_id and
                dev_id == cur_dev_id):
            # FIXME: Maybe what really needs to be done here is to just
            # update the view?
            # Maybe this method is obsolete?
            self._show_configuration(proxy)

    def _update_apply_all_actions(self, configuration):
        tree_widget = self._stacked_tree_widgets.widget(CONFIGURATION_PAGE)

        selected_count = tree_widget.nbSelectedApplyEnabledItems()
        if self.pbApplyAll.isEnabled() and selected_count > 0:
            if selected_count == 1:
                text = "Apply selected"
            else:
                text = "Apply ({}) selected".format(selected_count)

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

        if self.has_conflicts:
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

    def _update_reset_all_actions(self, configuration):
        tree_widget = self._stacked_tree_widgets.widget(CONFIGURATION_PAGE)
        selected_count = tree_widget.nbSelectedApplyEnabledItems()

        if self.pbResetAll.isEnabled() and selected_count > 0:
            if selected_count == 1:
                text = "Decline selected"
            else:
                text = "Decline ({}) selected".format(selected_count)
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

    # ----------------------------------------------------------------------
    # property attributes

    @property
    def has_conflicts(self):
        return self._has_conflicts

    @has_conflicts.setter
    def has_conflicts(self, has_conflicts):
        self._has_conflicts = has_conflicts

        if has_conflicts:
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
        self.acApplyLocalChanges.setVisible(has_conflicts)
        self.acApplyRemoteChanges.setVisible(has_conflicts)

        self.acApplySelectedChanges.setVisible(not has_conflicts)
        self.acApplySelectedRemoteChanges.setVisible(not has_conflicts)

    def _update_buttons_visibility(self, visible):
        self.pbInitDevice.setVisible(visible)

        self.pbKillInstance.setVisible(not visible)
        self.pbApplyAll.setVisible(not visible)
        self.pbResetAll.setVisible(not visible)

        self.acKillInstance.setVisible(not visible)
        self.acApplyAll.setVisible(not visible)
        self.acResetAll.setVisible(not visible)
    update_buttons_visibility = property(fset=_update_buttons_visibility)

    # -----------------------------------------------------------------------
    # slots

    @pyqtSlot()
    def _on_apply_all(self):
        tree_widget = self._stacked_tree_widgets.widget(CONFIGURATION_PAGE)
        tree_widget.apply_all()

    @pyqtSlot()
    def _on_apply_all_remote_changes(self):
        tree_widget = self._stacked_tree_widgets.widget(CONFIGURATION_PAGE)
        tree_widget.decline_all_changes()

    @pyqtSlot()
    def _on_apply_selected_remote_changes(self):
        tree_widget = self._stacked_tree_widgets.widget(CONFIGURATION_PAGE)
        selected = tree_widget.selectedItems()
        for item in selected:
            tree_widget.decline_item_changes(item)

    @pyqtSlot()
    def _on_reset_all(self):
        tree_widget = self._stacked_tree_widgets.widget(CONFIGURATION_PAGE)
        tree_widget.decline_all()

    @pyqtSlot()
    def _on_open_from_file(self):
        if self._showing_proxy is not None:
            loadConfigurationFromFile(self._showing_proxy)

    @pyqtSlot()
    def _on_save_to_file(self):
        if self._showing_proxy is not None:
            saveConfigurationToFile(self._showing_proxy)

    @pyqtSlot()
    def _on_kill_device(self):
        if not isinstance(self._showing_proxy, DeviceProxy):
            return
        get_manager().shutdownDevice(self._showing_proxy.device_id)

    @pyqtSlot()
    def _on_init_device(self):
        config = None
        proxy = self._showing_proxy
        server_id = proxy.server_id
        class_id = proxy.binding.class_id
        device_id = ''

        if isinstance(proxy, ProjectDeviceProxy):
            config = extract_configuration(proxy.binding)
            device_id = proxy.device_id

        get_manager().initDevice(server_id, class_id, device_id, config=config)
