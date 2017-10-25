#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on May 9, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtGui import QDialog, QDialogButtonBox

from karabo.common.api import Capabilities
from karabo_gui.schema import Dummy
from karabo_gui.singletons.api import get_topology


class DeviceScenesDialog(QDialog):
    def __init__(self, device_id='', parent=None):
        """A dialog to load scenes provided by devices

        :param device_id: If provided, the single device to request scenes from
        :param parent: The parent of the dialog
        """
        super(DeviceScenesDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'device_scenes.ui')
        uic.loadUi(filepath, self)

        # The currently selected device configuration
        self._selected_device = None

        if device_id:
            # Add a single device and select it
            self.deviceNames.addItem(device_id)
            self.deviceNames.setCurrentItem(self.deviceNames.item(0))
        else:
            # Fill in the available devices
            self.deviceNames.addItems(self._find_devices_with_scenes())

        # Initialize the Ok button
        self._enable_ok_button()

    @property
    def device_id(self):
        item = self.deviceNames.currentItem()
        if item:
            return item.text()
        return ''

    @property
    def scene_name(self):
        item = self.sceneNames.currentItem()
        if item:
            return item.text()
        return ''

    def done(self, result):
        """Reimplement ``QDialog`` virtual slot"""
        self._clean_up()
        super(DeviceScenesDialog, self).done(result)

    def on_deviceNames_itemSelectionChanged(self):
        """A device was selected. Update the available scenes.
        """
        self.sceneNames.clear()

        item = self.deviceNames.currentItem()
        if item:
            self.sceneNames.addItems(self._get_device_scenes(item.text()))

    def on_sceneNames_itemSelectionChanged(self):
        """Synchronize the Ok button with the scene selection
        """
        self._enable_ok_button()

    def on_sceneNames_itemDoubleClicked(self, item):
        """Just accept immediately when a scene is double-clicked
        """
        self.accept()

    def _clean_up(self):
        """Clean up:
            - Qt signal/slot connections
            - Device configuration subscriptions
        """
        if self._selected_device is None:
            return

        box = self._selected_device
        box.signalUpdateComponent.disconnect(self._device_update)
        box.removeVisible()
        self._selected_device = None

    def _enable_ok_button(self):
        """Only enable Ok button when a scene is selected
        """
        enabled = len(self.sceneNames.selectedItems()) == 1
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enabled)

    def _find_devices_with_scenes(self):
        """Walk the system topology and collect IDs of devices with scenes on
        offer.
        """
        device_ids = []

        def _test_mask(mask, bit):
            return (mask & bit) == bit

        def visitor(node):
            nonlocal device_ids
            if _test_mask(node.capabilities, Capabilities.PROVIDES_SCENES):
                device_ids.append(node.node_id)

        get_topology().visit_system_tree(visitor)
        return device_ids

    def _get_device_scenes(self, device_id):
        """Walk the system topology and collect IDs of devices with scenes on
        offer.
        """
        device_configuration = get_topology().get_device(device_id)

        try:
            scenes = device_configuration.boxvalue.availableScenes.value
        except AttributeError:
            # This device lied about its capabilities!
            return []

        if isinstance(scenes, Dummy):
            # Not loaded yet. Request it.
            self._request_device_scenes(device_configuration)
            return []
        return scenes

    def _request_device_scenes(self, device_configuration):
        """Request the configuration from a device so that we can get the
        available scenes
        """
        self._clean_up()

        device_configuration.signalUpdateComponent.connect(self._device_update)
        device_configuration.addVisible()
        self._selected_device = device_configuration

    def _device_update(self, box, value, timestamp):
        if box is not self._selected_device:
            return

        self._clean_up()
        self.on_deviceNames_itemSelectionChanged()
