from karabo.middlelayer import Configurable, Bool
from karabo_gui.testing import GuiTestCase, get_class_property_proxy
from ..trendline import DisplayTrendline


class Object(Configurable):
    prop = Bool(defaultValue=True)


class TestDisplayTrendline(GuiTestCase):
    def setUp(self):
        super(TestDisplayTrendline, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = DisplayTrendline(proxy=self.proxy)
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        self.proxy.value = True
        # Allow the update to propogate
        self.process_qt_events()
