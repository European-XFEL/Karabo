from unittest.mock import patch

from karabo.common.scenemodel.api import FloatSpinBoxModel
from karabo.native import Configurable, Double
from karabogui.binding.api import build_binding
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from ..floatspinbox import FloatSpinBox


class Object(Configurable):
    prop = Double(minInc=-10.0, maxInc=10.0, absoluteError=0.5)


class Other(Configurable):
    prop = Double(minInc=1.0, maxInc=42.0)


class TestFloatSpinBox(GuiTestCase):
    def setUp(self):
        super(TestFloatSpinBox, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
        model = FloatSpinBoxModel(step=0.1, decimals=5)
        self.controller = FloatSpinBox(proxy=self.proxy, model=model)
        self.controller.create(None)
        self.controller.set_read_only(False)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        set_proxy_value(self.proxy, 'prop', 5.0)
        assert self.controller._internal_widget.value() == 5.0

    def test_edit_value(self):
        self.controller._internal_widget.setValue(3.0)
        assert self.proxy.edit_value == 3.0

    def test_schema_update(self):
        proxy = get_class_property_proxy(Other.getClassSchema(), 'prop')
        controller = FloatSpinBox(proxy=proxy, model=FloatSpinBoxModel())
        controller.create(None)

        assert controller._internal_widget.minimum() == 1.0
        assert controller._internal_widget.maximum() == 42.0

        build_binding(Object.getClassSchema(),
                      existing=proxy.root_proxy.binding)

        assert controller._internal_widget.minimum() == -10.0
        assert controller._internal_widget.maximum() == 10.0

    def test_actions(self):
        actions = self.controller.widget.actions()
        change_step, change_decimals = actions

        assert 'step' in change_step.text().lower()
        assert 'decimals' in change_decimals.text().lower()

        sym = 'karabogui.controllers.edit.floatspinbox.QInputDialog'
        with patch(sym) as QInputDialog:
            QInputDialog.getDouble.return_value = 0.25, True
            QInputDialog.getInt.return_value = 9, True

            change_step.trigger()
            assert self.controller.model.step == 0.25

            change_decimals.trigger()
            assert self.controller.model.decimals == 9
