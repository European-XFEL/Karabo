# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest.mock import patch

import numpy as np
from qtpy.QtCore import Qt

from karabo.common.scenemodel.api import (
    DoubleLineEditModel, EditableRegexModel, IntLineEditModel)
from karabo.common.states import State
from karabo.native import (
    Configurable, Double, Float, Int32, RegexString, String, UInt8)
from karabogui.binding.api import build_binding
from karabogui.binding.util import get_editor_value
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from ..lineedit import DoubleLineEdit, EditRegex, Hexadecimal, IntLineEdit


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
        super().setUp()
        self.proxy = get_class_property_proxy(StringObject.getClassSchema(),
                                              "prop")
        self.controller = EditRegex(proxy=self.proxy,
                                    model=EditableRegexModel())
        self.controller.create(None)
        self.controller.set_read_only(False)
        build_binding(StringObject.getClassSchema(),
                      existing=self.proxy.root_proxy.binding)
        self.controller.binding_update(self.proxy)
        self.normal_sheet = self.controller._style_sheet.format("black")
        self.error_sheet = self.controller._style_sheet.format("red")

    @property
    def sheet(self):
        return self.controller.internal_widget.styleSheet()

    @property
    def text(self):
        return self.controller.internal_widget.text()

    @text.setter
    def text(self, value):
        return self.controller.internal_widget.setText(value)

    def tearDown(self):
        super().tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_focus_policy(self):
        assert self.controller.internal_widget.focusPolicy() == Qt.StrongFocus

    def test_set_value(self):
        set_proxy_value(self.proxy, "prop", "1")
        self.assertEqual(self.text, "1")

        set_proxy_value(self.proxy, "prop", "2")
        self.assertEqual(self.text, "2")

        set_proxy_value(self.proxy, "prop", "False")
        self.assertEqual(self.text, "False")

    def test_edit_value(self):
        self.text = "2"
        self.assertIsNone(self.proxy.edit_value)
        self.text = "1"
        self.assertIsNotNone(self.proxy.edit_value)

    def test_decline_color(self):
        self.text = "2.0"
        self.assertIsNone(self.proxy.edit_value)
        self.assertEqual(self.sheet, self.error_sheet)
        self.controller.on_decline()
        self.assertEqual(self.sheet, self.normal_sheet)


