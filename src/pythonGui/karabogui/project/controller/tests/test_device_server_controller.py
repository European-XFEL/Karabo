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
from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, DeviceServerModel)
from karabo.native import Hash
from karabogui.project.api import DeviceServerController
from karabogui.testing import singletons, system_hash
from karabogui.topology.api import SystemTopology

from .utils import status_icon_path


def test_device_server_controller(mocker):
    """Test the device server controller"""
    device_icon = mocker.patch(status_icon_path("device"))
    device_icon.return_value = None
    server_icon = mocker.patch(status_icon_path("server"))
    server_icon.return_value = None
    network = mocker.Mock()
    topology = SystemTopology()
    topology.initialize(system_hash())
    with singletons(network=network, topology=topology):
        # 1. Foo device with config
        foo_config = DeviceConfigurationModel(
            class_id="BazClass",
            configuration=Hash("value", "1"))
        foo_config.initialized = True

        foo_model = DeviceInstanceModel(
            class_id="BazClass",
            instance_id="B/B/fooDevice1",
            server_id="testServer",
            configs=[foo_config])
        foo_model.initialized = True
        foo_model.active_config_ref = foo_config.uuid

        # 2. Bar device with config
        bar_config = DeviceConfigurationModel(
            class_id="BazClass",
            configuration=Hash("value", "2"))
        bar_config.initialized = True

        bar_model = DeviceInstanceModel(
            class_id="BazClass",
            server_id="testServer",
            instance_id="A/A/fooDevice2",
            configs=[bar_config])
        bar_model.initialized = True
        bar_model.active_config_ref = bar_config.uuid

        # 3. Server
        server_model = DeviceServerModel(
            server_id="testServer", host="serverFoo",
            devices=[foo_model, bar_model])
        server_model.initialized = True

        # 4. Controllers
        server_controller = DeviceServerController(
            model=server_model)

        assert server_controller.model.devices[0] == foo_model
        assert server_controller.model.devices[1] == bar_model

        server_controller._sort_alphabetically(None)

        # bar starts with A, foo with B
        assert server_controller.model.devices[0] == bar_model
        assert server_controller.model.devices[1] == foo_model

        server_controller._sort_devices_naming(2, None)

        # Sort by member, foo has 1, bar has 2
        assert server_controller.model.devices[0] == foo_model
        assert server_controller.model.devices[1] == bar_model

        server_controller._sort_devices_naming(1, None)

        # Sort by type, foo has B, bar has A
        assert server_controller.model.devices[0] == bar_model
        assert server_controller.model.devices[1] == foo_model

        server_controller._sort_devices_naming(0, None)

        # Sort by domain, but here nothing happens! bar starts with A
        # and foo with B
        assert server_controller.model.devices[0] == bar_model
        assert server_controller.model.devices[1] == foo_model

        # sort by member again foo has 1, bar has 2
        server_controller._sort_devices_naming(2, None)
        assert server_controller.model.devices[0] == foo_model
        assert server_controller.model.devices[1] == bar_model

        # Then sort by domain, changes are happening.
        server_controller._sort_devices_naming(0, None)
        assert server_controller.model.devices[0] == bar_model
        assert server_controller.model.devices[1] == foo_model
