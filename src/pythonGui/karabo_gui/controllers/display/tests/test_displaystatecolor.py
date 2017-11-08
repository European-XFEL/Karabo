from unittest.mock import patch

from karabo.common.api import State
from karabo.common.scenemodel.api import DisplayStateColorModel
from karabo.middlelayer import Configurable, String
from karabo_gui.indicators import STATE_COLORS
from karabo_gui.testing import GuiTestCase, get_class_property_proxy
from ..displaystatecolor import DisplayStateColor


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
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_values(self):
        states = ('CHANGING', 'ACTIVE', 'PASSIVE', 'DISABLED', 'STATIC',
                  'NORMAL', 'ERROR', 'INIT', 'UNKNOWN')

        for state in states:
            self.proxy.value = state
            color = STATE_COLORS[getattr(State, state)]
            sheet = self.controller.widget.styleSheet()
            assert str(color) in sheet

    def test_static_text(self):
        action = self.controller.widget.actions()[0]
        assert action.text() == 'Edit Static Text...'

        sym = 'karabo_gui.controllers.display.displaystatecolor.QInputDialog'
        with patch(sym) as QInputDialog:
            QInputDialog.getText.return_value = 'fake news', True
            action.trigger()

        assert self.controller.widget.text() == 'fake news'
