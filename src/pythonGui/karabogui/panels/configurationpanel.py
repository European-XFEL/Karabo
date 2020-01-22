#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt5.QtCore import Qt, pyqtSlot
from PyQt5.QtGui import QPalette
from PyQt5.QtWidgets import (
    QAction, QHBoxLayout, QPushButton, QScrollArea, QStackedWidget,
    QVBoxLayout, QWidget)

from karabo.native import AccessMode
from karabogui import globals as krb_globals, icons, messagebox
from karabogui.binding.api import (
    ChoiceOfNodesBinding, DeviceProxy, ListOfNodesBinding, ProjectDeviceProxy,
    attr_fast_deepcopy, apply_configuration, extract_configuration,
    flat_iter_hash, has_changes)
from karabogui.configurator.api import ConfigurationTreeView
from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.singletons.api import get_manager
from karabogui.util import (
    get_spin_widget, load_configuration_from_file, save_configuration_to_file
)
from karabogui.widgets.toolbar import ToolBar
from .base import BasePanelWidget

BLANK_PAGE = 0
WAITING_PAGE = 1
CONFIGURATION_PAGE = 2

RECURSIVE_BINDINGS = (ChoiceOfNodesBinding, ListOfNodesBinding)


class ConfigurationPanel(BasePanelWidget):
    def __init__(self):
        super(ConfigurationPanel, self).__init__("Configuration Editor")
        self._showing_proxy = None
        self._awaiting_schema = None

        # Register for broadcast events.
        event_map = {
            KaraboEvent.ShowConfiguration: self._event_show_configuration,
            KaraboEvent.UpdateDeviceConfigurator: self._event_update_config,
            KaraboEvent.UpdateValueConfigurator: self._event_display_update,
            KaraboEvent.ClearConfigurator: self._event_clear_configurator,
            KaraboEvent.LoadConfiguration: self._event_load_configuration,
            KaraboEvent.ShowConfigurationFromPast: self._event_config_past,
            KaraboEvent.NetworkConnectStatus: self._event_network
        }
        register_for_broadcasts(event_map)

    # -----------------------------------------------------------------------
    # Karabo Events

    def _event_show_configuration(self, data):
        proxy = data['proxy']
        self._show_configuration(proxy)

    def _event_update_config(self, data):
        proxy = data['proxy']
        self._update_displayed_configuration(proxy)

    def _event_display_update(self, data):
        proxy = data['proxy']
        self._update_displayed_values(proxy)

    def _event_clear_configurator(self, data):
        devices = data.get('devices', [])
        self._remove_departed_device(devices)

    def _event_load_configuration(self, data):
        proxy = data['proxy']
        configuration = data['configuration']
        self._apply_loaded_configuration(proxy, configuration)

    def _event_config_past(self, data):
        deviceId = data['deviceId']
        configuration = data['configuration']
        time = data['time']
        self._apply_configuration_from_past(deviceId, configuration, time)

    def _event_network(self, data):
        connected = data['status']
        if not connected:
            self._reset_panel()

    # -----------------------------------------------------------------------

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
        wait_widget = get_spin_widget(icon='wait', parent=widget)
        wait_widget.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        wait_widget.setAutoFillBackground(True)
        wait_widget.setBackgroundRole(QPalette.Base)
        self._stacked_tree_widgets.addWidget(wait_widget)

        # CONFIGURATION_PAGE
        editor = ConfigurationTreeView(widget)
        editor.model().signalHasModifications.connect(self._on_button_changes)
        self._stacked_tree_widgets.addWidget(editor)

        hLayout = QHBoxLayout()
        hLayout.setContentsMargins(0, 5, 5, 5)

        text = "Instantiate device"
        self.pbInitDevice = QPushButton(icons.run, text)
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
        self.acApplyAll.triggered.connect(self._on_apply_all)
        self.pbApplyAll.clicked.connect(self.acApplyAll.triggered)
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
        icon_size = toolbar.iconSize()

        text = "Open configuration from file (*.xml)"
        tb_open_config = QPushButton()
        tb_open_config.setIcon(icons.load)
        tb_open_config.setIconSize(icon_size)
        tb_open_config.setStatusTip(text)
        tb_open_config.setToolTip(text)
        tb_open_config.setVisible(False)
        tb_open_config.setFlat(True)
        tb_open_config.clicked.connect(self._on_open_from_file)

        text = "Save configuration to file (*.xml)"
        tb_save_config = QPushButton()
        tb_save_config.setIcon(icons.saveAs)
        tb_save_config.setIconSize(icon_size)
        tb_save_config.setStatusTip(text)
        tb_save_config.setToolTip(text)
        tb_save_config.setVisible(False)
        tb_save_config.setFlat(True)
        tb_save_config.clicked.connect(self._on_save_to_file)

        self.ui_open_config = toolbar.addWidget(tb_open_config)
        self.ui_save_config = toolbar.addWidget(tb_save_config)

        return [toolbar]

    # -----------------------------------------------------------------------
    # private methods

    def _apply_configuration_from_past(self, deviceId, configuration, time):
        """Apply the retrieved configuration from getConfigurationFromPast
        """
        proxy = self._showing_proxy
        if proxy is None:
            return

        binding = proxy.binding
        # The check we can provide is to check the deviceId and classId
        # NOTE: Schema evolution should not be a problem!
        classId = configuration.get('classId', None)
        if classId is None:
            # XXX: We might still get an invalid configuration without classId
            messagebox.show_error("A configuration without classId arrived "
                                  "and is ignored! ", parent=self)
            return

        if proxy.device_id != deviceId:
            messagebox.show_error("A configuration for '{}' arrived, but is "
                                  "ignored since '{}' shown in editor.".format(
                                   deviceId, proxy.device_id), parent=self)
            return
        if binding.class_id != classId:
            messagebox.show_error("A configuration for classId '{}' arrived, "
                                  "but device in editor is a '{}'".format(
                                   classId, binding.class_id), parent=self)
            return

        self._set_proxy_configuration(proxy, configuration)

        messagebox.show_information("Configuration from '{}' has arrived "
                                    "for '{}'!".format(time, deviceId),
                                    parent=self)

    def _apply_loaded_configuration(self, proxy, configuration):
        """Apply a configuration loaded from a file to a proxy
        """
        if self._showing_proxy is not proxy:
            return

        binding = proxy.binding
        # Loading a configuration from file has to check for a present Schema
        # and if the classId is the first key of the configuration
        if len(binding.value) == 0 or binding.class_id not in configuration:
            messagebox.show_error('Configuration load failed in configurator.',
                                  parent=self)
            return

        configuration = configuration[binding.class_id]
        self._set_proxy_configuration(proxy, configuration)

    def _set_proxy_configuration(self, proxy, configuration):
        """Internal method to apply the configuration in the Configurator
        """
        binding = proxy.binding
        access_level = krb_globals.GLOBAL_ACCESS_LEVEL
        if isinstance(proxy, DeviceProxy):
            # Load the configuration into PropertyProxy instances
            state = proxy.state_binding.value
            editor = self._stacked_tree_widgets.widget(CONFIGURATION_PAGE)
            model = editor.model()
            for path, value, _ in flat_iter_hash(configuration):
                prop_proxy = model.property_proxy(path)
                prop_binding = prop_proxy.binding
                if prop_binding is None or isinstance(
                        prop_binding, RECURSIVE_BINDINGS):
                    # NOTE: This property most likely was removed from the
                    # device, we have schema evolution and will continue here!
                    # NOTE: Recursive bindings are not supported here!
                    continue
                if prop_binding.access_mode is AccessMode.RECONFIGURABLE:
                    if (prop_binding.required_access_level <= access_level and
                            prop_binding.is_allowed(state) and
                            has_changes(prop_binding,
                                        prop_proxy.value, value)):
                        prop_proxy.edit_value = value
                    else:
                        prop_proxy.edit_value = None
            # NOTE: We tell the model directly to send dataChanged signal and
            # notify for changes!
            model._config_update()
        else:
            # Load the configuration directly into the binding
            apply_configuration(configuration, binding)
            # Apply attributes in a second step
            for path, _, attrs in flat_iter_hash(configuration):
                binding = proxy.get_property_binding(path)
                if binding is not None:
                    # only update editable attribute values
                    binding.update_attributes(attr_fast_deepcopy(attrs, {}))
            # Notify again. XXX: Schema update too?
            proxy.binding.config_update = True

    def _hide_all_buttons(self):
        """Hide buttons and actions"""
        self.pbInitDevice.setVisible(False)
        self.pbKillInstance.setVisible(False)
        self.acKillInstance.setVisible(False)
        self.pbApplyAll.setVisible(False)
        self.acApplyAll.setVisible(False)
        self.pbResetAll.setVisible(False)
        self.acResetAll.setVisible(False)

    def _remove_departed_device(self, devices):
        """Clear the configuration panel when a device goes away

        :param devices: List of devices that are gone!
        """
        proxy = self._showing_proxy
        if proxy is None or not isinstance(proxy, DeviceProxy):
            return

        if proxy.device_id in devices:
            self._show_configuration(None)

    def _reset_panel(self):
        """This is called when the configurator needs a reset which means all
        parameter editor pages need to be cleaned and removed.
        """
        self._showing_proxy = None

        tree_widget = self._stacked_tree_widgets.widget(CONFIGURATION_PAGE)
        if tree_widget.popup_widget is not None:
            tree_widget.popup_widget.close()
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
        self.ui_open_config.setVisible(visible)
        self.ui_save_config.setVisible(visible)

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

    def _update_displayed_values(self, proxy):
        """Update the apply and decline buttons after displayed value update
        """
        if self._showing_proxy is not proxy:
            return

        index = CONFIGURATION_PAGE
        tree_widget = self._stacked_tree_widgets.widget(index)
        model = tree_widget.model()
        if model is not None:
            model._notify_of_modifications()

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
            # Iff current showing device proxy matches the updated one,
            # refresh the view.
            self._show_configuration(proxy)

    # ----------------------------------------------------------------------
    # property attributes

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

    @pyqtSlot(bool)
    def _on_button_changes(self, has_changes):
        self.pbApplyAll.setEnabled(has_changes)
        self.pbResetAll.setEnabled(has_changes)
        self.acApplyAll.setEnabled(has_changes)
        self.acResetAll.setEnabled(has_changes)

    @pyqtSlot()
    def _on_apply_all(self):
        tree_widget = self._stacked_tree_widgets.widget(CONFIGURATION_PAGE)
        tree_widget.apply_all()

    @pyqtSlot()
    def _on_reset_all(self):
        tree_widget = self._stacked_tree_widgets.widget(CONFIGURATION_PAGE)
        tree_widget.decline_all()

    @pyqtSlot()
    def _on_open_from_file(self):
        if self._showing_proxy is not None:
            load_configuration_from_file(self._showing_proxy, parent=self)

    @pyqtSlot()
    def _on_save_to_file(self):
        if self._showing_proxy is not None:
            save_configuration_to_file(self._showing_proxy, parent=self)

    @pyqtSlot()
    def _on_kill_device(self):
        if not isinstance(self._showing_proxy, DeviceProxy):
            return
        get_manager().shutdownDevice(self._showing_proxy.device_id,
                                     parent=self)

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
