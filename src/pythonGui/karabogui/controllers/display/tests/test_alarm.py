from unittest.mock import patch

from PyQt4.QtGui import QWidget

from karabo.common.scenemodel.api import GlobalAlarmModel
from karabo.native import Configurable, Hash, String
from karabogui.alarms.api import get_alarm_svg
from karabogui.binding.api import apply_configuration
from karabogui.testing import GuiTestCase, get_class_property_proxy
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
        self.proxy = get_class_property_proxy(schema, 'alarmCondition')
        self.model = GlobalAlarmModel()

    def test_basics(self):
        controller = DisplayAlarm(proxy=self.proxy, model=self.model)
        controller.create(None)
        self.assertIsNotNone(controller.widget)

        controller.destroy()
        self.assertIsNone(controller.widget)

    def test_icon_paths(self):
        target = 'karabogui.controllers.display.alarm.QSvgWidget'
        with patch(target, new=MockQSvgWidget):
            self.proxy.value = 'warn'

            controller = DisplayAlarm(proxy=self.proxy, model=self.model)
            controller.create(None)
            for alarm_type in ('warn', 'interlock', 'alarm', 'none'):
                apply_configuration(Hash('alarmCondition', alarm_type),
                                    self.proxy.root_proxy.binding)
                active = get_alarm_svg(alarm_type)
                self.assertEqual(controller.widget.loaded_data, active)
