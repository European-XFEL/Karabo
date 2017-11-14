from unittest.mock import patch

from karabo.common.api import State
from karabo.common.scenemodel.api import SingleBitModel
from karabo.middlelayer import Configurable, Int32
from karabo_gui.binding.api import KARABO_SCHEMA_DISPLAY_TYPE
from karabo_gui.indicators import STATE_COLORS
from karabo_gui.testing import GuiTestCase, get_class_property_proxy
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
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        singlebit = self.controller._internal_widget

        self.model.invert = False
        self.model.bit = 5

        self.proxy.value = 32
        assert COLORS[State.ACTIVE] in singlebit.styleSheet()

        self.proxy.value = 31
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

        sym = 'karabo_gui.controllers.display.singlebit.QInputDialog'
        with patch(sym) as QInputDialog:
            QInputDialog.getInt.return_value = 3, True
            change_action.trigger()
            assert self.model.bit == 3

            attrs = self.proxy.binding.attributes
            attrs[KARABO_SCHEMA_DISPLAY_TYPE] = 'bin|1,2,3'
            QInputDialog.getItem.return_value = '2:', True
            change_action.trigger()

            assert self.model.bit == 2
