# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import numpy as np
import pytest
from qtpy.QtCore import Qt

from karabo.common.scenemodel.api import (
    DoubleLineEditModel, EditableRegexModel, IntLineEditModel)
from karabo.common.states import State
from karabo.native import (
    Configurable, Double, Float, Int32, RegexString, String, UInt8)
from karabogui.binding.api import build_binding
from karabogui.binding.util import get_editor_value
from karabogui.testing import get_class_property_proxy, set_proxy_value

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


class RegexEditSetUp:
    def __init__(self):
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


@pytest.fixture
def regex_edit_setup(gui_app):
    test_object = RegexEditSetUp()
    yield test_object
    # teardown
    test_object.controller.destroy()
    assert test_object.controller.widget is None


def test_regex_lineedit_focus_policy(regex_edit_setup):
    regex_lineedit = regex_edit_setup
    assert (regex_lineedit.controller.internal_widget.focusPolicy()
            == Qt.StrongFocus)


def test_regex_lineedit_set_value(regex_edit_setup):
    regex_lineedit = regex_edit_setup
    set_proxy_value(regex_lineedit.proxy, "prop", "1")
    assert regex_lineedit.text == "1"

    set_proxy_value(regex_lineedit.proxy, "prop", "2")
    assert regex_lineedit.text == "2"

    set_proxy_value(regex_lineedit.proxy, "prop", "False")
    assert regex_lineedit.text == "False"


def test_regex_lineedit_edit_value(regex_edit_setup):
    regex_lineedit = regex_edit_setup
    regex_lineedit.text = "2"
    assert regex_lineedit.proxy.edit_value is None
    regex_lineedit.text = "1"
    assert regex_lineedit.proxy.edit_value is not None


def test_regex_lineedit_decline_color(regex_edit_setup):
    regex_lineedit = regex_edit_setup
    regex_lineedit.text = "2.0"
    assert regex_lineedit.proxy.edit_value is None
    assert regex_lineedit.sheet == regex_lineedit.error_sheet
    regex_lineedit.controller.on_decline()
    assert regex_lineedit.sheet == regex_lineedit.normal_sheet


class NumberLineEditSetUp:
    def __init__(self):
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


@pytest.fixture
def number_lineedit_setup(gui_app):
    test_object = NumberLineEditSetUp()
    yield test_object
    # teardown
    test_object.double_controller.destroy()
    assert test_object.double_controller.widget is None
    test_object.float_controller.destroy()
    assert test_object.float_controller.widget is None
    test_object.int_controller.destroy()
    assert test_object.int_controller.widget is None


def test_number_lineedit_set_value(number_lineedit_setup):
    number_lineedit = number_lineedit_setup
    set_proxy_value(number_lineedit.double_proxy, "prop", np.float32(0.00123))
    assert number_lineedit.double_text == "0.00123"
    set_proxy_value(number_lineedit.double_proxy, "prop", 5.4)
    assert number_lineedit.double_text == "5.4"
    set_proxy_value(number_lineedit.double_proxy, "prop", np.float64(0.0088))
    assert number_lineedit.double_text == "0.0088"

    set_proxy_value(number_lineedit.float_proxy, "prop", np.float32(0.00123))
    assert number_lineedit.float_text == "0.00123"
    set_proxy_value(number_lineedit.float_proxy, "prop", np.float64(0.0088))
    assert number_lineedit.float_text == "0.0088"


def test_number_lineedit_state_update(number_lineedit_setup):
    number_lineedit = number_lineedit_setup
    set_proxy_value(number_lineedit.double_proxy, "state", "CHANGING")
    assert not number_lineedit.double_controller.internal_widget.isEnabled()
    set_proxy_value(number_lineedit.double_proxy, "state", "INIT")
    assert number_lineedit.double_controller.internal_widget.isEnabled()


def test_number_lineedit_edit_value(number_lineedit_setup):
    number_lineedit = number_lineedit_setup
    number_lineedit.double_text = "3.14"
    assert abs(number_lineedit.double_proxy.edit_value - 3.14) < 0.0001
    number_lineedit.double_text = "3.14e-2"
    assert abs(number_lineedit.double_proxy.edit_value - 0.0314) < 0.00001
    # Since `maxInc=1000`, then the following shouldn't be accepted,
    # so the value is still 0.0314
    number_lineedit.double_text = "3.14e9"
    assert (abs(number_lineedit.double_proxy._edit_binding.value - 0.0314)
            < 0.00001)
    assert number_lineedit.double_proxy.edit_value is None
    # Integers
    number_lineedit.int_text = "3"
    assert number_lineedit.int_proxy.edit_value == 3
    assert number_lineedit.int_text == "3"

    # Since 12 is greater than `maxInc=10`, then it shouldn't be accepted,
    # so the value is still 3
    number_lineedit.int_text = "12"
    assert number_lineedit.int_proxy.edit_value != "12"
    assert number_lineedit.int_proxy.edit_value is None
    assert number_lineedit.int_text == "12"


