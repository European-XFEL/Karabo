import operator
from unittest import mock

from qtpy.QtGui import QPixmap

from karabo.common.scenemodel.api import (
    DigitIconsModel, IconData, SelectionIconsModel, TextIconsModel)
from karabo.native import Configurable, Int32, String
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from ..icons import DigitIcons, SelectionIcons, TextIcons

NUM_OPTIONS = [1, 2, 3, 4]
TEXT_OPTIONS = ['foo', 'bar', 'baz', 'qux']

ICON_ITEM_PATH = "karabogui.controllers.icons_dialogs.IconItem"


class NumberObject(Configurable):
    prop = Int32(options=NUM_OPTIONS)


class StringObject(Configurable):
    prop = String(options=TEXT_OPTIONS)


def _digits_model():
    values = [IconData(value=2, equal=True), IconData(value=4, equal=False)]
    return DigitIconsModel(values=values)


def _selection_model():
    values = [IconData(value=i) for i in TEXT_OPTIONS[:2]]
    return SelectionIconsModel(values=values)


def _text_model():
    values = [IconData(value=i) for i in TEXT_OPTIONS[:2]]
    return TextIconsModel(values=values)


class TestDigitIcons(GuiTestCase):
    def setUp(self):
        super(TestDigitIcons, self).setUp()

        schema = NumberObject.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = DigitIcons(proxy=self.proxy, model=_digits_model())
        self.controller.create(None)
        self.assertIsNotNone(self.controller.widget)
        self.assertIsNone(self.controller.current_item)

    def tearDown(self):
        super(TestDigitIcons, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_set_value(self):
        self._assert_current_item(2, changed=True)
        self._assert_current_item(1, changed=False)
        self._assert_current_item(3, changed=True)
        self._assert_current_item(4, valid=False)

    def _assert_current_item(self, value, valid=True, changed=True):
        # Save a reference on the previous item
        current_item = self.controller.current_item

        # Set the proxy with mocked icon item to avoid pixmap problems
        method_path = ICON_ITEM_PATH + "._load_pixmap"
        with mock.patch(method_path, return_value=QPixmap()):
            set_proxy_value(self.proxy, 'prop', value)

        if not valid:
            self.assertIsNone(self.controller.current_item)
            return

        assert_same = self.assertIsNot if changed else self.assertIs
        assert_same(current_item, self.controller.current_item)
        current_item = self.controller.current_item
        self.assertIsNotNone(current_item)
        compare = operator.le if current_item.equal else operator.lt
        self.assertTrue(compare(value, float(current_item.value)))


class TestSelectionIcons(GuiTestCase):
    def setUp(self):
        super(TestSelectionIcons, self).setUp()

        schema = StringObject.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = SelectionIcons(proxy=self.proxy,
                                         model=_selection_model())
        self.controller.create(None)
        self.assertIsNotNone(self.controller.widget)
        self.assertIsNone(self.controller.current_item)

    def tearDown(self):
        super(TestSelectionIcons, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_set_value(self):
        self._assert_current_item('foo', changed=True)
        self._assert_current_item('foo', changed=False)
        self._assert_current_item('bar', changed=True)

        # widget will automatically create the items for each option
        self._assert_current_item('qux', changed=True)

    def _assert_current_item(self, value, changed=True):
        # Save a reference on the previous item
        current_item = self.controller.current_item

        # Set the proxy with mocked icon item to avoid pixmap problems
        method_path = ICON_ITEM_PATH + "._load_pixmap"
        with mock.patch(method_path, return_value=QPixmap()):
            set_proxy_value(self.proxy, 'prop', value)

        assert_same = self.assertIsNot if changed else self.assertIs
        assert_same(current_item, self.controller.current_item)
        current_item = self.controller.current_item
        self.assertIsNotNone(current_item)
        self.assertEqual(value, current_item.value)


class TestTextIcons(GuiTestCase):
    def setUp(self):
        super(TestTextIcons, self).setUp()

        schema = StringObject.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = TextIcons(proxy=self.proxy, model=_text_model())
        self.controller.create(None)
        self.assertIsNotNone(self.controller.widget)

    def tearDown(self):
        super(TestTextIcons, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_set_value(self):
        self._assert_current_item('foo', changed=True)
        self._assert_current_item('foo', changed=False)
        self._assert_current_item('bar', changed=True)

        # widget will NOT automatically create the items for each option
        self._assert_current_item('qux', valid=False)

    def _assert_current_item(self, value, valid=True, changed=True):
        # Save a reference on the previous item
        current_item = self.controller.current_item

        # Set the proxy with mocked icon item to avoid pixmap problems
        method_path = ICON_ITEM_PATH + "._load_pixmap"
        with mock.patch(method_path, return_value=QPixmap()):
            set_proxy_value(self.proxy, 'prop', value)

        if not valid:
            self.assertIsNone(self.controller.current_item)
            return

        assert_same = self.assertIsNot if changed else self.assertIs
        assert_same(current_item, self.controller.current_item)
        current_item = self.controller.current_item
        self.assertIsNotNone(current_item)
        self.assertEqual(value, current_item.value)
