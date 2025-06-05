#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
from qtpy.QtCore import Qt, Slot
from qtpy.QtGui import QCursor, QPalette
from qtpy.QtWidgets import (
    QAction, QDialog, QHBoxLayout, QPushButton, QScrollArea, QStackedWidget,
    QToolButton, QTreeView, QVBoxLayout, QWidget)

import karabogui.access as krb_access
from karabo.native import AccessMode, Assignment, Hash, has_changes
from karabogui import icons, messagebox
from karabogui.access import AccessRole, access_role_allowed
from karabogui.binding.api import (
    DeviceClassProxy, DeviceProxy, ProjectDeviceProxy, VectorHashBinding,
    apply_configuration, get_binding_value, validate_table_value,
    validate_value)
from karabogui.configurator.api import ConfigurationTreeView
from karabogui.dialogs.api import (
    ConfigurationFromPastDialog, ConfigurationFromPastPreview, DataViewDialog,
    DeviceSelectorDialog)
from karabogui.events import (
    KaraboEvent, register_for_broadcasts, unregister_from_broadcasts)
from karabogui.logger import get_logger
from karabogui.singletons.api import get_manager
from karabogui.util import (
    get_spin_widget, load_configuration_from_file, move_to_cursor,
    save_configuration_to_file)
from karabogui.widgets.toolbar import ToolBar

from .base import BasePanelWidget
from .panel_info import create_configurator_info
from .switch import SwitchButton
from .tool_widget import ConfiguratorSearch
from .utils import (
    compare_proxy_essential, format_property_details,
    format_vector_hash_details)

BLANK_PAGE = 0
WAITING_PAGE = 1
CONFIGURATION_PAGE = 2


