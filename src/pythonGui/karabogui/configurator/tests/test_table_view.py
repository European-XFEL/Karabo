from karabo.native import Configurable, Hash, String, VectorHash
from karabogui.configurator.api import TableDialog
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_hash)


class TableSchema(Configurable):
    foo = String(
        defaultValue="NoString")
    bar = String(
        displayType="State",
        displayedName="Bar",
        defaultValue="ON")


class BigTableSchema(Configurable):
    a = String()
    b = String()
    c = String()
    d = String()
    e = String()


TABLE_HASH = Hash("prop", [Hash("foo", "1", "bar", "hello"),
                           Hash("foo", "2", "bar", "test"),
                           Hash("foo", "3", "bar", "No"),
                           Hash("foo", "4", "bar", "Jo")])


class Object(Configurable):
    prop = VectorHash(rows=TableSchema)
    bigProp = VectorHash(rows=BigTableSchema)


class TableDialogTest(GuiTestCase):
    def test_basic_dialog(self):
        proxy = get_class_property_proxy(Object.getClassSchema(), "prop")

        dialog = TableDialog(proxy, True)
        self.assertIsNotNone(dialog)
        self.assertEqual(dialog.width(), 450)
        set_proxy_hash(proxy, TABLE_HASH)

        self.assertEqual(dialog.width(), 450)
        # Finish dialog, destroy widget
        dialog.done(1)
        self.assertIsNotNone(dialog.controller)
        self.assertIsNone(dialog.controller.widget)

        # New dialog, with big table schema
        proxy = get_class_property_proxy(Object.getClassSchema(), "bigProp")
        dialog = TableDialog(proxy, True)
        self.assertIsNotNone(dialog)
        self.assertEqual(dialog.width(), 600)
        # Finish dialog without success
        dialog.done(0)
        self.assertIsNotNone(dialog.controller)
        self.assertIsNone(dialog.controller.widget)
