import pytest

from karabo.common.scenemodel.api import EvaluatorModel
from karabo.native import Configurable, Float
from karabogui.binding.api import (
    DeviceClassProxy, PropertyProxy, ProxyStatus, build_binding)
from karabogui.controllers.display.eval import Evaluator
from karabogui.indicators import (
    ALL_OK_COLOR, PROPERTY_ALARM_COLOR, PROPERTY_WARN_COLOR)
from karabogui.testing import set_proxy_value


class Object(Configurable):
    prop = Float()
    alarms = Float(alarmLow=-2.0, alarmHigh=2.0, warnLow=-1.0, warnHigh=1.0)


@pytest.fixture
def evaluator_setup(gui_app):
    schema = Object.getClassSchema()
    binding = build_binding(schema)
    device = DeviceClassProxy(binding=binding, server_id="Fake",
                              status=ProxyStatus.OFFLINE)
    prop = PropertyProxy(root_proxy=device, path="prop")
    alarms = PropertyProxy(root_proxy=device, path="alarms")
    yield prop, alarms


def test_basics(evaluator_setup):
    prop, _ = evaluator_setup
    controller = Evaluator(proxy=prop, model=EvaluatorModel())
    controller.create(None)
    assert controller.widget is not None

    controller.destroy()
    assert controller.widget is None


def test_set_value(evaluator_setup):
    prop, _ = evaluator_setup
    model = EvaluatorModel(expression="x**2")
    controller = Evaluator(proxy=prop, model=model)
    controller.create(None)
    set_proxy_value(prop, "prop", 2.0)
    assert controller._internal_widget.text() == "4.0"


def test_builtin_function(evaluator_setup):
    prop, _ = evaluator_setup
    model = EvaluatorModel(expression="round(x)")
    controller = Evaluator(proxy=prop, model=model)
    controller.create(None)
    set_proxy_value(prop, "prop", 2.5)
    expected = str(round(prop.value))
    assert controller._internal_widget.text() == expected


def test_change_expression(evaluator_setup, mocker):
    prop, _ = evaluator_setup
    set_proxy_value(prop, "prop", 21.0)
    controller = Evaluator(proxy=prop, model=EvaluatorModel())
    controller.create(None)
    action = controller.widget.actions()[0]
    assert action.text() == "Change expression..."

    dsym = "karabogui.controllers.display.eval.QInputDialog"
    QInputDialog = mocker.patch(dsym)
    QInputDialog.getText.return_value = "x*2", True
    action.trigger()
    assert controller._internal_widget.text() == "42.0"

    # Cause a SyntaxError messagebox
    msym = "karabogui.controllers.display.eval.messagebox"
    messagebox = mocker.patch(msym)
    QInputDialog.getText.return_value = "sin(x", True
    action.trigger()
    assert messagebox.show_warning.call_count == 1


def test_alarm_warning_fine_color(evaluator_setup):
    _, alarms = evaluator_setup
    controller = Evaluator(proxy=alarms)
    controller.create(None)

    set_proxy_value(alarms, "alarms", 0.75)
    assert controller._internal_widget.text() == "0.75"
    assert controller._bg_color == ALL_OK_COLOR

    set_proxy_value(alarms, "alarms", 3.0)
    assert controller._bg_color == PROPERTY_ALARM_COLOR

    set_proxy_value(alarms, "alarms", 1.5)
    assert controller._bg_color == PROPERTY_WARN_COLOR
