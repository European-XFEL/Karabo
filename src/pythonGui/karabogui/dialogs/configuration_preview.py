#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on April 23, 2021
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
import os.path as op

from qtpy import uic
from qtpy.QtCore import Qt, Slot
from qtpy.QtWidgets import QDialog, QDialogButtonBox
from traits.api import Undefined

from karabo.native import (
    AccessMode, Assignment, Hash, create_html_hash, writeXML)
from karabogui import icons, messagebox
from karabogui.binding.api import (
    ProjectDeviceProxy, extract_configuration, get_config_changes,
    iterate_binding)
from karabogui.singletons.api import get_config
from karabogui.util import SignalBlocker, getSaveFileName

from .utils import get_dialog_ui


class ConfigurationFromPastPreview(QDialog):
    """A simple configuration preview for configuration from name and time

    :param title: The title of the dialog
    :param info: The text info for the field (sub title). If not info is
                 provided, the field is invisible
    :param proxy: The proxy instance, can be from project or topology
    :param parent: The dialog parent
    """

    def __init__(self, title, info, configuration, proxy, parent=None):
        super().__init__(parent=parent)
        self._currently_reconfigurable = ""
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.setModal(False)
        ui_file = get_dialog_ui("configuration_past_preview.ui")
        uic.loadUi(ui_file, self)
        self.setWindowTitle(title)
        flags = Qt.WindowCloseButtonHint | Qt.WindowStaysOnTopHint
        self.setWindowFlags(self.windowFlags() | flags)

        if info is None:
            self.ui_info.setVisible(False)
        else:
            status_text = "online" if proxy.online else "offline"
            info = f"{info} The device is {status_text}."
            self.ui_info.setText(info)

        self._show_changes = False
        self.ui_swap.setIcon(icons.change)
        self.ui_swap.clicked.connect(self._swap_view)
        ok_button = self.ui_buttonBox.button(QDialogButtonBox.Ok)
        ok_button.clicked.connect(self.accept)

        cancel_button = self.ui_buttonBox.button(QDialogButtonBox.Cancel)
        cancel_button.clicked.connect(self.reject)

        self.ui_save_configuration.clicked.connect(
            self.save_configuration_to_file)
        self.ui_save_configuration.setIcon(icons.save)

        self.proxy = proxy
        self.configuration = configuration
        self._configuration_by_access_mode()
        self._show_configuration()
        self._load_configuration_changes()
        self.ui_synchronize_bars.toggled.connect(
            self._synchronize_toggled)
        self._synchronize_toggled(True)
        self.ui_hide_readonly.toggled.connect(self._show_configuration_changes)
        self._show_configuration_changes(hide_readonly=False)
        self.ui_reconfigurable_only.setVisible(self.proxy.online)
        self.ui_config_count.setVisible(self.proxy.online)
        self.ui_reconfigurable_only.toggled.connect(
            self._show_reconfigurable_only)

    def _configuration_by_access_mode(self):
        reconfig_text = "No Reconfigurable Property Available"
        if len(self.proxy.binding.value):
            if self.proxy.online:
                reconfig, current_reconfig = \
                    self._extract_online_reconfigurable()
                self._currently_reconfigurable = create_html_hash(
                    current_reconfig)
                allowed_count = len(current_reconfig.paths(intermediate=False))
                reconfig_count = len(reconfig.paths(intermediate=False))
                text = (f"Showing {reconfig_count} reconfigurable parameters "
                        f"and {allowed_count} are currently allowed.")
                self.ui_config_count.setText(text)
            else:
                reconfig = self._extract_offline_reconfigurable()
            reconfig_text = create_html_hash(reconfig)
            self._reconfigurable_props = reconfig_text
        self.ui_text_info_configurable.setHtml(reconfig_text)

    def _load_configuration_changes(self):
        """Store the changed configuration """
        if not len(self.proxy.binding.value):
            # Note: We are protected by the menu bar that does not allow us
            # to be launched in this case. However, we protect again here...
            self.all_changes_existing = self.configurable_changes_existing = (
                "No schema available to extract a configuration")
            self.configurable_changes_existing = self.all_changes_retrieved = (
                "No schema available for comparing configurations!")
            return

        project = isinstance(self.proxy, ProjectDeviceProxy)
        existing = extract_configuration(self.proxy.binding)
        changes_a, changes_b = get_config_changes(existing, self.configuration,
                                                  project)
        self.all_changes_existing = create_html_hash(
            changes_a, include_attributes=False)
        self.all_changes_retrieved = create_html_hash(
            changes_b, include_attributes=False)

        configurable_changes_a, configurable_changes_b = \
            self._skip_read_only_changes(changes_a, changes_b)
        self.configurable_changes_existing = create_html_hash(
            configurable_changes_a, include_attributes=False)
        self.configurable_changes_retrieved = create_html_hash(
            configurable_changes_b, include_attributes=False)

    def _show_configuration(self):
        html = create_html_hash(self.configuration, include_attributes=False)
        self.ui_text_info_all.setHtml(html)

    def _skip_read_only_changes(self, changes_a: Hash, changes_b: Hash) -> \
            tuple[Hash, Hash]:
        """Remove the read only properties from the given Configurations"""
        read_only_changes_a, read_only_changes_b = Hash(), Hash()
        paths_a = changes_a.paths(intermediate=False)
        paths_b = changes_b.paths(intermediate=False)
        keys = sorted(set(paths_a).union(set(paths_b)))
        for key in keys:
            if key in self._read_only_props:
                continue
            read_only_changes_a[key] = changes_a[key]
            read_only_changes_b[key] = changes_b[key]
        return read_only_changes_a, read_only_changes_b

    def _extract_offline_reconfigurable(self):
        """
        Extract the readonly and reconfigurable property from the configuration
        for an offline device. Reconfigurable properties includes the init only
        properties as well.
        """
        binding = self.proxy.binding
        read_only = Hash()
        reconfigurable = Hash()

        for key, node in iterate_binding(binding):
            value = self.configuration.get(key, None)
            if value is None or value is Undefined:
                continue

            accessMode = node.accessMode
            if accessMode in (
                    AccessMode.RECONFIGURABLE, AccessMode.INITONLY) and (
                    node.assignment != Assignment.INTERNAL):
                reconfigurable[key] = value
            else:
                read_only[key] = value
        self._read_only_props = read_only
        return reconfigurable

    def _extract_online_reconfigurable(self):
        """
        Extract the readonly and reconfigurable property from the configuration
        for an online device. Consider the 'allowedStates' attribute to
        determine if the property is reconfigurable or not allowed in the
        current state of the device.
        """

        device_state = self.proxy.state_binding
        binding = self.proxy.binding
        read_only = Hash()
        reconfigurable = Hash()
        currently_reconfigurable = Hash()

        for key, node in iterate_binding(binding):
            value = self.configuration.get(key, None)
            if value is None or value is Undefined:
                continue

            accessMode = node.accessMode
            if accessMode == AccessMode.RECONFIGURABLE:
                reconfigurable[key] = value
                if node.is_allowed(state=device_state):
                    currently_reconfigurable[key] = value
            else:
                read_only[key] = value
        self._read_only_props = read_only
        return reconfigurable, currently_reconfigurable

    # ---------------------------------------------------------------------
    # Slot Interface

    @Slot(bool)
    def _synchronize_toggled(self, enabled):
        """Internal method to synchronize the scrollbars"""
        existing = self.ui_existing.verticalScrollBar()
        retrieved = self.ui_retrieved.verticalScrollBar()
        if enabled:
            existing.valueChanged.connect(self.existing_bar_moved)
            retrieved.valueChanged.connect(self.retrieved_bar_moved)
        else:
            existing.valueChanged.disconnect(self.existing_bar_moved)
            retrieved.valueChanged.disconnect(self.retrieved_bar_moved)

    @Slot(int)
    def existing_bar_moved(self, value):
        with SignalBlocker(self.ui_retrieved):
            self.ui_retrieved.verticalScrollBar().setValue(value)

    @Slot(int)
    def retrieved_bar_moved(self, value):
        with SignalBlocker(self.ui_existing):
            self.ui_existing.verticalScrollBar().setValue(value)

    @Slot()
    def _swap_view(self):
        self._show_changes = not self._show_changes
        text = "Show Configuration" if self._show_changes else "Show Changes"
        self.ui_swap.setText(text)
        self.ui_stack_widget.setCurrentIndex(int(self._show_changes))

        text = "Changes" if self._show_changes else "Retrieved Configuration"
        self.ui_show.setText(text)

    @Slot()
    def save_configuration_to_file(self):
        if self.configuration.empty():
            messagebox.show_error("The configuration is empty and cannot "
                                  "be saved", title='No configuration',
                                  parent=self)
            return

        config = get_config()
        path = config['data_dir']
        directory = path if path and op.isdir(path) else ""

        class_id = self.configuration.get("classId", "unknown-class")

        # Use deviceId for the name!
        default_name = self.configuration.get('deviceId')
        if default_name:
            default_name = default_name.replace('/', '-') + '.xml'
        else:
            default_name = class_id + '.xml'

        filename = getSaveFileName(caption="Save configuration as",
                                   filter="Configuration (*.xml)",
                                   suffix="xml",
                                   directory=directory,
                                   selectFile=default_name,
                                   parent=self)
        if not filename:
            return

        config = Hash(class_id, self.configuration)

        # Save configuration to file
        with open(filename, 'w') as fp:
            writeXML(config, fp)

        # save the last config directory
        get_config()['data_dir'] = op.dirname(filename)

    @Slot(bool)
    def _show_configuration_changes(self, hide_readonly: bool) -> None:
        """Show the configuration in the widgets.Optionally hides the
        readonly properties"""
        if hide_readonly:
            html_a = self.configurable_changes_existing
            html_b = self.configurable_changes_retrieved
        else:
            html_a = self.all_changes_existing
            html_b = self.all_changes_retrieved
        self.ui_existing.setHtml(html_a)
        self.ui_retrieved.setHtml(html_b)

    @Slot(bool)
    def _show_reconfigurable_only(self, toggled: bool) -> None:
        """
        For online device, show either only the properties that are
        reconfigurable in the current state of the device or all
        reconfigurable properties.
        """
        text = (self._currently_reconfigurable if toggled else
                self._reconfigurable_props)
        self.ui_text_info_configurable.setHtml(text)
