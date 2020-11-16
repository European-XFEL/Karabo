from platform import system

from karabo.common.scenemodel.api import SCENE_DEFAULT_DPI, SCENE_MAC_DPI
from karabogui.binding.api import PropertyProxy
from karabogui.singletons.api import get_topology

SCENE_DPI_FACTOR = (
    SCENE_DEFAULT_DPI / SCENE_MAC_DPI if system() == "Darwin" else 1)


def get_proxy(device_id, path):
    """Return a `PropertyProxy` instance for a given device and property path.
    """
    device_proxy = get_topology().get_device(device_id)
    return PropertyProxy(root_proxy=device_proxy, path=path)


def get_size_from_dpi(size):
    """Calculates the effective DPI from the OS DPI. As Qt and the majority
    of the OSes use DPI=96, we should add some scaling factor on the sizes
    for Mac DPI, as it has DPI=72."""
    return size * SCENE_DPI_FACTOR
