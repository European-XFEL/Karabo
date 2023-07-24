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
from traits.api import Undefined

from karabogui.binding.api import apply_default_configuration, build_binding
from karabogui.testing import get_all_props_schema, system_hash
from karabogui.topology.api import SystemTopology

from ..utils import compare_proxy_essential, extract_proxy_info


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


def test_extract_proxy_info():
    schema = get_all_props_schema()
    binding = build_binding(schema)

    assert binding.value.a.value is Undefined
    assert binding.value.b.value is Undefined
    assert binding.value.c.value is Undefined

    apply_default_configuration(binding)

    assert binding.value.a.value
    assert binding.value.b.value == "c"
    assert binding.value.c.value is Undefined

    # Make sure the extracted default conversion is minimal
    # It should include properties with default values, options, or node types
    config = extract_proxy_info(binding)
    default_props = ("a", "b", "h1", "i1", "j1")
    for prop in default_props:
        assert prop != "Undefined"

    # List and Choice of Node and not present
    assert "i1" not in config
    assert "j1" not in config

    # Slot k1 is not present
    assert "k1" not in config

    # m has no default value, but present with Undefined
    assert "m" in config
    assert config["m"] == "Undefined"

    assert "c" in config
    assert config["c"] == "Undefined"
