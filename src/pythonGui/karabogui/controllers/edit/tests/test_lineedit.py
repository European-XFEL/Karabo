from unittest.mock import patch
from unittest import skipIf
from platform import system

import numpy as np

from karabo.common.states import State
from karabo.common.scenemodel.api import (
    DoubleLineEditModel, EditableRegexModel)
from karabo.common.scenemodel.widgets.simple import IntLineEditModel
from karabo.native import (
    Configurable, Double, Float, Int32, String, UInt8, RegexString)
from karabogui.binding.api import build_binding
from karabogui.binding.util import get_editor_value
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from ..lineedit import (
    DoubleLineEdit, Hexadecimal, IntLineEdit, EditRegex)


class IntObject(Configurable):
    state = String(defaultValue=State.INIT)
    prop = Int32(minInc=-10, maxInc=10,
                 allowedStates=[State.INIT])


class DoubleObject(Configurable):
    state = String(defaultValue=State.INIT)
    prop = Double(minInc=-1000, maxInc=1000,
                  allowedStates=[State.INIT])


class FloatObject(Configurable):
    state = String(defaultValue=State.INIT)
    prop = Float(defaultValue=0.0001)


class Uint8Object(Configurable):
    prop = UInt8()


class StringObject(Configurable):
    prop = RegexString(regex="^(0|1|[T]rue|[F]alse)")


