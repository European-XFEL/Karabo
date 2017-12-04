from karabo.common.scenemodel.api import (
    DigitIconsModel, IconData, SelectionIconsModel, TextIconsModel)
from karabo.middlelayer import Configurable, Int32, String
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from ..icons import DigitIcons, SelectionIcons, TextIcons

NUM_OPTIONS = [1, 2, 3, 4]
TEXT_OPTIONS = ['foo', 'bar', 'baz', 'qux']


class NumberObject(Configurable):
    prop = Int32(options=NUM_OPTIONS)


class StringObject(Configurable):
    prop = String(options=TEXT_OPTIONS)


def _digits_model():
    values = [IconData(value=str(i)) for i in NUM_OPTIONS]
    return DigitIconsModel(values=values)


def _selection_model():
    values = [IconData(value=i) for i in TEXT_OPTIONS]
    return SelectionIconsModel(values=values)


def _text_model():
    values = [IconData(value=i) for i in TEXT_OPTIONS]
    return TextIconsModel(values=values)


class TestDigitIcons(GuiTestCase):
    def setUp(self):
        super(TestDigitIcons, self).setUp()

        schema = NumberObject.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = DigitIcons(proxy=self.proxy, model=_digits_model())
        self.controller.create(None)
        assert self.controller.widget is not None

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        set_proxy_value(self.proxy, 'prop', 3)


class TestSelectionIcons(GuiTestCase):
    def setUp(self):
        super(TestSelectionIcons, self).setUp()

        schema = StringObject.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = SelectionIcons(proxy=self.proxy,
                                         model=_selection_model())
        self.controller.create(None)
        assert self.controller.widget is not None

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        set_proxy_value(self.proxy, 'prop', 'qux')


class TestTextIcons(GuiTestCase):
    def setUp(self):
        super(TestTextIcons, self).setUp()

        schema = StringObject.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = TextIcons(proxy=self.proxy, model=_text_model())
        self.controller.create(None)
        assert self.controller.widget is not None

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        set_proxy_value(self.proxy, 'prop', 'baz')
