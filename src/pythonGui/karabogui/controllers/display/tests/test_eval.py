from unittest.mock import patch

from karabo.common.scenemodel.api import EvaluatorModel
from karabo.native import Configurable, Float
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from ..eval import Evaluator


class Object(Configurable):
    prop = Float()


class TestEvaluator(GuiTestCase):
    def setUp(self):
        super(TestEvaluator, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')

    def test_basics(self):
        controller = Evaluator(proxy=self.proxy, model=EvaluatorModel())
        controller.create(None)
        assert controller.widget is not None

        controller.destroy()
        assert controller.widget is None

    def test_set_value(self):
        model = EvaluatorModel(expression='x**2')
        controller = Evaluator(proxy=self.proxy, model=model)
        controller.create(None)
        set_proxy_value(self.proxy, 'prop', 2.0)
        assert controller._internal_widget.text() == '4.0'

    def test_builtin_function(self):
        model = EvaluatorModel(expression='round(x)')
        controller = Evaluator(proxy=self.proxy, model=model)
        controller.create(None)
        set_proxy_value(self.proxy, 'prop', 2.5)
        expected = str(round(self.proxy.value))
        assert controller._internal_widget.text() == expected

    def test_change_expression(self):
        set_proxy_value(self.proxy, 'prop', 21.0)
        controller = Evaluator(proxy=self.proxy, model=EvaluatorModel())
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