class TestRegexEdit(GuiTestCase):

    def setUp(self):
        super(TestRegexEdit, self).setUp()
        self.proxy = get_class_property_proxy(StringObject.getClassSchema(),
                                              'prop')
        self.controller = EditRegex(proxy=self.proxy,
                                    model=EditableRegexModel())
        self.controller.create(None)
        self.controller.set_read_only(False)
        build_binding(StringObject.getClassSchema(),
                      existing=self.proxy.root_proxy.binding)

        self.controller.binding_update(self.proxy)
        self.normal_palette = self.controller._normal_palette
        self.error_palette = self.controller._error_palette

    @property
    def palette(self):
        return self.controller._internal_widget.palette()

    @property
    def text(self):
        return self.controller._internal_widget.text()

    @text.setter
    def text(self, value):
        return self.controller._internal_widget.setText(value)

    def tearDown(self):
        super(TestRegexEdit, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_set_value(self):
        set_proxy_value(self.proxy, 'prop', '1')
        self.assertEqual(self.text, '1')

        set_proxy_value(self.proxy, 'prop', '2')
        self.assertEqual(self.text, '2')

        set_proxy_value(self.proxy, 'prop', 'False')
        self.assertEqual(self.text, 'False')

    def test_edit_value(self):
        self.text = '2'
        self.assertIsNone(self.proxy.edit_value)
        self.text = '1'
        self.assertIsNotNone(self.proxy.edit_value)

    @skipIf(system() in ("Darwin"), reason="MacOS Palette misbehaves")
    def test_decline_color(self):
        self.text = '2.0'
        self.assertIsNone(self.proxy.edit_value)
        self.assertEqual(self.palette, self.error_palette)
        self.controller.on_decline()
        self.assertEqual(self.palette, self.normal_palette)


class TestNumberLineEdit(GuiTestCase):
    def setUp(self):
        super(TestNumberLineEdit, self).setUp()
        self.d_proxy = get_class_property_proxy(DoubleObject.getClassSchema(),
                                                'prop')
        self.d_controller = DoubleLineEdit(proxy=self.d_proxy,
                                           model=DoubleLineEditModel())
        self.d_controller.create(None)
        self.d_controller.set_read_only(False)

        self.f_proxy = get_class_property_proxy(FloatObject.getClassSchema(),
                                                'prop')
        self.f_controller = DoubleLineEdit(proxy=self.f_proxy,
                                           model=DoubleLineEditModel())
        self.f_controller.create(None)
        self.f_controller.set_read_only(False)

        self.i_proxy = get_class_property_proxy(IntObject.getClassSchema(),
                                                'prop')
        self.i_controller = IntLineEdit(proxy=self.i_proxy,
                                        model=IntLineEditModel())
        self.i_controller.create(None)
        self.i_controller.set_read_only(False)

    def tearDown(self):
        super(TestNumberLineEdit, self).tearDown()
        self.d_controller.destroy()
        self.assertIsNone(self.d_controller.widget)
        self.f_controller.destroy()
        self.assertIsNone(self.f_controller.widget)
        self.i_controller.destroy()
        self.assertIsNone(self.i_controller.widget)

    def test_set_value(self):
        set_proxy_value(self.d_proxy, 'prop', np.float32(0.00123))
        self.assertEqual(self.d_controller._internal_widget.text(), '0.00123')
        set_proxy_value(self.d_proxy, 'prop', 5.4)
        self.assertEqual(self.d_controller._internal_widget.text(), '5.4')
        set_proxy_value(self.d_proxy, 'prop', np.float64(0.0088))
        self.assertEqual(self.d_controller._internal_widget.text(), '0.0088')

        set_proxy_value(self.f_proxy, 'prop', np.float32(0.00123))
        self.assertEqual(self.f_controller._internal_widget.text(), '0.00123')
        set_proxy_value(self.f_proxy, 'prop', np.float64(0.0088))
        self.assertEqual(self.f_controller._internal_widget.text(), '0.0088')

        set_proxy_value(self.i_proxy, 'prop', 5)
        self.assertEqual(self.i_controller._internal_widget.text(), '5')

    def test_state_update(self):
        set_proxy_value(self.i_proxy, 'state', 'CHANGING')
        self.assertFalse(self.i_controller._internal_widget.isEnabled())
        set_proxy_value(self.i_proxy, 'state', 'INIT')
        self.assertTrue(self.i_controller._internal_widget.isEnabled())

        set_proxy_value(self.d_proxy, 'state', 'CHANGING')
        self.assertFalse(self.d_controller._internal_widget.isEnabled())
        set_proxy_value(self.d_proxy, 'state', 'INIT')
        self.assertTrue(self.d_controller._internal_widget.isEnabled())

    def test_edit_value(self):
        self.d_controller._internal_widget.setText('3.14')
        self.assertLess(abs(self.d_proxy.edit_value - 3.14), 0.0001)
        self.d_controller._internal_widget.setText('3.14e-2')
        self.assertLess(abs(self.d_proxy.edit_value - 0.0314), 0.00001)
        # Since `maxInc=1000`, then the following shouldn't be accepted,
        # so the value is still 0.0314
        self.d_controller._internal_widget.setText('3.14e9')
        self.assertLess(abs(self.d_proxy._edit_binding.value - 0.0314),
                        0.00001)
        self.assertIsNone(self.d_proxy.edit_value)
        self.i_controller._internal_widget.setText('3')
        self.assertEqual(self.i_proxy.edit_value, 3)
        # Since 12 is greater than `maxInc=10`, then it shouldn't be accepted,
        # so the value is still 3
        self.i_controller._internal_widget.setText('12')
        self.assertNotEqual(self.i_proxy.edit_value, '12')
        self.assertIsNone(self.i_proxy.edit_value)
        self.assertEqual(self.i_controller._internal_value, '3')

    def test_scientific_notation(self):
        test_strings = ['3.141592e0', '1.23e2', '1.23e+2', '1.23e-1']
        results = [3.141592, 123., 123., 0.123]
        for i, test_string in enumerate(test_strings):
            self.d_controller._internal_widget.setText(test_string)
            self.assertEqual(self.d_proxy.edit_value, results[i])
            self.f_controller._internal_widget.setText(test_string)
            self.assertEqual(self.f_proxy.edit_value, results[i])

        last_accepted_value = '1.2'
        self.d_controller._internal_widget.setText(last_accepted_value)
        test_out_of_range = ['1.0e5', '-1.0e5']
        results = [100000, -100000]
        for i, test_string in enumerate(test_out_of_range):
            # shouldn't be accepted, the edit value is None:
            self.d_controller._internal_widget.setText(test_string)
            self.assertNotEqual(self.d_proxy.edit_value, results[i])
            self.assertIsNone(self.d_proxy.edit_value)

    def test_change_decimals(self):
        action = self.d_controller.widget.actions()[0]
        self.assertIn('decimals', action.text().lower())

        sym = 'karabogui.controllers.edit.lineedit.QInputDialog'
        with patch(sym) as QInputDialog:
            QInputDialog.getInt.return_value = 4, True
            action.trigger()

        self.assertEqual(self.d_controller.model.decimals, 4)

    def test_decimal_validation(self):
        self.d_controller._internal_widget.setText('1.0')
        self.assertEqual(self.d_proxy.edit_value, 1.0)
        self.d_controller.model.decimals = 3
        # invalid input for floating decimals
        self.d_controller._internal_widget.setText('1.0003')
        self.assertIsNone(self.d_proxy.edit_value)
        self.d_controller._internal_widget.setText('1.231')
        self.assertEqual(self.d_proxy.edit_value, 1.231)
        self.d_controller._internal_widget.setText('1e-1')
        self.assertEqual(self.d_proxy.edit_value, 0.1)
        # try to trick decimals fails!
        self.d_controller._internal_widget.setText('1.278e-2')
        self.assertIsNone(self.d_proxy.edit_value)
        # follow user input, we are only allowed to set 3 digits
        self.d_controller._internal_widget.setText('1')
        self.assertEqual(self.d_proxy.edit_value, 1.0)
        self.d_controller._internal_widget.setText('1.1')
        self.assertEqual(self.d_proxy.edit_value, 1.1)
        self.d_controller._internal_widget.setText('1.12')
        self.assertEqual(self.d_proxy.edit_value, 1.12)
        self.d_controller._internal_widget.setText('1.124')
        self.assertEqual(self.d_proxy.edit_value, 1.124)
        self.d_controller._internal_widget.setText('1.1244')
        self.assertIsNone(self.d_proxy.edit_value)

    @skipIf(system() in ("Darwin"), reason="MacOS Palette misbehaves")
    def test_decline_color(self):
        self.d_controller._internal_widget.setText('10000.0')
        self.assertIsNone(self.d_proxy.edit_value)
        error_pal = self.d_controller._error_palette
        self.assertEqual(self.d_controller._internal_widget.palette(),
                         error_pal)
        self.d_controller.on_decline()
        normal_pal = self.d_controller._normal_palette
        self.assertEqual(self.d_controller._internal_widget.palette(),
                         normal_pal)


class IntObject24(Configurable):
    prop = Int32(minExc=-2 ** 24,
                 maxExc=2 ** 24)  # 12345678 < 2*24 < 123456789


class TestNumberIntEdit(GuiTestCase):
    def setUp(self):
        super(TestNumberIntEdit, self).setUp()
        self.i_proxy = get_class_property_proxy(IntObject24.getClassSchema(),
                                                'prop')
        self.i_controller = IntLineEdit(proxy=self.i_proxy,
                                        model=IntLineEditModel())
        self.i_controller.create(None)
        self.i_controller.set_read_only(False)
        self.normal_palette = self.i_controller._normal_palette
        self.error_palette = self.i_controller._error_palette

    @property
    def palette(self):
        return self.i_controller._internal_widget.palette()

    def tearDown(self):
        super(TestNumberIntEdit, self).tearDown()
        self.i_controller.destroy()
        self.assertIsNone(self.i_controller.widget)

    @skipIf(system() in ("Darwin"), reason="MacOS Palette misbehaves")
    def test_property_proxy_edit_values_from_text_input(self):
        set_proxy_value(self.i_proxy, 'prop', 1234)

        self.i_controller._internal_widget.setText("12345")
        self.i_controller._internal_widget.setText("123456")
        self.i_controller._internal_widget.setText("1234567")
        self.i_controller._internal_widget.setText("12345678")
        self.assertEqual(self.palette, self.normal_palette)
        self.i_controller._internal_widget.setText("123456789")
        self.assertEqual(self.palette, self.error_palette)
        self.assertNotEqual(get_editor_value(self.i_proxy), 12345678)
        self.assertEqual(get_editor_value(self.i_proxy), 1234)


class TestHexadecimal(GuiTestCase):
    def setUp(self):
        super(TestHexadecimal, self).setUp()
        self.proxy = get_class_property_proxy(Uint8Object.getClassSchema(),
                                              'prop')
        self.controller = Hexadecimal(proxy=self.proxy)
        self.controller.create(None)
        self.controller.set_read_only(False)
        self.normal_palette = self.controller._normal_palette
        self.error_palette = self.controller._error_palette

    def tearDown(self):
        super(TestHexadecimal, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    @property
    def palette(self):
        return self.controller._internal_widget.palette()

    @property
    def text(self):
        return self.controller._internal_widget.text()

    @text.setter
    def text(self, value):
        return self.controller._internal_widget.setText(value)

    @skipIf(system() in ("Darwin"), reason="MacOS Palette misbehaves")
    def test_set_value(self):
        set_proxy_value(self.proxy, 'prop', 0x40)
        self.text = ""
        self.assertEqual(self.palette, self.error_palette)

        self.text = '40'
        self.assertEqual(self.text, '40')
        self.assertEqual(self.palette, self.normal_palette)

        self.text = '-40'
        self.assertEqual(self.text, '-40')
        self.assertNotEqual(get_editor_value(self.proxy), -0x40)
        self.assertEqual(self.palette, self.error_palette)

        self.text = '8F'
        self.assertEqual(self.palette, self.normal_palette)
        self.assertEqual(get_editor_value(self.proxy), 0x8F)
