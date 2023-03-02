from karabo.native import Configurable, Float
from karabogui.binding.api import (
    DeviceClassProxy, PropertyProxy, ProxyStatus, build_binding)
from karabogui.indicators import (
    ALL_OK_COLOR, PROPERTY_ALARM_COLOR, PROPERTY_WARN_COLOR)
from karabogui.testing import set_proxy_value

from ..label import DisplayAlarmFloat


class Object(Configurable):
    alarms = Float(
        displayType="format|fmt=f&decimals=4",
        alarmLow=-2.0, alarmHigh=2.0,
        warnLow=-1.0, warnHigh=1.0)


def test_alarm_float(gui_app):
    schema = Object.getClassSchema()
    binding = build_binding(schema)
    device = DeviceClassProxy(binding=binding, server_id="Fake",
                              status=ProxyStatus.OFFLINE)
    proxy = PropertyProxy(root_proxy=device, path="alarms")

    controller = DisplayAlarmFloat(proxy=proxy)
    # Initialize model for testing
    model = controller.model
    controller.initialize_model(proxy, model)
    controller.create(None)

    assert controller.fmt == "{:.4f}"
    assert model.warnLow == -1.0
    assert model.warnHigh == 1.0
    assert model.alarmHigh == 2.0
    assert model.alarmLow == -2.0

    set_proxy_value(proxy, "alarms", 0.75)
    assert controller.internal_widget.text() == "0.7500"
    assert controller.bg_color == ALL_OK_COLOR
    set_proxy_value(proxy, "alarms", 3.0)
    assert controller.bg_color == PROPERTY_ALARM_COLOR
    set_proxy_value(proxy, "alarms", 0.75)
    assert controller.bg_color == ALL_OK_COLOR
    set_proxy_value(proxy, "alarms", 1.2)
    assert controller.bg_color == PROPERTY_WARN_COLOR
