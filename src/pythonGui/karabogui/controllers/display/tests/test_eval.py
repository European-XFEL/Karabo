from unittest.mock import patch

from karabo.common.api import ProxyStatus
from karabo.common.scenemodel.api import EvaluatorModel
from karabo.native import Configurable, Float
from karabogui.binding.api import (
    DeviceClassProxy, PropertyProxy, build_binding)
from karabogui.indicators import (
    ALL_OK_COLOR, PROPERTY_ALARM_COLOR, PROPERTY_WARN_COLOR)
from karabogui.testing import GuiTestCase, set_proxy_value

from ..eval import Evaluator


class Object(Configurable):
    prop = Float()
    alarms = Float(alarmLow=-2.0, alarmHigh=2.0,
                   warnLow=-1.0, warnHigh=1.0)


class TestEvaluator(GuiTestCase):
    def setUp(self):
        super(TestEvaluator, self).setUp()

        schema = Object.getClassSchema()
        binding = build_binding(schema)
        device = DeviceClassProxy(binding=binding, server_id='Fake',
                                  status=ProxyStatus.OFFLINE)
        self.prop = PropertyProxy(root_proxy=device, path='prop')
        self.alarms = PropertyProxy(root_proxy=device, path='alarms')

    def test_basics(self):
        controller = Evaluator(proxy=self.prop, model=EvaluatorModel())
        controller.create(None)
        assert controller.widget is not None

        controller.destroy()
        assert controller.widget is None

    def test_set_value(self):
        model = EvaluatorModel(expression='x**2')
        controller = Evaluator(proxy=self.prop, model=model)
        controller.create(None)
        set_proxy_value(self.prop, 'prop', 2.0)
        assert controller._internal_widget.text() == '4.0'

    def test_builtin_function(self):
        model = EvaluatorModel(expression='round(x)')
        controller = Evaluator(proxy=self.prop, model=model)
        controller.create(None)
        set_proxy_value(self.prop, 'prop', 2.5)
        expected = str(round(self.prop.value))
        assert controller._internal_widget.text() == expected

    def test_change_expression(self):
        set_proxy_value(self.prop, 'prop', 21.0)
        controller = Evaluator(proxy=self.prop, model=EvaluatorModel())
        controller.create(None)
        action = controller.widget.actions()[0]
        assert action.text() == 'Change expression...'

        dsym = 'karabogui.controllers.display.eval.QInputDialog'
        with patch(dsym) as QInputDialog:
            QInputDialog.getText.return_value = 'x*2', True
            action.trigger()
            assert controller._internal_widget.text() == '42.0'

        # Cause a SyntaxError messagebox
        msym = 'karabogui.controllers.display.eval.messagebox'
        with patch(dsym) as QInputDialog, patch(msym) as messagebox:
            QInputDialog.getText.return_value = 'sin(x', True
            action.trigger()
            assert messagebox.show_warning.call_count == 1

    def test_alarm_warning_fine_color(self):
        controller = Evaluator(proxy=self.alarms)
        controller.create(None)

        set_proxy_value(self.alarms, 'alarms', 0.75)
        assert controller._internal_widget.text() == '0.75'
        assert controller._bg_color == ALL_OK_COLOR

        set_proxy_value(self.alarms, 'alarms', 3.0)
        assert controller._bg_color == PROPERTY_ALARM_COLOR

        set_proxy_value(self.alarms, 'alarms', 1.5)
        assert controller._bg_color == PROPERTY_WARN_COLOR
