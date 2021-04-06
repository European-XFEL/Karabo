from unittest.mock import patch

from qtpy.QtWidgets import QWidget
from karabo.common.scenemodel.api import ErrorBoolModel
from karabo.native import Configurable, Bool, Hash
from karabogui.binding.api import apply_configuration
from karabogui.testing import GuiTestCase, get_class_property_proxy

from ..errorbool import DisplayErrorBool, OK_BOOL, ERROR_BOOL


class MockQSvgWidget(QWidget):
    def load(self, svg):
        self.loaded_data = svg


class Object(Configurable):
    prop = Bool(defaultValue=True)


class TestErrorBool(GuiTestCase):
    def setUp(self):
        super(TestErrorBool, self).setUp()

        schema = Object.getClassSchema()
        new_schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.new_proxy = get_class_property_proxy(new_schema, 'prop')
        self.model = ErrorBoolModel()

    def test_basics(self):
        controller = DisplayErrorBool(proxy=self.proxy, model=self.model)
        controller.create(None)
        assert controller.widget is not None

        controller.destroy()
        assert controller.widget is None

    def test_exercise_code_paths(self):
        target = 'karabogui.controllers.display.errorbool.SvgWidget'
        with patch(target, new=MockQSvgWidget):
            self.proxy.value = False

            controller = DisplayErrorBool(proxy=self.proxy, model=self.model)
            controller.create(None)

            apply_configuration(Hash('prop', True),
                                self.proxy.root_proxy.binding)
            self.assertEqual(controller.widget.loaded_data, OK_BOOL)

            apply_configuration(Hash('prop', False),
                                self.proxy.root_proxy.binding)
            self.assertEqual(controller.widget.loaded_data, ERROR_BOOL)

            controller.add_proxy(self.new_proxy)
            self.assertEqual(controller.widget.loaded_data, ERROR_BOOL)

            apply_configuration(Hash('prop', True),
                                self.proxy.root_proxy.binding)

            self.assertEqual(controller.widget.loaded_data, OK_BOOL)
