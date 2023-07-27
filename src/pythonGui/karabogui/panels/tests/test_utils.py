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

from karabogui.testing import system_hash
from karabogui.topology.api import SystemTopology

from ..utils import compare_proxy_essential


def test_proxy_essential_compare():
    topology = SystemTopology()
    topology.initialize(system_hash())
    proxy = topology.get_project_device_proxy("divvy", "swerver", "FooClass")
    showing_proxy = topology.get_project_device_proxy("divvy", "swerver",
                                                      "FooClass")
    assert compare_proxy_essential(proxy, showing_proxy)

    # Project device and online device
    showing_proxy = topology.get_device("divvy", request=False)
    assert compare_proxy_essential(proxy, showing_proxy)

    # Different deviceId to compare
    showing_proxy = topology.get_device("divvyyyyy", request=False)
    assert not compare_proxy_essential(proxy, showing_proxy)

    # Two different serverids
    showing_proxy = topology.get_project_device_proxy("divvy", "swwww",
                                                      "FooClass")
    assert not compare_proxy_essential(proxy, showing_proxy)

    # Different classIds
    showing_proxy = topology.get_project_device_proxy("divvy", "swerver",
                                                      "BarClass")
    assert compare_proxy_essential(proxy, showing_proxy)
