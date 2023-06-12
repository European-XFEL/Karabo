#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on May 9, 2017
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
from qtpy import uic
from qtpy.QtCore import Qt, Slot
from qtpy.QtWidgets import QDialog, QDialogButtonBox, QListWidgetItem
from traits.api import Undefined

from karabo.common.api import Capabilities
from karabogui.singletons.api import get_topology

from .utils import get_dialog_ui


class DeviceCapabilityDialog(QDialog):
    def __init__(self, device_id="", capability=Capabilities.PROVIDES_SCENES,
                 parent=None):
        """A dialog to load capability items provided by devices

        :param device_id: If provided, the single device to request items from
        :param parent: The parent of the dialog
        """
        super().__init__(parent)
        filepath = get_dialog_ui("device_capability.ui")
        uic.loadUi(filepath, self)

        self.capability = capability
        # The currently selected device configuration
        self._selected_device = None
        self._requested_capa = None

        if device_id:
            # Add a single device and select it
            self.deviceNames.addItem(device_id)
            self.deviceNames.setCurrentItem(self.deviceNames.item(0))
            self.device_filter.setVisible(False)
            self.clear_button.setVisible(False)
        else:
            # Fill in the available devices
            self.topology = self._find_devices_with_capability()
            self.deviceNames.addItems(self.topology)

        # Initialize the Ok button
        self._enable_ok_button()

    @property
    def device_id(self):
        item = self.deviceNames.currentItem()
        if item:
            return item.text()
        return ""

    @property
    def capa_name(self):
        item = self.capaNames.currentItem()
        if item:
            return item.text()
        return ""

    def done(self, result):
        """Reimplement ``QDialog`` virtual slot"""
        self._clean_up()
        super().done(result)

    @Slot(str)
    def on_device_filter_textChanged(self, text):
        """Perform a case insensitive filtering on the topology"""
        topo = [device for device in self.topology if text.lower()
                in device.lower()]
        self.deviceNames.clearSelection()
        self.deviceNames.clear()
        self.deviceNames.addItems(topo)
        self.capaNames.clear()

    @Slot()
    def on_deviceNames_itemSelectionChanged(self):
        """A device was selected. Update the available capabilities.
        """
        self.capaNames.clear()

        item = self.deviceNames.currentItem()
        if item:
            provided_item = self._get_device_items(item.text())
            self.capaNames.addItems(provided_item)

    @Slot()
    def on_capaNames_itemSelectionChanged(self):
        """Synchronize the Ok button with the capability selection
        """
        self._enable_ok_button()

    @Slot(QListWidgetItem)
    def on_capaNames_itemDoubleClicked(self, item):
        """Just accept immediately when a capability is double-clicked"""
        self.accept()

    # -----------------------------------------------------------------------
    # Public interface

    def find_and_select(self, name):
        """Public interface to find and selected the capa name `name`"""
        self._requested_capa = name
        self._set_requested_capa()

    # -----------------------------------------------------------------------

    def _set_requested_capa(self):
        name = self._requested_capa
        if name is None:
            return

        items = self.capaNames.findItems(name, Qt.MatchExactly)
        if items:
            self.capaNames.setCurrentItem(items[0])
            self._requested_capa = None

    def _clean_up(self):
        """Clean up:
            - Trait notifications
            - Device monitor subscriptions
        """
        if self._selected_device is None:
            return

        device_proxy = self._selected_device
        device_proxy.on_trait_change(self._device_update, "config_update",
                                     remove=True)
        device_proxy.remove_monitor()
        self._selected_device = None

    def _enable_ok_button(self):
        """Only enable Ok button when a capability is selected
        """
        enabled = len(self.capaNames.selectedItems()) == 1
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enabled)

    def _find_devices_with_capability(self):
        """Walk the system topology and collect IDs of devices with capability
        on offer.
        """
        device_ids = []

        def _test_mask(mask, bit):
            return (mask & bit) == bit

        def visitor(node):
            nonlocal device_ids
            if _test_mask(node.capabilities, self.capability):
                device_ids.append(node.node_id)

        get_topology().visit_system_tree(visitor)
        device_ids.sort()

        return device_ids

    def _get_device_items(self, device_id):
        """Walk the system topology and collect IDs of devices with capability
        on offer.
        """
        device = get_topology().get_device(device_id)
        try:
            if self.capability is Capabilities.PROVIDES_SCENES:
                items = device.binding.value.availableScenes.value
            elif self.capability is Capabilities.PROVIDES_MACROS:
                items = device.binding.value.availableMacros.value
        except AttributeError:
            # This device lied about its capabilities or the information was
            # not yet received!
            binding = device.binding.value
            if not len(binding):
                self._request_device_capabilities(device)
            return []
        if items == [] or items is Undefined:
            # Not loaded yet. Request it.
            self._request_device_capabilities(device)
            # and return for now an empty list
            return []

        return items

    def _request_device_capabilities(self, device_proxy):
        """Request the configuration from a device so that we can get the
        available capabilities
        """
        self._clean_up()

        device_proxy.on_trait_change(self._device_update, "config_update")
        device_proxy.add_monitor()
        self._selected_device = device_proxy

    def _device_update(self, obj, name, new):
        if obj is not self._selected_device:
            return

        self._clean_up()
        self.on_deviceNames_itemSelectionChanged()
        self._set_requested_capa()