class TestNumberLineEdit(GuiTestCase):
    def setUp(self):
        super().setUp()
        self.double_proxy = get_class_property_proxy(
            DoubleObject.getClassSchema(), "prop")
        self.double_controller = DoubleLineEdit(proxy=self.double_proxy,
                                                model=DoubleLineEditModel())
        self.double_controller.create(None)
        self.double_controller.set_read_only(False)

        self.float_proxy = get_class_property_proxy(
            FloatObject.getClassSchema(), "prop")
        self.float_controller = DoubleLineEdit(proxy=self.float_proxy,
                                               model=DoubleLineEditModel())
        self.float_controller.create(None)
        self.float_controller.set_read_only(False)

        self.int_proxy = get_class_property_proxy(
            IntObject.getClassSchema(), "prop")
        self.int_controller = IntLineEdit(proxy=self.int_proxy,
                                          model=IntLineEditModel())
        self.int_controller.create(None)
        self.int_controller.set_read_only(False)

    def tearDown(self):
        super().tearDown()
        self.double_controller.destroy()
        self.assertIsNone(self.double_controller.widget)
        self.float_controller.destroy()
        self.assertIsNone(self.float_controller.widget)
        self.int_controller.destroy()
        self.assertIsNone(self.int_controller.widget)

    @property
    def float_text(self):
        return self.float_controller.internal_widget.text()

    @float_text.setter
    def float_text(self, text):
        self.float_controller.internal_widget.setText(text)

    @property
    def double_text(self):
        return self.double_controller.internal_widget.text()

    @double_text.setter
    def double_text(self, text):
        self.double_controller.internal_widget.setText(text)

    @property
    def int_text(self):
        return self.int_controller.internal_widget.text()

    @int_text.setter
    def int_text(self, text):
        self.int_controller.internal_widget.setText(text)

    def test_set_value(self):
        set_proxy_value(self.double_proxy, "prop", np.float32(0.00123))
        self.assertEqual(self.double_text, "0.00123")
        set_proxy_value(self.double_proxy, "prop", 5.4)
        self.assertEqual(self.double_text, "5.4")
        set_proxy_value(self.double_proxy, "prop", np.float64(0.0088))
        self.assertEqual(self.double_text, "0.0088")

        set_proxy_value(self.float_proxy, "prop", np.float32(0.00123))
        self.assertEqual(self.float_text, "0.00123")
        set_proxy_value(self.float_proxy, "prop", np.float64(0.0088))
        self.assertEqual(self.float_text, "0.0088")

    def test_state_update(self):
        set_proxy_value(self.double_proxy, "state", "CHANGING")
        self.assertFalse(self.double_controller.internal_widget.isEnabled())
        set_proxy_value(self.double_proxy, "state", "INIT")
        self.assertTrue(self.double_controller.internal_widget.isEnabled())

    def test_edit_value(self):
        self.double_text = "3.14"
        self.assertLess(abs(self.double_proxy.edit_value - 3.14), 0.0001)
        self.double_text = "3.14e-2"
        self.assertLess(abs(self.double_proxy.edit_value - 0.0314), 0.00001)
        # Since `maxInc=1000`, then the following shouldn"t be accepted,
        # so the value is still 0.0314
        self.double_text = "3.14e9"
        self.assertLess(abs(self.double_proxy._edit_binding.value - 0.0314),
                        0.00001)
        self.assertIsNone(self.double_proxy.edit_value)
        # Integers
        self.int_text = "3"
        self.assertEqual(self.int_proxy.edit_value, 3)
        self.assertEqual(self.int_text, "3")

        # Since 12 is greater than `maxInc=10`, then it shouldn't be accepted,
        # so the value is still 3
        self.int_text = "12"
        self.assertNotEqual(self.int_proxy.edit_value, "12")
        self.assertIsNone(self.int_proxy.edit_value)
        self.assertEqual(self.int_text, "12")

    def test_scientific_notation(self):
        test_strings = ["3.141592e0", "1.23e2", "1.23e+2", "1.23e-1"]
        results = [3.141592, 123., 123., 0.123]
        for i, test_string in enumerate(test_strings):
            self.double_text = test_string
            self.assertEqual(self.double_proxy.edit_value, results[i])
            self.float_controller.internal_widget.setText(test_string)
            self.assertEqual(self.float_proxy.edit_value, results[i])

        last_accepted_value = "1.2"
        self.double_text = last_accepted_value
        test_out_of_range = ["1.0e5", "-1.0e5"]
        results = [100000, -100000]
        for i, test_string in enumerate(test_out_of_range):
            # shouldn't be accepted, the edit value is None:
            self.double_text = test_string
            self.assertNotEqual(self.double_proxy.edit_value, results[i])
            self.assertIsNone(self.double_proxy.edit_value)

    def test_change_decimals(self):
        action = self.double_controller.widget.actions()[0]
        self.assertIn("decimals", action.text().lower())

        sym = "karabogui.controllers.edit.lineedit.QInputDialog"
        with patch(sym) as QInputDialog:
            QInputDialog.getInt.return_value = 4, True
            action.trigger()

        self.assertEqual(self.double_controller.model.decimals, 4)

    def test_decimal_validation(self):
        self.double_text = "1.0"
        self.assertEqual(self.double_proxy.edit_value, 1.0)
        self.double_controller.model.decimals = 3
        # invalid input for floating decimals
        self.double_text = "1.0003"
        self.assertIsNone(self.double_proxy.edit_value)
        self.double_text = "1.231"
        self.assertEqual(self.double_proxy.edit_value, 1.231)
        self.double_text = "1e-1"
        self.assertEqual(self.double_proxy.edit_value, 0.1)
        # try to trick decimals fails!
        self.double_text = "1.278e-2"
        self.assertIsNone(self.double_proxy.edit_value)
        # follow user input, we are only allowed to set 3 digits
        self.double_text = "1"
        self.assertEqual(self.double_proxy.edit_value, 1.0)
        self.double_text = "1.1"
        self.assertEqual(self.double_proxy.edit_value, 1.1)
        self.double_text = "1.12"
        self.assertEqual(self.double_proxy.edit_value, 1.12)
        self.double_text = "1.124"
        self.assertEqual(self.double_proxy.edit_value, 1.124)
        self.double_text = "1.1244"
        self.assertIsNone(self.double_proxy.edit_value)

    def test_decline_color(self):
        self.double_text = "10000.0"
        self.assertIsNone(self.double_proxy.edit_value)
        error_sheet = self.double_controller._style_sheet.format("red")
        self.assertEqual(self.double_controller.internal_widget.styleSheet(),
                         error_sheet)
        self.double_controller.on_decline()
        normal_sheet = self.double_controller._style_sheet.format("black")
        self.assertEqual(self.double_controller.internal_widget.styleSheet(),
                         normal_sheet)


