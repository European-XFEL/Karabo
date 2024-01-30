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
            assert not dialog.isModal()
            # We have 1 device in the QListWidget found in topology
            assert dialog.deviceNames.count() == 1

            # Selection of device
            assert dialog.device_id == ""
            dialog.deviceNames.setCurrentRow(0)
            dialog.on_deviceNames_itemSelectionChanged()
            assert dialog.device_id == "divvy"

            # Device in topology with default 'scene`
            device = topology.get_device("divvy")
            build_binding(get_device_schema(), device.binding)
            apply_default_configuration(device.binding)

            # Scene is available and can be selected
            dialog.on_deviceNames_itemSelectionChanged()
            assert dialog.capaNames.count() == 1
            dialog.capaNames.setCurrentRow(0)
            assert dialog.capa_name == "scene"

            # Finish dialog with success!
            dialog.done(1)

            # 2. Test with a deviceId in the beginning!
            dialog = DeviceCapabilityDialog(device_id="divvy")
            assert not dialog.isModal()
            # We have 1 device in the QListWidget found in topology
            assert dialog.deviceNames.count() == 1
            assert dialog.device_id == "divvy"

            # Finish dialog declined!
            dialog.done(0)


if __name__ == "__main__":
    main()
