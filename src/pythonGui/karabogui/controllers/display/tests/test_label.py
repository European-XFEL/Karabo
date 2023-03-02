from karabo.native import Configurable, Float, String
from karabogui.binding.api import (
    DeviceClassProxy, PropertyProxy, ProxyStatus, build_binding)
from karabogui.indicators import (
    ALL_OK_COLOR, PROPERTY_ALARM_COLOR, PROPERTY_WARN_COLOR)
from karabogui.testing import GuiTestCase, set_proxy_value

from ..label import DisplayLabel


class Object(Configurable):
    string = String()
    alarms = Float(alarmLow=-2.0, alarmHigh=2.0,
                   warnLow=-1.0, warnHigh=1.0)
    faulty = Float(displayType="fmt|{:*1271f}")


class TestDisplayLabel(GuiTestCase):
    def setUp(self):
        super(TestDisplayLabel, self).setUp()

        schema = Object.getClassSchema()
        binding = build_binding(schema)
        device = DeviceClassProxy(binding=binding, server_id="Fake",
                                  status=ProxyStatus.OFFLINE)
        self.string = PropertyProxy(root_proxy=device, path="string")
        self.alarms = PropertyProxy(root_proxy=device, path="alarms")
        self.faulty = PropertyProxy(root_proxy=device, path="faulty")

    def test_basics(self):
        controller = DisplayLabel(proxy=self.string)
        controller.create(None)
        assert controller.widget is not None

        controller.destroy()
        assert controller.widget is None

    def test_set_string_value(self):
        controller = DisplayLabel(proxy=self.string)
        controller.create(None)
        set_proxy_value(self.string, "string", "hello")
        assert controller.internal_widget.text() == "hello"

    def test_alarm_color(self):
        controller = DisplayLabel(proxy=self.alarms)
        controller.create(None)
        set_proxy_value(self.alarms, 'alarms', 0.75)
        assert controller.internal_widget.text() == '0.75'
        assert controller.bg_color == ALL_OK_COLOR
        set_proxy_value(self.alarms, 'alarms', 3.0)
        assert controller.bg_color == PROPERTY_ALARM_COLOR
        set_proxy_value(self.alarms, 'alarms', 1.5)
        assert controller.bg_color == PROPERTY_WARN_COLOR

    def test_wrong_format(self):
        controller = DisplayLabel(proxy=self.faulty)
        controller.create(None)
        set_proxy_value(self.faulty, "faulty", 0.25)
        assert controller.internal_widget.text() == "0.25"
