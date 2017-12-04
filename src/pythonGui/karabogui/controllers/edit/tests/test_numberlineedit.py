from unittest.mock import patch

from karabo.common.scenemodel.api import DoubleLineEditModel, IntLineEditModel
from karabo.middlelayer import Configurable, Double, Int32
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from ..numberlineedit import DoubleLineEdit, IntLineEdit


class IntObject(Configurable):
    prop = Int32(minInc=-10, maxInc=10)


class FloatObject(Configurable):
    prop = Double(minInc=-10, maxInc=10)


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
        assert abs(self.d_proxy.value - 3.14) < 0.0001

        self.i_controller._internal_widget.setText('3')
        assert self.i_proxy.value == 3

    def test_change_decimals(self):
        action = self.d_controller.widget.actions()[0]
        assert 'decimals' in action.text().lower()

        sym = 'karabogui.controllers.edit.numberlineedit.QInputDialog'
        with patch(sym) as QInputDialog:
            QInputDialog.getInt.return_value = 4, True
            action.trigger()

        assert self.d_controller.model.decimals == 4