class ConfigurationPanel(BasePanelWidget):
    def __init__(self, allow_closing=False):
        super().__init__("Configuration Editor", allow_closing)
        self._showing_proxy = None
        self._awaiting_schema = None

        # Register for broadcast events.
        self.event_map = {
            KaraboEvent.ShowConfiguration: self._event_show_configuration,
            KaraboEvent.UpdateDeviceConfigurator: self._event_update_config,
            KaraboEvent.UpdateValueConfigurator: self._event_display_update,
            KaraboEvent.ClearConfigurator: self._event_clear_configurator,
            KaraboEvent.LoadConfiguration: self._event_load_configuration,
            KaraboEvent.ShowConfigurationFromPast: self._event_config_past,
            KaraboEvent.ShowInitConfiguration: self._event_init_config,
            KaraboEvent.NetworkConnectStatus: self._event_network,
            KaraboEvent.AccessLevelChanged: self._event_access_level,
            KaraboEvent.LoginUserChanged: self._event_access_level,
        }
        register_for_broadcasts(self.event_map)

    # -----------------------------------------------------------------------
    # Karabo Events

    def _event_access_level(self, data):
        self._update_buttons(self._showing_proxy)
        access = krb_access.GLOBAL_ACCESS_LEVEL
        self.treeView().setAccessLevel(access)

    def _event_show_configuration(self, data):
        proxy = data['proxy']
        self._show_configuration(proxy)

    def _event_update_config(self, data):
        proxy = data['proxy']
        refresh = data.get('refresh', False)
        self._update_displayed_configuration(proxy, refresh)

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
        req_time = data['time']
        config_time = data['config_time']
        time_match = data['time_match']
        preview = data['preview']
        self._apply_configuration_from_past(deviceId, configuration, req_time,
                                            config_time, time_match, preview)

    def _event_init_config(self, data):
        deviceId = data['deviceId']
        configuration = data['configuration']
        name = data['name']
        preview = data['preview']
        classId = data['classId']
        self._apply_init_configuration(
            deviceId, classId, configuration, name, preview)

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
        mainLayout.setContentsMargins(0, 0, 0, 2)

        # Stacked widget for configuration parameters
        self._stacked_tree_widgets = QStackedWidget(widget)
        # BLANK_PAGE
        self._stacked_tree_widgets.addWidget(QTreeView(widget))

        # WAITING_PAGE
        wait_widget = get_spin_widget(icon='wait', parent=widget)
        wait_widget.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        wait_widget.setAutoFillBackground(True)
        wait_widget.setBackgroundRole(QPalette.Base)
        self._stacked_tree_widgets.addWidget(wait_widget)

        # CONFIGURATION_PAGE
        editor = ConfigurationTreeView(widget)
        editor.sourceModel().signalHasModifications.connect(
            self._on_button_changes)
        self._stacked_tree_widgets.addWidget(editor)

        self.searchBar = ConfiguratorSearch()
        self.searchBar.setView(editor)
        mainLayout.addWidget(self.searchBar)

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
        self.pbKillInstance.setEnabled(not self.allow_closing)
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

        text = "Configuration from past"
        tb_history_config = QToolButton()
        tb_history_config.setIcon(icons.clock)
        tb_history_config.setStatusTip(text)
        tb_history_config.setToolTip(text)
        tb_history_config.setVisible(False)
        tb_history_config.clicked.connect(self._on_config_from_past)

        text = "Compare Configuration"
        tb_compare_config = QToolButton()
        tb_compare_config.setIcon(icons.change)
        tb_compare_config.setStatusTip(text)
        tb_compare_config.setToolTip(text)
        tb_compare_config.setVisible(False)
        tb_compare_config.clicked.connect(self._on_compare_configuration)

        text = "Switch Configuration"
        tb_switch_mode = SwitchButton()
        tb_switch_mode.setStatusTip(text)
        tb_switch_mode.setToolTip(text)
        tb_switch_mode.setVisible(True)
        tb_switch_mode.setMinimumWidth(140)
        tb_switch_mode.toggled.connect(self._switch_mode_toggled)

        text = "Search Device Properties"
        tb_search_device = QToolButton()
        tb_search_device.setIcon(icons.zoomImage)
        tb_search_device.setIconSize(icon_size)
        tb_search_device.setStatusTip(text)
        tb_search_device.setToolTip(text)
        tb_search_device.setVisible(True)
        tb_search_device.clicked.connect(self.searchBar.toggleActivate)

        self.ui_open_config = toolbar.addWidget(tb_open_config)
        self.ui_save_config = toolbar.addWidget(tb_save_config)
        self.ui_history_config = toolbar.addWidget(tb_history_config)
        self.ui_compare_config = toolbar.addWidget(tb_compare_config)
        self.ui_search_device = toolbar.addWidget(tb_search_device)
        self.ui_switch_mode = toolbar.addWidget(tb_switch_mode)

        return [toolbar]

    # -----------------------------------------------------------------------
    # private methods

    def _check_configuration(self, deviceId, classId):
        proxy = self._showing_proxy
        if proxy is None:
            return False

        binding = proxy.binding
        # The check we can provide is to check the deviceId and classId
        if classId is None:
            # XXX: We might still get an invalid configuration without classId
            messagebox.show_error("A configuration without classId arrived "
                                  "and is ignored! ", parent=self)
            return False

        if proxy.device_id != deviceId:
            messagebox.show_error(
                "A configuration for '{}' arrived, but is "
                "ignored since '{}' shown in editor.".format(
                    deviceId, proxy.device_id), parent=self)
            return False
        if binding.class_id != classId:
            # Note: Previously we validated strong here. Since we validate
            # binding by binding, we continue gracefully but leave a log
            # message. Schema evolution is not a problem anymore.
            text = (f"A configuration for classId <b>{classId}</b> arrived, "
                    f"but the device <b>{deviceId}</b> shown in editor is a "
                    f"<b>{binding.class_id}</b>")
            get_logger().error(text)
            return True

        return True

    def _apply_init_configuration(
            self, deviceId, classId, config, name, preview):
        """Apply the retrieved configuration from getConfigurationFromName"""
        validated = self._check_configuration(deviceId, classId)
        if not validated:
            return

        proxy = self._showing_proxy
        if preview:
            title = (f"Showing configuration {name} for device "
                     f"{proxy.device_id}")
            info = ("Note: Only changes from the device defaults "
                    "are stored as configuration.")
            dialog = DataViewDialog(title, info, config, parent=self)
            if dialog.exec() == QDialog.Accepted:
                self._set_proxy_configuration(proxy, config)
        else:
            self._set_proxy_configuration(proxy, config)
            text = (f"Configuration with name <b>{name}</b> has arrived "
                    f"for <b>{deviceId}</b>.\n")
            messagebox.show_information(text, parent=self)

    def _apply_configuration_from_past(self, deviceId, config, req_time,
                                       config_time, match, preview):
        """Apply the retrieved configuration from getConfigurationFromPast"""
        classId = config.get("classId")
        validated = self._check_configuration(deviceId, classId)
        if not validated:
            return

        proxy = self._showing_proxy
        # Show a nice information for the user!
        matched_text = ("The configuration has been retrieved for the "
                        "requested time point!")
        no_match_text = ("Requested time was '{}', but the device was not "
                         "logged then.").format(req_time)
        time_text = matched_text if match else no_match_text
        text = "Configuration from '{}' has arrived for '{}'.\n{}".format(
            config_time, deviceId, time_text)

        if preview:
            title = f"Showing configuration for device {proxy.device_id}"
            dialog = ConfigurationFromPastPreview(title, text, config, proxy,
                                                  parent=self)
            if dialog.exec() == QDialog.Accepted:
                self._set_proxy_configuration(proxy, config)
        else:
            self._set_proxy_configuration(proxy, config)
            messagebox.show_information(text, parent=self)

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
        if isinstance(proxy, DeviceProxy):
            self._set_device_config(proxy, configuration)
        else:
            self._set_project_device_config(proxy, configuration)

    def _set_device_config(self, proxy, configuration):
        """Sets the configuration of an online device by validating and setting
           the edit value of every reconfigurable property. This is done as
           follows:

           1. Iterate over each property
           2. Check if the property binding is valid
           3. Check if the binding can be edited
              (reconfigurable, allowed state and allowed access level)
           4. Check if there are changes in the value (first check)
           5. If there are changes, we validate the value from the binding
              information. If it fails to be validated, we report it as invalid
           6. We again compare to check changes (second check)
           7. If there are changes, we set it on the edit value.
        """
        # Load the configuration into PropertyProxy instances
        invalid_prop = {}
        access_level = krb_access.GLOBAL_ACCESS_LEVEL
        state = proxy.state_binding.value

        editor = self._stacked_tree_widgets.widget(CONFIGURATION_PAGE)
        model = editor.sourceModel()
        for path, value, _ in Hash.flat_iterall(configuration):
            prop_proxy = model.property_proxy(path)
            prop_binding = prop_proxy.binding
            if prop_binding is None:
                # NOTE: This property most likely was removed from the
                # device, we have schema evolution and will continue here!
                continue
            if prop_binding.accessMode is not AccessMode.RECONFIGURABLE:
                continue
            prop_value = get_binding_value(prop_binding)
            edit_value = None
            if (prop_binding.requiredAccessLevel <= access_level
                    and prop_binding.is_allowed(state)):
                if isinstance(prop_binding, VectorHashBinding):
                    valid, invalid = validate_table_value(prop_binding, value)
                    if (valid and has_changes(
                            prop_value, valid)):
                        edit_value = valid
                    if invalid:
                        invalid_prop[path] = invalid
                elif has_changes(prop_value, value):
                    valid = validate_value(prop_binding, value)
                    if valid is None:
                        invalid_prop[path] = value
                    elif has_changes(prop_value, valid):
                        edit_value = valid
            prop_proxy.edit_value = edit_value
        # Note: We tell the model directly to send dataChanged signal and
        # notify for changes!
        model.announceDataChanged()
        model.notify_of_modifications()

        # Show a dialog for invalid keys:
        if invalid_prop:
            self._show_not_loaded_properties(proxy, invalid_prop)

    def _set_project_device_config(self, proxy, configuration):
        """Sets the configuration of an of device by validating and setting
           the value of every non-readonly property. This is done as follows:

           1. Iterate over each writable property by (deep)copying the
              configuration and deleting nonexistent and non-configurable
              properties to exclude it from bulk setting
           2. Check if the property binding is valid and writeable
              (non-readonly). If not, we exclude the property from the bulk
              setting by deleting it from the configuration
           3. Validate the value from the binding information. If it fails to
              be validated, we report it as invalid and exclude it from
              the bulk setting.
           4. Apply the validated configuration and its attributes
        """

        # Get the validated config for the bulk update
        valid, invalid = self._get_validated_config(proxy, configuration)

        # Load the configuration directly into the binding
        if len(valid):
            apply_configuration(valid, proxy.binding)
            # Notify again
            proxy.binding.config_update = True

        # Show a dialog for invalid keys:
        if invalid:
            self._show_not_loaded_properties(proxy, invalid)

    def _get_validated_config(self, proxy, configuration):
        """Validate the configuration by populating a new Hash with the
           validated values and attributes. We ignore read-only properties
           and with invalid values"""

        valid, invalid = Hash(), Hash()  # {path: value}
        for path, value, attrs in Hash.flat_iterall(configuration):
            binding = proxy.get_property_binding(path)
            writable = (binding is not None and
                        binding.accessMode != AccessMode.READONLY and
                        binding.assignment != Assignment.INTERNAL)
            if not writable:
                continue
            if isinstance(binding, VectorHashBinding):
                valid_vhash, invalid_vhash = validate_table_value(
                    binding, value)
                if invalid_vhash:
                    invalid[path] = invalid_vhash
                if valid_vhash:
                    old_value = get_binding_value(binding)
                    if has_changes(old_value, valid_vhash):
                        valid[path] = valid_vhash
            else:
                validated_value = validate_value(binding, value)
                if validated_value is not None:
                    valid[path] = validated_value
                    valid[path, ...] = attrs
                else:
                    # Report only existing and configurable properties but
                    # with invalid values.
                    invalid[path] = value

        return valid, invalid

    def _show_not_loaded_properties(self, device_proxy, invalid_prop):
        msg = ("Some values are not loaded as they are "
               "invalid for the current property.")
        details = []
        for path, value in invalid_prop.items():
            binding = device_proxy.get_property_binding(path)
            if isinstance(binding, VectorHashBinding):
                value = format_vector_hash_details(binding, value)
            details.append(format_property_details(binding, path, value))

        messagebox.show_warning(msg, details="\n".join(details),
                                parent=self)

    def _update_buttons(self, proxy):
        """Main method to update the button visibility of the configurator"""
        access = access_role_allowed(AccessRole.SERVICE_EDIT)
        if (proxy is None or not len(proxy.binding.value)
                or (isinstance(proxy, DeviceClassProxy)
                    and not isinstance(proxy, ProjectDeviceProxy)) or
                not access):
            self._hide_all_buttons()
        else:
            is_device = isinstance(proxy, DeviceProxy)
            self._show_button_visibility(not (proxy is None or is_device))

        allow_fetch_config = isinstance(proxy,
                                        (DeviceProxy, ProjectDeviceProxy))
        self.ui_history_config.setVisible(allow_fetch_config)
        self.ui_compare_config.setVisible(allow_fetch_config and proxy.online)

    def _show_button_visibility(self, visible):
        """Show the configurator buttons depending of the `visible` parameter
        """
        self.pbInitDevice.setVisible(visible)

        self.pbKillInstance.setVisible(not visible)
        self.pbApplyAll.setVisible(not visible)
        self.pbResetAll.setVisible(not visible)

        self.acKillInstance.setVisible(not visible)
        self.acApplyAll.setVisible(not visible)
        self.acResetAll.setVisible(not visible)

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
        self.ui_history_config.setVisible(visible)

    def _set_tree_widget_configuration(self, tree_widget, proxy):
        tree_widget.assign_proxy(proxy)
        tree_widget.resizeColumnToContents(0)
        tree_widget.resizeColumnToContents(1)

    def _set_configuration(self, proxy):
        """Adapt to the Configuration which is currently showing
        """
        # Update buttons at the bottom
        self._update_buttons(proxy)

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
        model = tree_widget.sourceModel()
        if model is not None:
            model.notify_of_modifications()

    def _update_displayed_configuration(self, proxy, refresh=False):
        """Update the displayed configuration for a `proxy`

        This can be enforced with `refresh` equal to `True`.
        """
        if self._showing_proxy is None:
            return
        if compare_proxy_essential(proxy, self._showing_proxy) or refresh:
            # - If current showing device proxy matches the updated one,
            #   refresh the view.
            # - The device might have been gone from online to offline. In case
            #   of a project device proxy, make sure we have a schema!
            # - We might have to refresh in case a device is renamed
            if isinstance(proxy, ProjectDeviceProxy):
                proxy.ensure_class_schema()

            self._show_configuration(proxy)

    # -----------------------------------------------------------------------
    # Public Interface

    def model(self):
        """Returns the `QAbstractItemModel` associated with the tree view"""
        tree_widget = self._stacked_tree_widgets.widget(CONFIGURATION_PAGE)
        return tree_widget.model()

    def treeView(self):
        """Returns the `QTreeView` associated with the configuration panel"""
        return self._stacked_tree_widgets.widget(CONFIGURATION_PAGE)

    def info(self):
        """Returns the panel data for the `KaraboLogBook`"""
        return create_configurator_info(self)

    def __repr__(self):
        return f"<{self.__class__.__name__} proxy={repr(self.model().root)}>"

    # -----------------------------------------------------------------------
    # Qt Slot

    @Slot(bool)
    def _on_button_changes(self, has_changes):
        self.pbApplyAll.setEnabled(has_changes)
        self.pbResetAll.setEnabled(has_changes)
        self.acApplyAll.setEnabled(has_changes)
        self.acResetAll.setEnabled(has_changes)

    @Slot()
    def _on_apply_all(self):
        tree_widget = self._stacked_tree_widgets.widget(CONFIGURATION_PAGE)
        tree_widget.apply_all()

    @Slot()
    def _on_reset_all(self):
        tree_widget = self._stacked_tree_widgets.widget(CONFIGURATION_PAGE)
        tree_widget.decline_all()

    @Slot()
    def _on_open_from_file(self):
        if self._showing_proxy is not None:
            load_configuration_from_file(self._showing_proxy, parent=self)

    @Slot()
    def _on_save_to_file(self):
        if self._showing_proxy is not None:
            save_configuration_to_file(self._showing_proxy, parent=self)

    @Slot()
    def _on_kill_device(self):
        if not isinstance(self._showing_proxy, DeviceProxy):
            return
        get_manager().shutdownDevice(self._showing_proxy.device_id,
                                     parent=self)

    @Slot()
    def _on_init_device(self):
        """Instantiate a project device from the configurator"""
        proxy = self._showing_proxy
        if not isinstance(proxy, ProjectDeviceProxy):
            # This should not happen, however we protect here.
            return

        serverId = proxy.server_id
        classId = proxy.binding.class_id
        deviceId = proxy.device_id
        get_manager().initDevice(serverId, classId, deviceId)

    @Slot()
    def _on_config_from_past(self) -> None:
        if not isinstance(self._showing_proxy,
                          (DeviceProxy, ProjectDeviceProxy)):
            return
        device_id = self._showing_proxy.device_id
        dialog = ConfigurationFromPastDialog(
            instance_id=device_id, parent=self)
        dialog.move(QCursor.pos())
        dialog.show()
        dialog.raise_()
        dialog.activateWindow()

    @Slot(bool)
    def _switch_mode_toggled(self, expert):
        """Change the mode setting of the tree widget"""
        tree_widget = self._stacked_tree_widgets.widget(CONFIGURATION_PAGE)
        tree_widget.setMode(expert)

    @Slot()
    def _on_compare_configuration(self) -> None:
        if not isinstance(self._showing_proxy,
                          (DeviceProxy, ProjectDeviceProxy)):
            return
        device_id = self._showing_proxy.device_id
        dialog = DeviceSelectorDialog(device_id, parent=self)
        move_to_cursor(dialog)
        dialog.show()

    # -----------------------------------------------------------------------
    # Qt

    def closeEvent(self, event):
        super().closeEvent(event)
        if event.isAccepted():
            self.signalPanelClosed.emit(self.windowTitle())
            unregister_from_broadcasts(self.event_map)
