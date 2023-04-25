# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest import main, mock

from karabogui.binding.api import apply_default_configuration, build_binding
from karabogui.dialogs.api import DeviceCapabilityDialog
from karabogui.testing import (
    GuiTestCase, get_device_schema, singletons, system_hash)
from karabogui.topology.system_topology import SystemTopology


class TestCapabilityDialog(GuiTestCase):

    def test_basic_dialog(self):
        topology = SystemTopology()
        topology.initialize(system_hash())
        network = mock.Mock()
        with singletons(topology=topology, network=network):
            # 1. Test init without deviceId as argument
            dialog = DeviceCapabilityDialog()
            self.assertFalse(dialog.isModal())
            # We have 1 device in the QListWidget found in topology
            self.assertEqual(dialog.deviceNames.count(), 1)

            # Selection of device
            self.assertEqual(dialog.device_id, "")
            dialog.deviceNames.setCurrentRow(0)
            dialog.on_deviceNames_itemSelectionChanged()
            self.assertEqual(dialog.device_id, "divvy")

            # Device in topology with default 'scene`
            device = topology.get_device("divvy")
            build_binding(get_device_schema(), device.binding)
            apply_default_configuration(device.binding)

            # Scene is available and can be selected
            dialog.on_deviceNames_itemSelectionChanged()
            self.assertEqual(dialog.capaNames.count(), 1)
            dialog.capaNames.setCurrentRow(0)
            self.assertEqual(dialog.capa_name, "scene")

            # Finish dialog with success!
            dialog.done(1)

            # 2. Test with a deviceId in the beginning!
            dialog = DeviceCapabilityDialog(device_id="divvy")
            self.assertFalse(dialog.isModal())
            # We have 1 device in the QListWidget found in topology
            self.assertEqual(dialog.deviceNames.count(), 1)
            self.assertEqual(dialog.device_id, "divvy")

            # Finish dialog declined!
            dialog.done(0)


if __name__ == "__main__":
    main()
