from qtpy.QtWidgets import QWidget

from karabo.common.scenemodel.api import GlobalAlarmModel
from karabo.native import Configurable, String
from karabogui.alarms.api import get_alarm_svg
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..alarm import DisplayAlarm


class MockQSvgWidget(QWidget):
    def load(self, svg):
        self.loaded_data = svg


class AlarmObject(Configurable):
    alarmCondition = String(
        defaultValue="alarm",
        displayType="AlarmCondition")


schema = AlarmObject.getClassSchema()
new_schema = AlarmObject.getClassSchema()


def test_alarm_controller(gui_app, mocker):
    proxy = get_class_property_proxy(schema, "alarmCondition")
    new_proxy = get_class_property_proxy(new_schema, "alarmCondition")
    model = GlobalAlarmModel()

    target = "karabogui.controllers.display.alarm.QSvgWidget"

    # Patch to fake Svg behavior
    mocker.patch(target, new=MockQSvgWidget)

    proxy.value = "warn"
    controller = DisplayAlarm(proxy=proxy, model=model)
    controller.create(None)
    assert controller.widget is not None

    for alarm_type in ("warn", "interlock", "alarm", "none"):
        set_proxy_value(proxy, "alarmCondition", alarm_type)
        active = get_alarm_svg(alarm_type)
        assert controller.widget.loaded_data == active

    none_svg = get_alarm_svg("none")
    assert controller.widget.loaded_data == none_svg

    set_proxy_value(new_proxy, "alarmCondition", "warn")
    # Add a new proxy
    controller.add_proxy(new_proxy)
    warn_svg = get_alarm_svg("warn")
    assert controller.widget.loaded_data == warn_svg

    # Change the old ``none`` alarm to ``alarm``
    set_proxy_value(proxy, "alarmCondition", "alarm")
    alarm_svg = get_alarm_svg("alarm")
    assert controller.widget.loaded_data == alarm_svg

    controller.destroy()
    assert controller.widget is None
