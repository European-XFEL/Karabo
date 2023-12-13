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

from qtpy.QtCore import QPoint, QRect
from qtpy.QtWidgets import QWidget

import karabogui.const as global_constants
from karabo.common.api import InstanceStatus
from karabo.common.scenemodel.api import InstanceStatusModel
from karabogui.binding.api import DeviceProxy, ProxyStatus
from karabogui.events import KaraboEvent
from karabogui.testing import check_renderer_against_svg, click_button
from karabogui.topology.api import SystemTreeNode

from ..status import (
    SVG_ERROR, SVG_OFFLINE, SVG_OK, SVG_UNKNOWN, InstanceStatusWidget)


def test_instance_info_widget(gui_app, mocker):
    path = "karabogui.sceneview.widget.status.get_topology"
    topo = mocker.Mock()
    mocker.patch(path).return_value = topo
    root_proxy = DeviceProxy(device_id="XHQ_EH/MDL/STATUS")
    node = SystemTreeNode()
    node.attributes = {"type": "device", "status": InstanceStatus.OK.value}
    root_proxy.status = ProxyStatus.OFFLINE
    topo.get_device.return_value = root_proxy

    model = InstanceStatusModel(
        x=0, y=0, width=100, height=100,
        keys=["XHQ_EH/MDL/STATUS.deviceId"])

    main_widget = QWidget()
    widget = InstanceStatusWidget(model=model,
                                  parent=main_widget)
    main_widget.show()

    model_rect = QRect(model.x, model.y,
                       model.width, model.height)
    assert widget.rect() == model_rect
    assert len(widget.actions()) == 0

    # Geometry
    rect = QRect(2, 10, 2, 10)
    assert widget.geometry() != rect
    widget.set_geometry(rect)
    assert widget.geometry() == rect

    # Translation
    assert widget.pos() == QPoint(2, 10)
    widget.translate(QPoint(10, 0))
    assert widget.pos() == QPoint(12, 10)

    def assert_widget(text, filename):
        assert widget.getToolTip() == f"XHQ_EH/MDL/STATUS - {text}"
        assert widget.renderer().isValid()
        check_renderer_against_svg(widget.renderer(), filename)

    assert not widget.proxy.online
    assert_widget("OFFLINE", SVG_OFFLINE)

    root_proxy.topology_node = node
    node.status = InstanceStatus.OK
    root_proxy.status = ProxyStatus.ONLINE
    assert widget.proxy.online
    assert_widget("OK", SVG_OK)

    node.status = InstanceStatus.OK
    assert_widget("OK", SVG_OK)
    node.status = InstanceStatus.ERROR
    assert_widget("ERROR", SVG_ERROR)
    node.status = InstanceStatus.UNKNOWN
    assert_widget("UNKNOWN", SVG_UNKNOWN)
    node.status = InstanceStatus.OK
    assert_widget("OK", SVG_OK)
    # offline again
    root_proxy.status = ProxyStatus.OFFLINE
    assert_widget("OFFLINE", SVG_OFFLINE)

    path = "karabogui.sceneview.widget.status.broadcast_event"
    broadcast = mocker.patch(path)

    click_button(widget)
    if not global_constants.APPLICATION_MODE:
        broadcast.assert_called_with(KaraboEvent.ShowDevice,
                                     {"deviceId": "XHQ_EH/MDL/STATUS"})
    else:
        broadcast.assert_called_with(
            KaraboEvent.RaiseEditor, {"proxy": root_proxy})

    # Make sure, basic interface is there
    widget.add_proxies(None)
    widget.apply_changes()
    widget.decline_changes()
    widget.set_visible(True)
    widget.update_global_access_level(None)
    widget.destroy()
