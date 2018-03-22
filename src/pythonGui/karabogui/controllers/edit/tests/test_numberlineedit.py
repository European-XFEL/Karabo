from unittest.mock import patch

from karabo.common.scenemodel.api import DoubleLineEditModel
from karabo.common.scenemodel.widgets.simple import IntLineEditModel
from karabo.middlelayer import Configurable, Double, Int32
from karabogui.binding.util import get_editor_value
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from ..numberlineedit import DoubleLineEdit, IntLineEdit


class IntObject(Configurable):
    prop = Int32(minInc=-10, maxInc=10)


class FloatObject(Configurable):
    prop = Double(minInc=-1000, maxInc=1000)


class TestNumberLineEdit(GuiTestCase):
    def setUp(self):
        super(TestNumberLineEdit, self).setUp()
        self.d_proxy = get_class_property_proxy(FloatObject.getClassSchema(),
                                                'prop')
        self.d_controller = DoubleLineEdit(proxy=self.d_proxy,
                                           model=DoubleLineEditModel())
        self.d_controller.create(None)
        self.d_controller.set_read_only(False)

        self.i_proxy = get_class_property_proxy(IntObject.getClassSchema(),
                                                'prop')
        self.i_controller = IntLineEdit(proxy=self.i_proxy,
                                        model=IntLineEditModel())
        self.i_controller.create(None)
        self.i_controller.set_read_only(False)

    def tearDown(self):
        self.d_controller.destroy()
        assert self.d_controller.widget is None
        self.i_controller.destroy()
        assert self.i_controller.widget is None

    def test_set_value(self):
        set_proxy_value(self.d_proxy, 'prop', 5.4)
        assert self.d_controller._internal_widget.text() == '5.4'

        set_proxy_value(self.i_proxy, 'prop', 5)
        assert self.i_controller._internal_widget.text() == '5'

    def test_edit_value(self):
        self.d_controller._internal_widget.setText('3.14')
        assert abs(self.d_proxy.edit_value - 3.14) < 0.0001
        self.d_controller._internal_widget.setText('3.14e-2')
        assert abs(self.d_proxy.edit_value - 0.0314) < 0.00001
        # Since `maxInc=1000`, then the following shouldn't be accepted,
        # so the value is still 0.0314
        self.d_controller._internal_widget.setText('3.14e9')
        assert abs(self.d_proxy._edit_binding.value - 0.0314) < 0.00001
        assert self.d_proxy.edit_value is None

        self.i_controller._internal_widget.setText('3')
        assert self.i_proxy.edit_value == 3
        # Since 12 is greater than `maxInc=10`, then it shouldn't be accepted,
        # so the value is still 3
        self.i_controller._internal_widget.setText('12')
        assert self.i_proxy.edit_value != '12'
        assert self.i_proxy.edit_value is None
        assert self.i_controller._internal_value == '3'

    def test_scientific_notation(self):
        test_strings = ['3.141592e0', '1.23e2', '1.23e+2', '1.23e-1']
        results = [3.141592, 123., 123., 0.123]
        for i, test_string in enumerate(test_strings):
            self.d_controller._internal_widget.setText(test_string)
            assert self.d_proxy.edit_value == results[i]

        last_accepted_value = '1.2'
        self.d_controller._internal_widget.setText(last_accepted_value)
        test_out_of_range = ['1.0e5', '-1.0e5']
        results = [100000, -100000]
        for i, test_string in enumerate(test_out_of_range):
            # shouldn't be accepted, the edit value is None:
            self.d_controller._internal_widget.setText(test_string)
            assert self.d_proxy.edit_value != results[i]
            assert self.d_proxy.edit_value is None

    def test_change_decimals(self):
        action = self.d_controller.widget.actions()[0]
        assert 'decimals' in action.text().lower()

        sym = 'karabogui.controllers.edit.numberlineedit.QInputDialog'
        with patch(sym) as QInputDialog:
            QInputDialog.getInt.return_value = 4, True
            action.trigger()

        assert self.d_controller.model.decimals == 4

    def test_decimal_validation(self):
        self.d_controller._internal_widget.setText('1.0')
        assert self.d_proxy.edit_value == 1.0
        self.d_controller.model.decimals = 3
        # invalid input for floating decimals
        self.d_controller._internal_widget.setText('1.0003')
        assert self.d_proxy.edit_value is None
        self.d_controller._internal_widget.setText('1.231')
        assert self.d_proxy.edit_value == 1.231
        self.d_controller._internal_widget.setText('1e-1')
        assert self.d_proxy.edit_value == 0.1
        # try to trick decimals fails!
        self.d_controller._internal_widget.setText('1.278e-2')
        assert self.d_proxy.edit_value is None
        # follow user input, we are only allowed to set 3 digits
        self.d_controller._internal_widget.setText('1')
        assert self.d_proxy.edit_value == 1.0
        self.d_controller._internal_widget.setText('1.1')
        assert self.d_proxy.edit_value == 1.1
        self.d_controller._internal_widget.setText('1.12')
        assert self.d_proxy.edit_value == 1.12
        self.d_controller._internal_widget.setText('1.124')
        assert self.d_proxy.edit_value == 1.124
        self.d_controller._internal_widget.setText('1.1244')
        assert self.d_proxy.edit_value is None

    def test_decline_color(self):
        self.d_controller._internal_widget.setText('10000.0')
        assert self.d_proxy.edit_value is None
        error_pal = self.d_controller._error_palette
        assert self.d_controller._internal_widget.palette() == error_pal
        self.d_controller.on_decline()
        normal_pal = self.d_controller._normal_palette
        assert self.d_controller._internal_widget.palette() == normal_pal


class IntObject24(Configurable):
    prop = Int32(minExc=-2**24, maxExc=2**24)   # 12345678 < 2*24 < 123456789


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
        self.i_controller.destroy()
        assert self.i_controller.widget is None

    def test_property_proxy_edit_values_from_text_input(self):
        set_proxy_value(self.i_proxy, 'prop', 1234)

        self.i_controller._internal_widget.setText("12345")
        self.i_controller._internal_widget.setText("123456")
        self.i_controller._internal_widget.setText("1234567")
        self.i_controller._internal_widget.setText("12345678")
        assert self.palette == self.normal_palette
        self.i_controller._internal_widget.setText("123456789")
        assert self.palette == self.error_palette
        assert not get_editor_value(self.i_proxy) == 12345678
        assert get_editor_value(self.i_proxy) == 1234
