from karabo.common.api import ProxyStatus
from karabo.native import Configurable, Float, String
from karabogui.binding.api import (
    DeviceClassProxy, PropertyProxy, build_binding)
from karabogui.indicators import (
    ALL_OK_COLOR, PROPERTY_ALARM_COLOR, PROPERTY_WARN_COLOR)
from karabogui.testing import GuiTestCase, set_proxy_value

from ..label import DisplayLabel


class Object(Configurable):
    string = String()
    alarms = Float(alarmLow=-2.0, alarmHigh=2.0,
                   warnLow=-1.0, warnHigh=1.0)
    absolute = Float(absoluteError=0.1)
    relative = Float(relativeError=0.5)

    # Bypass validation and modify class for testing
    absZero = Float()
    absZero.absoluteError = 0.0
    absNeg = Float()
    absNeg.absoluteError = -0.1


class TestDisplayLabel(GuiTestCase):
    def setUp(self):
        super(TestDisplayLabel, self).setUp()

        schema = Object.getClassSchema()
        binding = build_binding(schema)
        device = DeviceClassProxy(binding=binding, server_id='Fake',
                                  status=ProxyStatus.OFFLINE)
        self.string = PropertyProxy(root_proxy=device, path='string')
        self.alarms = PropertyProxy(root_proxy=device, path='alarms')
        self.absolute = PropertyProxy(root_proxy=device, path='absolute')
        self.absZero = PropertyProxy(root_proxy=device, path='absZero')
        self.absNeg = PropertyProxy(root_proxy=device, path='absNeg')
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
        set_proxy_value(self.string, 'string', 'hello')
        assert controller._internal_widget.text() == 'hello'

    def test_set_abs_err(self):
        controller = DisplayLabel(proxy=self.absolute)
        controller.create(None)
        set_proxy_value(self.absolute, 'absolute', 0.25)
        assert controller._internal_widget.text() == '0.2'

    def test_set_abs_err_zero(self):
        controller = DisplayLabel(proxy=self.absZero)
        controller.create(None)
        set_proxy_value(self.absZero, 'absZero', 0.25555)
        assert controller._internal_widget.text() == '0.25555'

    def test_set_abs_err_neg(self):
        controller = DisplayLabel(proxy=self.absNeg)
        controller.create(None)
        set_proxy_value(self.absNeg, 'absNeg', 0.25555)
        assert controller._internal_widget.text() == '0.25555'

    def test_set_rel_err(self):
        controller = DisplayLabel(proxy=self.relative)
        controller.create(None)
        set_proxy_value(self.relative, 'relative', 0.75)
        assert controller._internal_widget.text() == '0.8'

    def test_set_no_err(self):
        controller = DisplayLabel(proxy=self.alarms)
        controller.create(None)

        set_proxy_value(self.alarms, 'alarms', 0.75)
        assert controller._internal_widget.text() == '0.75'
        assert controller._bg_color == ALL_OK_COLOR

        set_proxy_value(self.alarms, 'alarms', 3.0)
        assert controller._bg_color == PROPERTY_ALARM_COLOR

        set_proxy_value(self.alarms, 'alarms', 1.5)
        assert controller._bg_color == PROPERTY_WARN_COLOR
