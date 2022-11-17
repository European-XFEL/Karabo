from qtpy.QtCore import Qt

from karabogui.dialogs.api import ConfigurationDialog
from karabogui.singletons.configuration import Configuration
from karabogui.testing import GuiTestCase, singletons


class TestAppConfDialog(GuiTestCase):

    def test_basic_dialog(self):
        config = Configuration()
        with singletons(configuration=config):
            dialog = ConfigurationDialog()
            self.assertFalse(dialog.isModal())
            model = dialog.tree_view.model()
            self.assertIsNotNone(model)
            # We have 6 childen on root (groups)
            self.assertEqual(model.rowCount(), 6)
            group_index = model.index(1, 0)
            index = model.index(0, 0, group_index)
            self.assertIsNotNone(index.data())
            index = model.index(0, 1, group_index)
            self.assertIsNotNone(index.data())

            self.assertEqual(model.headerData(
                0, Qt.Horizontal, Qt.DisplayRole), 'Name')
            self.assertEqual(model.headerData(
                1, Qt.Horizontal, Qt.DisplayRole), 'Setting')

            flag = model.flags(index)
            self.assertEqual(int(flag), 33)
            self.assertEqual(flag & Qt.ItemIsEnabled, Qt.ItemIsEnabled)
            self.assertEqual(flag & Qt.ItemIsSelectable, Qt.ItemIsSelectable)
            self.assertNotEqual(flag & Qt.ItemIsEditable, Qt.ItemIsEditable)
            self.assertEqual(model.columnCount(None), 2)

            self.assertTrue(model.setData(index, "Karabo", Qt.EditRole))
            self.assertEqual(index.data(), "Karabo")

            # Check a boolean
            group_index = model.index(5, 0)
            self.assertEqual(group_index.data(), "user")
            index = model.index(6, 0, group_index)
            self.assertEqual(index.data(), "wizard")
            index_value = model.index(6, 1, group_index)
            flag = model.flags(index_value)
            self.assertEqual(int(flag), 49)
            self.assertEqual(flag & Qt.ItemIsEnabled, Qt.ItemIsEnabled)
            self.assertEqual(flag & Qt.ItemIsUserCheckable,
                             Qt.ItemIsUserCheckable)

            self.assertFalse(dialog.expanded)
            dialog.expandAll()
            self.assertTrue(dialog.expanded)

            # Double clicks, unfortunately, we cannot use QTest for that
            dialog.onDoubleClickHeader()
            self.assertFalse(dialog.expanded)
            dialog.onDoubleClickHeader()
            self.assertTrue(dialog.expanded)

    def test_model_tester(self):
        from pytestqt.modeltest import ModelTester
        config = Configuration()
        with singletons(configuration=config):
            dialog = ConfigurationDialog()
            model = dialog.tree_view.model()
            tester = ModelTester(None)
            tester.check(model)
