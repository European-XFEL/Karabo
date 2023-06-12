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
from karabo.common.project.device import DeviceInstanceModel
from karabo.common.project.device_config import DeviceConfigurationModel
from karabo.common.project.server import DeviceServerModel
from karabo.native import Hash
from karabogui.project.controller.device import DeviceInstanceController
from karabogui.project.controller.server import DeviceServerController
from karabogui.singletons.mediator import Mediator
from karabogui.testing import singletons, system_hash
from karabogui.topology.system_topology import SystemTopology

from .utils import status_icon_path

PARENT_PATH = "karabogui.project.controller.device.find_parent_object"


def test_empty_fields_configurator(mocker):
    device_icon = mocker.patch(status_icon_path("device"))
    device_icon.return_value = None

    network = mocker.Mock()
    mediator = Mediator()
    topology = SystemTopology()
    topology.initialize(system_hash())
    with singletons(network=network, topology=topology, mediator=mediator):
        foo_config = Hash()
        foo_config["test1"] = "value1"
        foo_config["test2"] = "value2"
        foo_config = DeviceConfigurationModel(
            class_id="BazClass",
            configuration=foo_config)
        foo_config.initialized = True

        bar_config = Hash()
        bar_config["test1"] = "diff_value1"
        bar_config = DeviceConfigurationModel(
            class_id="BazClass",
            configuration=bar_config)
        bar_config.initialized = True

        config_list = [foo_config, bar_config]

        device_model = DeviceInstanceModel(
            class_id="BazClass",
            server_id="testServer",
            instance_id="fooDevice",
            configs=config_list)
        device_model.initialized = True
        device_model.active_config_ref = foo_config.uuid

        def assert_active_configuration():
            assert "test1" in foo_config.configuration
            assert foo_config.configuration["test1"] == "value1"

            assert "test2" in foo_config.configuration
            assert foo_config.configuration["test2"] == "value2"

            assert "test1" in bar_config.configuration
            assert bar_config.configuration["test1"] == "diff_value1"

            assert "test2" not in bar_config.configuration

        controller = DeviceInstanceController(model=device_model)
        controller.active_config_changed(bar_config)
        assert_active_configuration()

        controller.active_config_changed(foo_config)
        assert_active_configuration()

        controller.active_config_changed(bar_config)
        assert_active_configuration()


def test_move_devices(mocker):
    device_icon = mocker.patch(status_icon_path("device"))
    device_icon.return_value = None
    server_icon = mocker.patch(status_icon_path("device"))
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
            instance_id="fooDevice",
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
            instance_id="fooDevice2",
            configs=[bar_config])
        bar_model.initialized = True
        bar_model.active_config_ref = bar_config.uuid

        # 3. Server
        server_model = DeviceServerModel(
            server_id="testServer", host="serverFoo",
            devices=[foo_model, bar_model])
        server_model.initialized = True

        # 4. Controllers
        foo_controller = DeviceInstanceController(model=foo_model)
        bar_controller = DeviceInstanceController(model=bar_model)
        server_controller = DeviceServerController(model=server_model)

        assert server_controller.model.devices[0] == foo_model
        assert server_controller.model.devices[1] == bar_model

        # 5. Fake project class for testing with a model member
        class Project:
            model = None

        project = Project()

        find_parent = mocker.patch(PARENT_PATH)
        find_parent.return_value = server_model
        foo_controller._move_down(project)

        assert server_controller.model.devices[0] == bar_model
        assert server_controller.model.devices[1] == foo_model

        foo_controller._move_up(project)

        assert server_controller.model.devices[0] == foo_model
        assert server_controller.model.devices[1] == bar_model

        bar_controller._move_up(project)

        assert server_controller.model.devices[0] == bar_model
        assert server_controller.model.devices[1] == foo_model

        # Lets try to crack it and move out of range (< 0)
        bar_controller._move_up(project)

        assert server_controller.model.devices[0] == bar_model
        assert server_controller.model.devices[1] == foo_model

        # Lets try to crack it and move out of range (> len)
        foo_controller._move_down(project)

        assert server_controller.model.devices[0] == bar_model
        assert server_controller.model.devices[1] == foo_model
