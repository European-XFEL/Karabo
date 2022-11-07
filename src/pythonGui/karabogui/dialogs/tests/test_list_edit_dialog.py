from unittest import main, mock

from karabogui.binding.api import (
    VectorBoolBinding, VectorDoubleBinding, VectorInt64Binding,
    VectorStringBinding)
from karabogui.controllers.table.api import string2list
from karabogui.dialogs.api import ListEditDialog
from karabogui.testing import GuiTestCase


class TestListEditDialog(GuiTestCase):

    def setUp(self):
        super().setUp()
        string_binding = VectorStringBinding()
        self.string_dialog = ListEditDialog(binding=string_binding)
        self.string_dialog.set_list(["1", "2"])

        bool_binding = VectorBoolBinding()
        self.bool_dialog = ListEditDialog(binding=bool_binding)
        self.bool_dialog.set_list([True, False, True])

        int_binding = VectorInt64Binding()
        self.int_dialog = ListEditDialog(binding=int_binding)
        self.int_dialog.set_list([1, 2, 3])

        double_binding = VectorDoubleBinding()
        self.double_dialog = ListEditDialog(binding=double_binding)
        self.double_dialog.set_list([1.0, 2.0, 3.0])

    def test_allowed_choices(self):
        self.assertEqual(self.string_dialog._allowed_choices, {})
        self.assertEqual(self.int_dialog._allowed_choices, {})
        self.assertEqual(self.bool_dialog._allowed_choices, {"False": False,
                                                             "True": True})
        self.assertEqual(self.double_dialog._allowed_choices, {})

        self.assertEqual(self.string_dialog.windowTitle(), "Edit list")
        self.assertEqual(self.int_dialog.windowTitle(), "Edit list")
        self.assertEqual(self.bool_dialog.windowTitle(), "Edit list")
        self.assertEqual(self.double_dialog.windowTitle(), "Edit list")

    def test_set_list(self):
        dialog = self.string_dialog
        self.assertEqual(dialog.list_widget.count(), 2)
        dialog.set_list(["1", "2", "3"])
        self.assertEqual(dialog.list_widget.count(), 3)
        self.assertEqual(dialog.values, ["1", "2", "3"])

        dialog = self.bool_dialog
        self.assertEqual(dialog.list_widget.count(), 3)
        self.assertEqual(dialog.values, [True, False, True])
        dialog.set_list([])
        self.assertEqual(dialog.list_widget.count(), 0)
        self.assertEqual(dialog.values, [])

        dialog = self.int_dialog
        self.assertEqual(dialog.list_widget.count(), 3)
        dialog.set_list([1, 2, 3, 4, 5])
        self.assertEqual(dialog.list_widget.count(), 5)
        self.assertEqual(dialog.values, [1, 2, 3, 4, 5])

        dialog = self.double_dialog
        self.assertEqual(dialog.list_widget.count(), 3)
        dialog.set_list([1.1, 2.2, 3.3, 4.4, 5.5])
        self.assertEqual(dialog.list_widget.count(), 5)
        self.assertEqual(dialog.values, [1.1, 2.2, 3.3, 4.4, 5.5])

    def test_on_add_slot(self):
        """Test the add slot of the list edit dialog"""
        dialog = self.string_dialog
        with mock.patch("karabogui.dialogs.listedit.QInputDialog") as d:
            d.getText.return_value = "new", True
            dialog._add_clicked()
            self.assertEqual(dialog.list_widget.count(), 3)
        self.assertEqual(dialog.values, ["1", "2", "new"])

        dialog = self.int_dialog
        with mock.patch("karabogui.dialogs.listedit.QInputDialog") as d:
            d.getInt.return_value = 55, True
            dialog._add_clicked()
            self.assertEqual(dialog.list_widget.count(), 4)
        self.assertEqual(dialog.values, [1, 2, 3, 55])

        dialog = self.bool_dialog
        with mock.patch("karabogui.dialogs.listedit.QInputDialog") as d:
            # Item dialog with keys of choices, we return first choice
            d.getItem.return_value = "False", True
            dialog._add_clicked()
            self.assertEqual(dialog.list_widget.count(), 4)
        self.assertEqual(dialog.values, [True, False, True, False])

        dialog = self.double_dialog
        with mock.patch("karabogui.dialogs.listedit.QInputDialog") as d:
            d.getDouble.return_value = 42.2, True
            dialog._add_clicked()
            self.assertEqual(dialog.list_widget.count(), 4)
        self.assertEqual(dialog.values, [1.0, 2.0, 3.0, 42.2])

    def test_on_remove_slot(self):
        """Test the remove slot of the list edit dialog"""
        dialog = self.string_dialog
        self.assertEqual(dialog.list_widget.count(), 2)
        dialog.list_widget.setCurrentRow(0)
        dialog._remove_clicked()
        self.assertEqual(dialog.list_widget.count(), 1)
        self.assertEqual(dialog.values, ["2"])

        dialog = self.int_dialog
        self.assertEqual(dialog.list_widget.count(), 3)
        dialog.list_widget.setCurrentRow(0)
        dialog._remove_clicked()
        self.assertEqual(dialog.list_widget.count(), 2)
        self.assertEqual(dialog.values, [2, 3])

        dialog = self.bool_dialog
        self.assertEqual(dialog.list_widget.count(), 3)
        dialog.list_widget.setCurrentRow(0)
        dialog._remove_clicked()
        self.assertEqual(dialog.list_widget.count(), 2)
        # XXX: Booleans as integers
        self.assertEqual(dialog.values, [0, 1])

        dialog = self.double_dialog
        self.assertEqual(dialog.list_widget.count(), 3)
        dialog.list_widget.setCurrentRow(0)
        dialog._remove_clicked()
        self.assertEqual(dialog.list_widget.count(), 2)
        self.assertEqual(dialog.values, [2.0, 3.0])

    def test_move_up_slot(self):
        dialog = self.string_dialog
        dialog.list_widget.setCurrentRow(1)
        dialog._move_up_clicked()
        self.assertEqual(dialog.values, ["2", "1"])

        dialog = self.int_dialog
        dialog.list_widget.setCurrentRow(1)
        dialog._move_up_clicked()
        self.assertEqual(dialog.values, [2, 1, 3])

        dialog = self.bool_dialog
        dialog.list_widget.setCurrentRow(1)
        dialog._move_up_clicked()
        self.assertEqual(dialog.values, [0, 1, 1])

        dialog = self.double_dialog
        dialog.list_widget.setCurrentRow(1)
        dialog._move_up_clicked()
        self.assertEqual(dialog.values, [2.0, 1.0, 3.0])

    def test_move_down_slot(self):
        dialog = self.string_dialog
        dialog.list_widget.setCurrentRow(0)
        dialog._move_down_clicked()
        self.assertEqual(dialog.values, ["2", "1"])

        dialog = self.int_dialog
        dialog.list_widget.setCurrentRow(0)
        dialog._move_down_clicked()
        self.assertEqual(dialog.values, [2, 1, 3])

        dialog = self.bool_dialog
        dialog.list_widget.setCurrentRow(0)
        dialog._move_down_clicked()
        self.assertEqual(dialog.values, [0, 1, 1])

        dialog = self.double_dialog
        dialog.list_widget.setCurrentRow(0)
        dialog._move_down_clicked()
        self.assertEqual(dialog.values, [2.0, 1.0, 3.0])

    def test_edit_slot(self):
        dialog = self.string_dialog
        dialog.list_widget.setCurrentRow(0)
        with mock.patch("karabogui.dialogs.listedit.QInputDialog") as d:
            d.getText.return_value = "new", True
            dialog._edit_clicked()
            self.assertEqual(dialog.values, ["new", "2"])

        dialog = self.int_dialog
        dialog.list_widget.setCurrentRow(0)
        with mock.patch("karabogui.dialogs.listedit.QInputDialog") as d:
            d.getInt.return_value = 55, True
            dialog._edit_clicked()
            self.assertEqual(dialog.values, [55, 2, 3])

        dialog = self.bool_dialog
        dialog.list_widget.setCurrentRow(0)
        with mock.patch("karabogui.dialogs.listedit.QInputDialog") as d:
            # Item dialog with keys of choices, we return first choice
            d.getItem.return_value = "False", True
            dialog._edit_clicked()
            self.assertEqual(dialog.values, [0, 0, 1])

        dialog = self.double_dialog
        dialog.list_widget.setCurrentRow(0)
        with mock.patch("karabogui.dialogs.listedit.QInputDialog") as d:
            d.getDouble.return_value = 42.2, True
            dialog._edit_clicked()
            self.assertEqual(dialog.values, [42.2, 2.0, 3.0])

    def test_binding_validation(self):

        for dialog in (self.string_dialog, self.bool_dialog, self.int_dialog,
                       self.double_dialog):
            binding = dialog.binding
            values = string2list(dialog.string_values)
            assert binding.validate_trait("value", values) is not None, \
                   f"Validation of {values} for {binding} failed"


if __name__ == "__main__":
    main()
