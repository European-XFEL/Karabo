#############################################################################
# Author: <alessandro.silenzi@xfel.eu>
# Created on November 23, 2022
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from pathlib import Path

from qtpy.QtSvg import QSvgWidget

from karabo.common.api import InstanceStatus
from karabogui import icons
from karabogui.binding.api import ProxyStatus
from karabogui.singletons.api import get_topology
from karabogui.widgets.hints import KaraboSceneWidget

ICON_PATH = Path(icons.__file__).parent

SVG_OFFLINE = str(ICON_PATH.joinpath("statusOffline.svg"))
SVG_OK = str(ICON_PATH.joinpath("statusOk.svg"))
SVG_ERROR = str(ICON_PATH.joinpath("statusError.svg"))
SVG_UNKNOWN = str(ICON_PATH.joinpath("statusUnknown.svg"))

SVG_STATUS = {
    InstanceStatus.OK: SVG_OK,
    InstanceStatus.UNKNOWN: SVG_UNKNOWN,
    InstanceStatus.ERROR: SVG_ERROR,
    InstanceStatus.NONE: SVG_OFFLINE
}


class InstanceStatusWidget(KaraboSceneWidget, QSvgWidget):
    """A widget to display a device status on the scene"""

    def __init__(self, model, parent=None):
        self.device_id = model.keys[0].split(".", 1)[0]
        super().__init__(model=model, parent=parent)
        self.status = ProxyStatus.OFFLINE
        self.proxy = get_topology().get_device(self.device_id)
        self.proxy.on_trait_change(self.proxy_online_change, "online")
        self.proxy.on_trait_change(self.proxy_status_change,
                                   "topology_node:status")
        self.proxy_online_change(self.proxy.online)
        self.setGeometry(model.x, model.y, model.width, model.height)

    def getToolTip(self):
        return f"{self.device_id} - {self.status.name}"

    def proxy_online_change(self, online):
        if online and self.proxy.topology_node is not None:
            svg = SVG_STATUS[self.proxy.topology_node.status]
            self.status = self.proxy.topology_node.status
        else:
            svg = SVG_OFFLINE
            self.status = ProxyStatus.OFFLINE
        self.setToolTip(self.getToolTip())
        self.load(svg)

    def proxy_status_change(self, status):
        svg = SVG_STATUS[status]
        self.status = status
        self.setToolTip(self.getToolTip())
        self.load(svg)

    def set_geometry(self, rect):
        """Satisfy the informal widget interface."""
        self.model.trait_set(x=rect.x(), y=rect.y(),
                             width=rect.width(), height=rect.height())
        self.setGeometry(rect)

    def translate(self, offset):
        """Satisfy the informal widget interface."""
        new_pos = self.pos() + offset
        self.model.trait_set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)

    def add_proxies(self, proxies):
        """Satisfy the informal widget interface."""

    def apply_changes(self):
        """Satisfy the informal widget interface."""

    def decline_changes(self):
        """Satisfy the informal widget interface."""

    def destroy(self):
        """Satisfy the informal widget interface."""
        self.proxy.on_trait_change(self.proxy_status_change,
                                   "topology_node:status", remove=True)
        self.proxy.on_trait_change(self.proxy_online_change, "online",
                                   remove=True)

    def set_visible(self, visible):
        """Satisfy the informal widget interface."""

    def update_global_access_level(self, level):
        """Satisfy the informal widget interface."""
