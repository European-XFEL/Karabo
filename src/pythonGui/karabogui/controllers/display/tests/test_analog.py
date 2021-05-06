from unittest.mock import patch

from karabo.native import Configurable, Float
from karabogui.binding.api import build_binding
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from ..analog import DisplayAnalog


class ObjectWithAlarms(Configurable):
    prop = Float(defaultValue=0.0, alarmLow=-10.0, alarmHigh=10.0)


class ObjectWithWarnings(Configurable):
    prop = Float(defaultValue=0.0, warnLow=-5.0, warnHigh=5.0)


class ObjectWithBoth(Configurable):
    prop = Float(defaultValue=0.0,
                 warnLow=-5.0, alarmLow=-10.0,
                 warnHigh=5.0, alarmHigh=10.0)


class ObjectWithout(Configurable):
    prop = Float(defaultValue=0.0)


class TestDisplayAnalog(GuiTestCase):
    def setUp(self):
        super(TestDisplayAnalog, self).setUp()
        schema = ObjectWithBoth.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')

    def test_basics(self):
        controller = DisplayAnalog(proxy=self.proxy)
        controller.create(None)
        assert controller.widget is not None

        controller.destroy()
        assert controller.widget is None

    def test_exercise_draw_full(self):
        controller = DisplayAnalog(proxy=self.proxy)
        controller.create(None)
        # Just exercise the code paths...
        set_proxy_value(self.proxy, 'prop', -11.0)
        set_proxy_value(self.proxy, 'prop', -7.0)
        set_proxy_value(self.proxy, 'prop', 7.0)
        set_proxy_value(self.proxy, 'prop', 11.0)

    def test_exercise_draw_only_alarms(self):
        schema = ObjectWithAlarms.getClassSchema()
        proxy = get_class_property_proxy(schema, 'prop')
        controller = DisplayAnalog(proxy=proxy)
        controller.create(None)

        # Just exercise the code paths
        set_proxy_value(self.proxy, 'prop', 7.0)
        set_proxy_value(self.proxy, 'prop', -11.0)
        set_proxy_value(self.proxy, 'prop', 11.0)

    def test_exercise_draw_only_warnings(self):
        schema = ObjectWithWarnings.getClassSchema()
        proxy = get_class_property_proxy(schema, 'prop')
        controller = DisplayAnalog(proxy=proxy)
        controller.create(None)

        # Just exercise the code paths
        set_proxy_value(self.proxy, 'prop', 1.0)
        set_proxy_value(self.proxy, 'prop', -7.0)
        set_proxy_value(self.proxy, 'prop', 7.0)

    def test_no_alarms_messagebox(self):
        controller = DisplayAnalog(proxy=self.proxy)
        controller.create(None)

        try:
            schema = ObjectWithout.getClassSchema()
            sym = 'karabogui.controllers.display.analog.messagebox'
            with patch(sym) as messagebox:
                build_binding(schema, existing=self.proxy.root_proxy.binding)
                assert messagebox.show_warning.call_count == 1

            # Force an empty draw
            set_proxy_value(self.proxy, 'prop', 4.2)
        finally:
            # Put things back as they were!
            schema = ObjectWithBoth.getClassSchema()
            build_binding(schema, existing=self.proxy.root_proxy.binding)
