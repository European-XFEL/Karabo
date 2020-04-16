from platform import system
from unittest import skipIf

from karabo.common.scenemodel.api import DisplayTextLogModel
from karabo.native import Configurable, String
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from ..textlog import DisplayTextLog


class Object(Configurable):
    prop = String()


class TestDisplayTextLog(GuiTestCase):
    def setUp(self):
        super(TestDisplayTextLog, self).setUp()
        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = DisplayTextLog(proxy=self.proxy,
                                         model=DisplayTextLogModel())
        self.controller.create(None)
        assert self.controller.widget is not None

    def tearDown(self):
        super(TestDisplayTextLog, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    @skipIf(system() == "Windows",
            reason='toPlainText returns an empty string in windows')
    def test_set_value(self):
        self.controller.log_widget.clear()
        set_proxy_value(self.proxy, 'prop', 'Line 1')
        self.assertIn('Line 1', self.controller.log_widget.toPlainText())
