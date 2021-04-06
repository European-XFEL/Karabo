from unittest.mock import patch

from qtpy.QtWidgets import QWidget

from karabo.common.scenemodel.api import GlobalAlarmModel
from karabo.native import Configurable, String
from karabogui.alarms.api import get_alarm_svg
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from ..alarm import DisplayAlarm


class MockQSvgWidget(QWidget):
    def load(self, svg):
        self.loaded_data = svg


class AlarmObject(Configurable):
    alarmCondition = String(
        defaultValue='alarm',
        displayType='AlarmCondition')


class TestDisplayAlarm(GuiTestCase):
    def setUp(self):
        super(TestDisplayAlarm, self).setUp()
        schema = AlarmObject.getClassSchema()
        new_schema = AlarmObject.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'alarmCondition')
        self.new_proxy = get_class_property_proxy(new_schema, 'alarmCondition')

        self.model = GlobalAlarmModel()

    def test_basics(self):
        controller = DisplayAlarm(proxy=self.proxy, model=self.model)
        controller.create(None)
        self.assertIsNotNone(controller.widget)

        controller.destroy()
        self.assertIsNone(controller.widget)

    def test_alarms(self):
        target = 'karabogui.controllers.display.alarm.QSvgWidget'
        with patch(target, new=MockQSvgWidget):
            self.proxy.value = 'warn'

            controller = DisplayAlarm(proxy=self.proxy, model=self.model)
            controller.create(None)
            for alarm_type in ('warn', 'interlock', 'alarm', 'none'):
                set_proxy_value(self.proxy, 'alarmCondition', alarm_type)
                active = get_alarm_svg(alarm_type)
                self.assertEqual(controller.widget.loaded_data, active)

            none_svg = get_alarm_svg('none')
            self.assertEqual(controller.widget.loaded_data, none_svg)

            set_proxy_value(self.new_proxy, 'alarmCondition', 'warn')
            # Add a new proxy
            controller.add_proxy(self.new_proxy)
            warn_svg = get_alarm_svg('warn')
            self.assertEqual(controller.widget.loaded_data, warn_svg)

            # Change the old ``none`` alarm to ``alarm``
            set_proxy_value(self.proxy, 'alarmCondition', 'alarm')
            alarm_svg = get_alarm_svg('alarm')
            self.assertEqual(controller.widget.loaded_data, alarm_svg)
