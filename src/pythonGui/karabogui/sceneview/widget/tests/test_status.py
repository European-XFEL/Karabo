from unittest.mock import Mock

from qtpy.QtCore import QPoint, QRect
from qtpy.QtWidgets import QWidget

from karabo.common.api import InstanceStatus
from karabo.common.scenemodel.api import InstanceStatusModel
from karabogui.binding.api import DeviceProxy, ProxyStatus
from karabogui.testing import check_renderer_against_svg
from karabogui.topology.api import SystemTreeNode

from ..status import (
    SVG_ERROR, SVG_OFFLINE, SVG_OK, SVG_UNKNOWN, InstanceStatusWidget)


def test_instance_info_widget(gui_app, mocker):

    path = "karabogui.sceneview.widget.status.get_topology"
    topo = Mock()
    mocker.patch(path).return_value = topo
    root_proxy = DeviceProxy(device_id='XHQ_EH/MDL/STATUS')
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

    # Make sure, basic interface is there
    widget.add_proxies(None)
    widget.apply_changes()
    widget.decline_changes()
    widget.set_visible(True)
    widget.update_global_access_level(None)
    widget.destroy()
