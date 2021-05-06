from karabo.common.api import State
from karabo.common.scenemodel.api import DisplayStateColorModel
from karabo.native import Configurable, String
from karabogui.controllers.util import get_class_const_trait
from karabogui.indicators import STATE_COLORS
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from ..statecolor import DisplayStateColor


class Object(Configurable):
    state = String()


class TestStateColorModel(GuiTestCase):
    def setUp(self):
        super(TestStateColorModel, self).setUp()

        schema = Object.getClassSchema()
        self.model = DisplayStateColorModel()
        self.proxy = get_class_property_proxy(schema, 'state')
        self.controller = DisplayStateColor(model=self.model, proxy=self.proxy)
        self.controller.create(None)
        assert self.controller.widget is not None

    def tearDown(self):
        super(TestStateColorModel, self).tearDown()
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_values(self):
        states = ('CHANGING', 'ACTIVE', 'PASSIVE', 'DISABLED', 'RUNNING',
                  'STATIC', 'NORMAL', 'ERROR', 'INIT', 'UNKNOWN')

        for state in states:
            set_proxy_value(self.proxy, 'state', state)
            color = STATE_COLORS[getattr(State, state)]
            sheet = self.controller.widget.styleSheet()
            assert str(color) in sheet

    def test_string(self):
        action = self.controller.widget.actions()[0]
        assert action.text() == 'Show State String'
        state = 'CHANGING'
        set_proxy_value(self.proxy, 'state', state)
        assert self.controller.widget.text() == ''
        action.trigger()
        assert self.controller.widget.text() == 'CHANGING'
        state = 'ON'
        set_proxy_value(self.proxy, 'state', state)
        assert self.controller.widget.text() == 'ON'
        action.trigger()
        assert self.controller.widget.text() == ''

    def test_priority(self):
        assert get_class_const_trait(self.controller, '_priority') == 20
