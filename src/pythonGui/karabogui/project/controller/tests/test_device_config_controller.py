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
from karabo.native import AccessLevel, Hash
from karabogui.binding.api import ProxyStatus
from karabogui.project.controller.device import DeviceInstanceController
from karabogui.project.controller.device_config import (
    DeviceConfigurationController)
from karabogui.testing import singletons, system_hash
from karabogui.topology.system_topology import SystemTopology

IPATH = 'karabogui.project.controller.device.get_project_device_status_icon'
PARENT_OBJECT = 'karabogui.project.controller.device_config.find_parent_object'


def test_configuration_schema(gui_app, mocker):
    patch_func = mocker.patch(PARENT_OBJECT)
    network = mocker.Mock()
    topology = SystemTopology()
    topology.initialize(system_hash())
    with singletons(network=network, topology=topology):
        config = Hash()
        config['value'] = "a"
        config_model = DeviceConfigurationModel(class_id='BazClass',
                                                configuration=config)
        config_model.initialized = True
        config_controller = DeviceConfigurationController(model=config_model)
        config_list = [config_model]
        instance_model = DeviceInstanceModel(
            class_id='BazClass',
            instance_id='fooDevice',
            server_id='BazServer',
            configs=config_list)
        instance_model.initialized = True
        instance_model.active_config_ref = config_model.uuid

        icon_mock = mocker.patch(IPATH)
        icon_mock.return_value = None
        device_controller = DeviceInstanceController(model=instance_model)

        device = device_controller.project_device
        assert network.onGetClassSchema.call_count == 0
        assert device.proxy.status == ProxyStatus.NOSERVER
        assert not device.proxy.online
        patch_func.return_value = device_controller
        config_controller.single_click(None)
        assert network.onGetClassSchema.call_count == 0
        h = Hash()
        h['server.BazServer'] = None
        h['server.BazServer', ...] = {'host': 'BIG_IRON',
                                      'visibility': AccessLevel.OBSERVER,
                                      'deviceClasses': ['BazClass'],
                                      'visibilities': [
                                          AccessLevel.OBSERVER]}
        changes = Hash("new", h, "update", Hash(), "gone", Hash())
        topology.topology_update(changes)
        assert device.proxy.status == ProxyStatus.OFFLINE
        assert network.onGetClassSchema.call_count == 0
        config_controller.single_click(None)
        assert network.onGetClassSchema.call_count == 1
