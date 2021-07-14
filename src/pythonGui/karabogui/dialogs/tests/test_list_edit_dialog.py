from unittest import main, mock

from karabo.native import (
    Configurable, Hash, VectorBool, VectorDouble, VectorInt64, VectorString)
from karabogui.dialogs.api import ListEditDialog
from karabogui.testing import GuiTestCase, get_property_proxy, set_proxy_hash


class Object(Configurable):
    string = VectorString(
        defaultValue=["1", "2"])

    boolean = VectorBool(
        defaultValue=[True, False, True])

    integer = VectorInt64(
        defaultValue=[1, 2, 3])

    double = VectorDouble(
        defaultValue=[1.0, 2.0, 3.0])


class TestListEditDialog(GuiTestCase):

    def setUp(self):
        super(TestListEditDialog, self).setUp()
        string_proxy = get_property_proxy(Object.getClassSchema(), "string")
        set_proxy_hash(string_proxy, Hash("string", ["1", "2"]))
        self.string_dialog = ListEditDialog(proxy=string_proxy)

        bool_proxy = get_property_proxy(Object.getClassSchema(), "boolean")
        set_proxy_hash(bool_proxy, Hash("boolean", [True, False, True]))
        self.bool_dialog = ListEditDialog(proxy=bool_proxy)

        int_proxy = get_property_proxy(Object.getClassSchema(), "integer")
        set_proxy_hash(int_proxy, Hash("integer", [1, 2, 3]))
        self.int_dialog = ListEditDialog(proxy=int_proxy)

        double_proxy = get_property_proxy(Object.getClassSchema(), "double")
        set_proxy_hash(double_proxy, Hash("double", [1.0, 2.0, 3.0]))
        self.double_dialog = ListEditDialog(proxy=double_proxy)

    def test_allowed_choices(self):
        self.assertEqual(self.string_dialog._allowed_choices, {})
        self.assertEqual(self.int_dialog._allowed_choices, {})
        self.assertEqual(self.bool_dialog._allowed_choices, {"0": 0, "1": 1})
        self.assertEqual(self.double_dialog._allowed_choices, {})

        self.assertEqual(self.string_dialog.windowTitle(), "Edit list")
        self.assertEqual(self.int_dialog.windowTitle(), "Edit list")
        self.assertEqual(self.bool_dialog.windowTitle(), "Edit list")
        self.assertEqual(self.double_dialog.windowTitle(), "Edit list")

    def test_set_list(self):
        dialog = self.string_dialog
        self.assertEqual(dialog._list_widget.count(), 2)
        dialog.set_list(["1", "2", "3"])
        self.assertEqual(dialog._list_widget.count(), 3)
        self.assertEqual(dialog.values, ["1", "2", "3"])

        dialog = self.bool_dialog
        self.assertEqual(dialog._list_widget.count(), 3)
        self.assertEqual(dialog.values, [True, False, True])
        dialog.set_list([])
        self.assertEqual(dialog._list_widget.count(), 0)
        self.assertEqual(dialog.values, [])

        dialog = self.int_dialog
        self.assertEqual(dialog._list_widget.count(), 3)
        dialog.set_list([1, 2, 3, 4, 5])
        self.assertEqual(dialog._list_widget.count(), 5)
        self.assertEqual(dialog.values, [1, 2, 3, 4, 5])

        dialog = self.double_dialog
        self.assertEqual(dialog._list_widget.count(), 3)
        dialog.set_list([1.1, 2.2, 3.3, 4.4, 5.5])
        self.assertEqual(dialog._list_widget.count(), 5)
        self.assertEqual(dialog.values, [1.1, 2.2, 3.3, 4.4, 5.5])

    def test_on_add_slot(self):
        """Test the add slot of the list edit dialog"""
        dialog = self.string_dialog
        with mock.patch("karabogui.dialogs.listedit.QInputDialog") as d:
            d.getText.return_value = "new", True
            dialog._on_add_clicked()
            self.assertEqual(dialog._list_widget.count(), 3)
        self.assertEqual(dialog.values, ["1", "2", "new"])

        dialog = self.int_dialog
        with mock.patch("karabogui.dialogs.listedit.QInputDialog") as d:
            d.getInt.return_value = 55, True
            dialog._on_add_clicked()
            self.assertEqual(dialog._list_widget.count(), 4)
        self.assertEqual(dialog.values, [1, 2, 3, 55])

        dialog = self.bool_dialog
        with mock.patch("karabogui.dialogs.listedit.QInputDialog") as d:
            # Item dialog with keys of choices, we return first choice
            d.getItem.return_value = "0", True
            dialog._on_add_clicked()
            self.assertEqual(dialog._list_widget.count(), 4)
        self.assertEqual(dialog.values, [True, False, True, False])

        dialog = self.double_dialog
        with mock.patch("karabogui.dialogs.listedit.QInputDialog") as d:
            d.getDouble.return_value = 42.2, True
            dialog._on_add_clicked()
            self.assertEqual(dialog._list_widget.count(), 4)
        self.assertEqual(dialog.values, [1.0, 2.0, 3.0, 42.2])

    def test_on_remove_slot(self):
        """Test the remove slot of the list edit dialog"""
        dialog = self.string_dialog
        self.assertEqual(dialog._list_widget.count(), 2)
        dialog._list_widget.setCurrentRow(0)
        dialog._on_remove_clicked()
        self.assertEqual(dialog._list_widget.count(), 1)
        self.assertEqual(dialog.values, ["2"])

        dialog = self.int_dialog
        self.assertEqual(dialog._list_widget.count(), 3)
        dialog._list_widget.setCurrentRow(0)
        dialog._on_remove_clicked()
        self.assertEqual(dialog._list_widget.count(), 2)
        self.assertEqual(dialog.values, [2, 3])

        dialog = self.bool_dialog
        self.assertEqual(dialog._list_widget.count(), 3)
        dialog._list_widget.setCurrentRow(0)
        dialog._on_remove_clicked()
        self.assertEqual(dialog._list_widget.count(), 2)
        # XXX: Booleans as integers
        self.assertEqual(dialog.values, [0, 1])

        dialog = self.double_dialog
        self.assertEqual(dialog._list_widget.count(), 3)
        dialog._list_widget.setCurrentRow(0)
        dialog._on_remove_clicked()
        self.assertEqual(dialog._list_widget.count(), 2)
        self.assertEqual(dialog.values, [2.0, 3.0])

    def test_move_up_slot(self):
        dialog = self.string_dialog
        dialog._list_widget.setCurrentRow(1)
        dialog._on_moveup_clicked()
        self.assertEqual(dialog.values, ["2", "1"])

        dialog = self.int_dialog
        dialog._list_widget.setCurrentRow(1)
        dialog._on_moveup_clicked()
        self.assertEqual(dialog.values, [2, 1, 3])

        dialog = self.bool_dialog
        dialog._list_widget.setCurrentRow(1)
        dialog._on_moveup_clicked()
        self.assertEqual(dialog.values, [0, 1, 1])

        dialog = self.double_dialog
        dialog._list_widget.setCurrentRow(1)
        dialog._on_moveup_clicked()
        self.assertEqual(dialog.values, [2.0, 1.0, 3.0])

    def test_move_down_slot(self):
        dialog = self.string_dialog
        dialog._list_widget.setCurrentRow(0)
        dialog._on_movedown_clicked()
        self.assertEqual(dialog.values, ["2", "1"])

        dialog = self.int_dialog
        dialog._list_widget.setCurrentRow(0)
        dialog._on_movedown_clicked()
        self.assertEqual(dialog.values, [2, 1, 3])

        dialog = self.bool_dialog
        dialog._list_widget.setCurrentRow(0)
        dialog._on_movedown_clicked()
        self.assertEqual(dialog.values, [0, 1, 1])

        dialog = self.double_dialog
        dialog._list_widget.setCurrentRow(0)
        dialog._on_movedown_clicked()
        self.assertEqual(dialog.values, [2.0, 1.0, 3.0])

    def test_edit_slot(self):
        dialog = self.string_dialog
        dialog._list_widget.setCurrentRow(0)
        with mock.patch("karabogui.dialogs.listedit.QInputDialog") as d:
            d.getText.return_value = "new", True
            dialog._on_edit_clicked()
            self.assertEqual(dialog.values, ["new", "2"])

        dialog = self.int_dialog
        dialog._list_widget.setCurrentRow(0)
        with mock.patch("karabogui.dialogs.listedit.QInputDialog") as d:
            d.getInt.return_value = 55, True
            dialog._on_edit_clicked()
            self.assertEqual(dialog.values, [55, 2, 3])

        dialog = self.bool_dialog
        dialog._list_widget.setCurrentRow(0)
        with mock.patch("karabogui.dialogs.listedit.QInputDialog") as d:
            # Item dialog with keys of choices, we return first choice
            d.getItem.return_value = "0", True
            dialog._on_edit_clicked()
            self.assertEqual(dialog.values, [0, 0, 1])

        dialog = self.double_dialog
        dialog._list_widget.setCurrentRow(0)
        with mock.patch("karabogui.dialogs.listedit.QInputDialog") as d:
            d.getDouble.return_value = 42.2, True
            dialog._on_edit_clicked()
            self.assertEqual(dialog.values, [42.2, 2.0, 3.0])


if __name__ == "__main__":
    main()
