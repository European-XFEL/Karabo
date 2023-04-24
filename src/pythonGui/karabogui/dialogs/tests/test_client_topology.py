# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest import main, mock

from qtpy.QtCore import QItemSelectionModel, QPoint, Qt

from karabogui.dialogs.api import ClientTopologyDialog
from karabogui.testing import GuiTestCase, singletons, system_hash
from karabogui.topology.system_topology import SystemTopology


class TestClientTopology(GuiTestCase):

    def test_basic_dialog(self):
        topology = SystemTopology()
        topology.initialize(system_hash())
        manager = mock.Mock()
        with singletons(topology=topology, manager=manager):
            dialog = ClientTopologyDialog()
            self.assertFalse(dialog.isModal())
            model = dialog.tree_view.model()
            self.assertIsNotNone(model)
            # We have 1 client
            self.assertEqual(model.rowCount(), 1)
            client_index = model.index(0, 0)
            self.assertEqual(client_index.data(), "charlie")

            self.assertEqual(model.headerData(
                0, Qt.Horizontal, Qt.DisplayRole), 'Client Id')
            self.assertEqual(model.headerData(
                1, Qt.Horizontal, Qt.DisplayRole), 'Host')

            flag = model.flags(client_index)
            self.assertEqual(flag & Qt.ItemIsEnabled, Qt.ItemIsEnabled)
            self.assertEqual(flag & Qt.ItemIsSelectable, Qt.ItemIsSelectable)
            self.assertNotEqual(flag & Qt.ItemIsEditable, Qt.ItemIsEditable)

            # Check context menu without selection
            pos = QPoint(0, 0)
            with mock.patch("karabogui.dialogs.client_topology.QMenu") as menu:
                dialog._context_menu(pos)
                menu.assert_not_called()

            # Select the client and kill!
            selection = dialog.tree_view.selectionModel()
            selection.setCurrentIndex(client_index,
                                      QItemSelectionModel.ClearAndSelect)
            dialog.onKillInstance()
            manager.shutdownDevice.assert_called_with("charlie",
                                                      parent=dialog)

            # Check context menu with selection!
            pos = QPoint(0, 0)
            with mock.patch("karabogui.dialogs.client_topology.QMenu") as menu:
                dialog._context_menu(pos)
                menu.assert_called_once()

    def test_model_tester(self):
        from pytestqt.modeltest import ModelTester
        topology = SystemTopology()
        topology.initialize(system_hash())
        with singletons(topology=topology):
            dialog = ClientTopologyDialog()
            model = dialog.tree_view.model()
            tester = ModelTester(None)
            tester.check(model)


if __name__ == "__main__":
    main()
