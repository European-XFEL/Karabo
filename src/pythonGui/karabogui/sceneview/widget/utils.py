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
from karabo.native import AccessLevel
from karabogui.binding.api import PropertyProxy
from karabogui.singletons.api import get_topology


def get_proxy(device_id: str, path: str) -> PropertyProxy:
    """Return a `PropertyProxy` instance for a given device and property path.
    """
    device_proxy = get_topology().get_device(device_id)
    return PropertyProxy(root_proxy=device_proxy, path=path)


def is_controller_enabled(proxy: PropertyProxy, level: AccessLevel) -> bool:
    """Check if the controller with `proxy` is enabled"""
    root_proxy = proxy.root_proxy
    state_binding = root_proxy.state_binding
    if state_binding is None:
        return False
    value = state_binding.value
    if not value:
        return False

    binding = proxy.binding
    is_allowed = binding.is_allowed(value)
    is_accessible = level >= binding.requiredAccessLevel
    return is_accessible and is_allowed
