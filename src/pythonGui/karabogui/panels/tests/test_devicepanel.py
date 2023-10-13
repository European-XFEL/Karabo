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
from karabogui.panels.devicepanel import DevicePanel
from karabogui.singletons.api import get_config
from karabogui.singletons.configuration import Configuration
from karabogui.testing import singletons, system_hash
from karabogui.topology.api import SystemTopology


def test_device_topology_panel(gui_app, mocker):
    mediator = mocker.Mock()
    topology = SystemTopology()
    topology.initialize(system_hash())
    config = Configuration()
    with singletons(mediator=mediator, topology=topology,
                    configuration=config):
        panel = DevicePanel()
        assert repr(panel) == "<DevicePanel topic=None>"
        get_config()["broker_topic"] = "Test"
        assert repr(panel) == "<DevicePanel topic=Test>"
