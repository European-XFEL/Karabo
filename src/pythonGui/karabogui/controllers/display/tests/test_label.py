from karabo.common.api import DeviceStatus
from karabo.middlelayer import Configurable, Float, String
from karabogui.alarms.api import ALARM_COLOR, WARN_COLOR
from karabogui.binding.api import (
    DeviceClassProxy, PropertyProxy, build_binding
)
from karabogui.const import FINE_COLOR
from karabogui.testing import GuiTestCase
from ..label import DisplayLabel, ALPHA


class Object(Configurable):
    string = String()
    alarms = Float(alarmLow=-2.0, alarmHigh=2.0,
                   warnLow=-1.0, warnHigh=1.0)
    absolute = Float(absoluteError=0.1)
    relative = Float(relativeError=0.5)


class TestDisplayLabel(GuiTestCase):
    def setUp(self):
        super(TestDisplayLabel, self).setUp()

        schema = Object.getClassSchema()
        binding = build_binding(schema)
        device = DeviceClassProxy(binding=binding, server_id='Fake',
                                  status=DeviceStatus.OFFLINE)
        self.string = PropertyProxy(root_proxy=device, path='string')
        self.alarms = PropertyProxy(root_proxy=device, path='alarms')
        self.absolute = PropertyProxy(root_proxy=device, path='absolute')
        self.relative = PropertyProxy(root_proxy=device, path='relative')

    def test_basics(self):
        controller = DisplayLabel(proxy=self.string)
        controller.create(None)
        assert controller.widget is not None

        controller.destroy()
        assert controller.widget is None

    def test_set_string_value(self):
        controller = DisplayLabel(proxy=self.string)
        controller.create(None)
        self.string.value = 'hello'
        assert controller._internal_widget.text() == 'hello'

    def test_set_abs_err(self):
        controller = DisplayLabel(proxy=self.absolute)
        controller.create(None)
        self.absolute.value = 0.25
        assert controller._internal_widget.text() == '0.2'

    def test_set_rel_err(self):
        controller = DisplayLabel(proxy=self.relative)
        controller.create(None)
        self.relative.value = 0.75
        assert controller._internal_widget.text() == '0.8'

    def test_set_no_err(self):
        controller = DisplayLabel(proxy=self.alarms)
        controller.create(None)

        self.alarms.value = 0.75
        assert controller._internal_widget.text() == '0.75'
        assert controller._bg_color == FINE_COLOR

        self.alarms.value = 3.0
        assert controller._bg_color == ALARM_COLOR + ALPHA

        self.alarms.value = 1.5
        assert controller._bg_color == WARN_COLOR + ALPHA