class IntObject24(Configurable):
    prop = Int32(minExc=-2 ** 24,
                 maxExc=2 ** 24)  # 12345678 < 2*24 < 123456789


class TestNumberIntEdit(GuiTestCase):
    def setUp(self):
        super().setUp()
        self.proxy = get_class_property_proxy(
            IntObject24.getClassSchema(), "prop")
        self.controller = IntLineEdit(proxy=self.proxy,
                                      model=IntLineEditModel())
        self.controller.create(None)
        self.controller.set_read_only(False)
        self.normal_sheet = self.controller._style_sheet.format("black")
        self.error_sheet = self.controller._style_sheet.format("red")

    @property
    def sheet(self):
        return self.controller.internal_widget.styleSheet()

    @property
    def text(self):
        return self.controller.internal_widget.text()

    @text.setter
    def text(self, value):
        return self.controller.internal_widget.setText(value)

    def tearDown(self):
        super().tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_property_proxy_edit_values_from_text_input(self):
        set_proxy_value(self.proxy, "prop", 1234)
        self.text = "12345"
        self.text = "123456"
        self.text = "1234567"
        self.text = "12345678"
        self.assertEqual(self.sheet, self.normal_sheet)
        self.text = "123456789"
        self.assertEqual(self.sheet, self.error_sheet)
        self.assertNotEqual(get_editor_value(self.proxy), 12345678)
        self.assertEqual(get_editor_value(self.proxy), 1234)


class TestHexadecimal(GuiTestCase):
    def setUp(self):
        super().setUp()
        self.proxy = get_class_property_proxy(Uint8Object.getClassSchema(),
                                              "prop")
        self.controller = Hexadecimal(proxy=self.proxy)
        self.controller.create(None)
        self.controller.set_read_only(False)
        self.normal_sheet = self.controller._style_sheet.format("black")
        self.error_sheet = self.controller._style_sheet.format("red")

    def tearDown(self):
        super().tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    @property
    def sheet(self):
        return self.controller.internal_widget.styleSheet()

    @property
    def text(self):
        return self.controller.internal_widget.text()

    @text.setter
    def text(self, value):
        return self.controller.internal_widget.setText(value)

    def test_set_value(self):
        set_proxy_value(self.proxy, "prop", 0x40)
        self.text = ""
        self.assertEqual(self.sheet, self.error_sheet)

        self.text = "40"
        self.assertEqual(self.text, "40")
        self.assertEqual(self.sheet, self.normal_sheet)

        self.text = "-40"
        self.assertEqual(self.text, "-40")
        self.assertNotEqual(get_editor_value(self.proxy), -0x40)
        self.assertEqual(self.sheet, self.error_sheet)

        self.text = "8F"
        self.assertEqual(self.sheet, self.normal_sheet)
        self.assertEqual(get_editor_value(self.proxy), 0x8F)