def test_number_lineedit_scientific_notation(number_lineedit_setup):
    number_lineedit = number_lineedit_setup
    test_strings = ["3.141592e0", "1.23e2", "1.23e+2", "1.23e-1"]
    results = [3.141592, 123., 123., 0.123]
    for i, test_string in enumerate(test_strings):
        number_lineedit.double_text = test_string
        assert number_lineedit.double_proxy.edit_value == results[i]
        number_lineedit.float_controller.internal_widget.setText(test_string)
        assert number_lineedit.float_proxy.edit_value == results[i]

    last_accepted_value = "1.2"
    number_lineedit.double_text = last_accepted_value
    test_out_of_range = ["1.0e5", "-1.0e5"]
    results = [100000, -100000]
    for i, test_string in enumerate(test_out_of_range):
        # shouldn't be accepted, the edit value is None:
        number_lineedit.double_text = test_string
        assert number_lineedit.double_proxy.edit_value != results[i]
        assert number_lineedit.double_proxy.edit_value is None


def test_number_lineedit_change_decimals(number_lineedit_setup, mocker):
    number_lineedit = number_lineedit_setup
    action = number_lineedit.double_controller.widget.actions()[0]
    assert "decimals" in action.text().lower()

    sym = "karabogui.controllers.edit.lineedit.QInputDialog"
    QInputDialog = mocker.patch(sym)
    QInputDialog.getInt.return_value = 4, True
    action.trigger()

    assert number_lineedit.double_controller.model.decimals == 4


def test_number_lineedit_decimal_validation(number_lineedit_setup):
    number_lineedit = number_lineedit_setup
    number_lineedit.double_text = "1.0"
    assert number_lineedit.double_proxy.edit_value == 1.0
    number_lineedit.double_controller.model.decimals = 3
    # invalid input for floating decimals
    number_lineedit.double_text = "1.0003"
    assert number_lineedit.double_proxy.edit_value is None
    number_lineedit.double_text = "1.231"
    assert number_lineedit.double_proxy.edit_value == 1.231
    number_lineedit.double_text = "1e-1"
    assert number_lineedit.double_proxy.edit_value == 0.1
    # try to trick decimals fails!
    number_lineedit.double_text = "1.278e-2"
    assert number_lineedit.double_proxy.edit_value is None
    # follow user input, we are only allowed to set 3 digits
    number_lineedit.double_text = "1"
    assert number_lineedit.double_proxy.edit_value == 1.0
    number_lineedit.double_text = "1.1"
    assert number_lineedit.double_proxy.edit_value == 1.1
    number_lineedit.double_text = "1.12"
    assert number_lineedit.double_proxy.edit_value == 1.12
    number_lineedit.double_text = "1.124"
    assert number_lineedit.double_proxy.edit_value == 1.124
    number_lineedit.double_text = "1.1244"
    assert number_lineedit.double_proxy.edit_value is None


def test_number_lineedit_decline_color(number_lineedit_setup):
    number_lineedit = number_lineedit_setup
    number_lineedit.double_text = "10000.0"
    assert number_lineedit.double_proxy.edit_value is None
    error_sheet = number_lineedit.double_controller._style_sheet.format("red")
    assert (number_lineedit.double_controller.internal_widget.styleSheet()
            == error_sheet)
    number_lineedit.double_controller.on_decline()
    normal_sheet = number_lineedit.double_controller._style_sheet.format(
        "black")
    assert (number_lineedit.double_controller.internal_widget.styleSheet()
            == normal_sheet)


class IntObject24(Configurable):
    prop = Int32(minExc=-2 ** 24,
                 maxExc=2 ** 24)  # 12345678 < 2*24 < 123456789


class NumberIntEditSetUp:
    def __init__(self):
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


def test_number_int_property_proxy_edit_values_from_text_input(gui_app):
    # setup
    number_int = test_object = NumberIntEditSetUp()
    # test body
    set_proxy_value(number_int.proxy, "prop", 1234)
    number_int.text = "12345"
    number_int.text = "123456"
    number_int.text = "1234567"
    number_int.text = "12345678"
    assert number_int.sheet == number_int.normal_sheet
    number_int.text = "123456789"
    assert number_int.sheet == number_int.error_sheet
    assert get_editor_value(number_int.proxy) != 12345678
    assert get_editor_value(number_int.proxy) == 1234
    # teardown
    test_object.controller.destroy()
    assert test_object.controller.widget is None


class HexadecimalSetUp:
    def __init__(self):
        self.proxy = get_class_property_proxy(Uint8Object.getClassSchema(),
                                              "prop")
        self.controller = Hexadecimal(proxy=self.proxy)
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


def test_hexadecimal_set_value(gui_app):
    # setup
    hexadecimal_edit = HexadecimalSetUp()
    # test body
    set_proxy_value(hexadecimal_edit.proxy, "prop", 0x40)
    hexadecimal_edit.text = ""
    assert hexadecimal_edit.sheet == hexadecimal_edit.error_sheet

    hexadecimal_edit.text = "40"
    assert hexadecimal_edit.text == "40"
    assert hexadecimal_edit.sheet == hexadecimal_edit.normal_sheet

    hexadecimal_edit.text = "-40"
    assert hexadecimal_edit.text == "-40"
    assert get_editor_value(hexadecimal_edit.proxy) != -0x40
    assert hexadecimal_edit.sheet == hexadecimal_edit.error_sheet

    hexadecimal_edit.text = "8F"
    assert hexadecimal_edit.sheet == hexadecimal_edit.normal_sheet
    assert get_editor_value(hexadecimal_edit.proxy) == 0x8F
    # teardown
    hexadecimal_edit.controller.destroy()
    assert hexadecimal_edit.controller.widget is None
