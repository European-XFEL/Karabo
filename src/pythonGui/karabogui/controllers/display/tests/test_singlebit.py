from unittest.mock import patch

from numpy import uint64

from karabo.common.api import State
from karabo.common.scenemodel.api import SingleBitModel
from karabo.native import Configurable, Int32
from karabogui.indicators import STATE_COLORS
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from ..singlebit import SingleBit

COLORS = {s: str(STATE_COLORS[s]) for s in (State.ACTIVE, State.PASSIVE)}


class Object(Configurable):
    prop = Int32(defaultValue=0)


class TestSingleBit(GuiTestCase):
    def setUp(self):
        super(TestSingleBit, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.model = SingleBitModel()
        self.controller = SingleBit(proxy=self.proxy, model=self.model)
        self.controller.create(None)
        assert self.controller.widget is not None

    def tearDown(self):
        super(TestSingleBit, self).tearDown()
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        singlebit = self.controller._internal_widget

        self.model.invert = False
        self.model.bit = 5

        set_proxy_value(self.proxy, 'prop', uint64(32))
        assert COLORS[State.ACTIVE] in singlebit.styleSheet()

        set_proxy_value(self.proxy, 'prop', uint64(31))
        assert COLORS[State.PASSIVE] in singlebit.styleSheet()

        self.model.bit = 4
        assert COLORS[State.ACTIVE] in singlebit.styleSheet()

        self.model.invert = True
        assert COLORS[State.PASSIVE] in singlebit.styleSheet()

    def test_read_only(self):
        singlebit = self.controller._internal_widget

        self.controller.set_read_only(True)
        assert not singlebit.isEnabled()

        self.controller.set_read_only(False)
        assert singlebit.isEnabled()

    def test_actions(self):
        actions = self.controller.widget.actions()
        invert_action = actions[0]
        change_action = actions[1]
        assert 'invert' in invert_action.text().lower()
        assert 'change' in change_action.text().lower()

        self.model.invert = False
        invert_action.trigger()
        assert self.model.invert

        sym = 'karabogui.controllers.display.singlebit.QInputDialog'
        with patch(sym) as QInputDialog:
            QInputDialog.getInt.return_value = 3, True
            change_action.trigger()
            assert self.model.bit == 3

            self.proxy.binding.display_type = 'bin|1,2,3'
            QInputDialog.getItem.return_value = '2:', True
            change_action.trigger()

            assert self.model.bit == 2
