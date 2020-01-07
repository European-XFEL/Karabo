from unittest.mock import patch

from karabo.native import Configurable, Double
from karabogui.binding.api import apply_default_configuration, build_binding
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from ..doublewheel import EditableWheelBox


class Object(Configurable):
    prop = Double(defaultValue=1.0)


class MinMaxSchema(Configurable):
    prop = Double(defaultValue=1.0, minInc=0, maxInc=10)


class NoneObject(Configurable):
    prop = Double(defaultValue=None)


class TestDoubleWheelBox(GuiTestCase):
    def setUp(self):
        super(TestDoubleWheelBox, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
        self.controller = EditableWheelBox(proxy=self.proxy)
        self.controller.create(None)
        apply_default_configuration(self.proxy.root_proxy.binding)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_default_configuration(self):
        widget = self.controller._internal_widget
        self.assertEqual(widget.string_value, '+000000.000')
        self.assertEqual(widget.integers, 6)
        self.assertEqual(widget.decimals, 3)

    def test_set_none(self):
        widget = self.controller._internal_widget
        widget.set_value(None)
        # A None value cannot be shown by the wheel widget. This should not
        # harm, as we have an editable widget
        self.assertEqual(widget.string_value, '+000000.000')
        self.assertEqual(widget.integers, 6)
        self.assertEqual(widget.decimals, 3)

    def test_set_values_normal(self):
        widget = self.controller._internal_widget
        widget.set_value(3.121)
        self.assertEqual(widget.string_value, '+000003.121')
        self.assertEqual(widget.integers, 6)
        self.assertEqual(widget.decimals, 3)
        set_proxy_value(self.proxy, 'prop', 4.0)
        self.assertEqual(widget.string_value, '+000004.000')

    def test_set_values_exceed(self):
        widget = self.controller._internal_widget
        widget.set_value(3000000000.0)
        # We enforce a value that does not fit the widget, the widget aligns
        # with space
        self.assertEqual(widget.string_value, '+03000000000.000')
        self.assertEqual(widget.integers, 11)
        self.assertEqual(widget.decimals, 3)

    def test_set_values_exceed_proxy(self):
        widget = self.controller._internal_widget
        set_proxy_value(self.proxy, 'prop', 3000000000.0)
        # We enforce a value that does not fit the widget, the widget aligns
        # with space
        self.assertEqual(widget.string_value, '+03000000000.000')
        self.assertEqual(widget.integers, 11)
        self.assertEqual(widget.decimals, 3)

    def test_set_values_exceed_internal(self):
        widget = self.controller._internal_widget
        widget.set_value(3000000000.0, external=True)
        # We are exceeding and catched by the widget, it is not set. This is
        # for validated values, e.g. by buttons, as they should not modify
        # the number of digits
        self.assertEqual(widget.string_value, '+000000.000')
        self.assertEqual(widget.integers, 6)
        self.assertEqual(widget.decimals, 3)

    def test_set_decimals(self):
        action = self.controller.widget.actions()[0]
        self.assertIn('decimals', action.text().lower())

        sym = 'karabogui.controllers.edit.doublewheel.QInputDialog'
        with patch(sym) as QInputDialog:
            QInputDialog.getInt.return_value = 4, True
            action.trigger()

        self.assertEqual(self.controller.model.decimals, 4)
        widget = self.controller._internal_widget
        self.assertEqual(widget.string_value, '+000000.0000')

    def test_set_integers(self):
        action = self.controller.widget.actions()[1]
        self.assertIn('integers', action.text().lower())

        sym = 'karabogui.controllers.edit.doublewheel.QInputDialog'
        with patch(sym) as QInputDialog:
            QInputDialog.getInt.return_value = 1, True
            action.trigger()

        self.assertEqual(self.controller.model.integers, 1)
        widget = self.controller._internal_widget
        self.assertEqual(widget.string_value, '+0.000')

    def test_minimum_maximum(self):
        schema = MinMaxSchema.getClassSchema()
        self.assertEqual(self.controller._internal_widget._value_minimum,
                         -1.7976931348623157e+308)
        self.assertEqual(self.controller._internal_widget._value_maximum,
                         1.7976931348623157e+308)
        # Just frame min and max
        self.assertEqual(self.controller._internal_widget.total_minimum,
                         -999999.999)
        self.assertEqual(self.controller._internal_widget.total_maximum,
                         999999.999)
        build_binding(schema, existing=self.proxy.root_proxy.binding)
        self.assertEqual(self.controller._internal_widget._value_minimum, 0)
        self.assertEqual(self.controller._internal_widget._value_maximum, 10)
        self.assertEqual(self.controller._internal_widget.total_minimum, 0)
        self.assertEqual(self.controller._internal_widget.total_maximum, 10)
