# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabogui.binding.api import PropertyProxy
from karabogui.singletons.api import get_topology


def get_proxy(device_id, path):
    """Return a `PropertyProxy` instance for a given device and property path.
    """
    device_proxy = get_topology().get_device(device_id)
    return PropertyProxy(root_proxy=device_proxy, path=path)
