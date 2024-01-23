# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from pytestqt.modeltest import ModelTester
from qtpy.QtCore import QItemSelectionModel, QPoint, Qt

from karabogui.dialogs.api import DevelopmentTopologyDialog
from karabogui.testing import development_system_hash, singletons
from karabogui.topology.system_topology import SystemTopology


def test_development_topology_dialog(gui_app, mocker):
    topology = SystemTopology()
    topology.initialize(development_system_hash())
    manager = mocker.Mock()
    with singletons(topology=topology, manager=manager):
        dialog = DevelopmentTopologyDialog()
        assert not dialog.isModal()
        model = dialog.tree_view.model()
        assert model is not None
        # We have 1 client
        assert model.rowCount() == 1
        client_index = model.index(0, 0)
        assert client_index.data() == "devserver"

        assert model.headerData(
            0, Qt.Horizontal, Qt.DisplayRole) == "Server Id"
        assert model.headerData(1, Qt.Horizontal, Qt.DisplayRole) == "Host"

        flag = model.flags(client_index)
        assert flag & Qt.ItemIsEnabled == Qt.ItemIsEnabled
        assert flag & Qt.ItemIsSelectable == Qt.ItemIsSelectable
        assert flag & Qt.ItemIsEditable != Qt.ItemIsEditable

        # Check context menu without selection
        pos = QPoint(0, 0)
        menu = mocker.patch("karabogui.dialogs.client_topology.QMenu")
        dialog._context_menu(pos)
        menu.assert_not_called()

        # Select the client and kill!
        selection = dialog.tree_view.selectionModel()
        selection.setCurrentIndex(client_index,
                                  QItemSelectionModel.ClearAndSelect)
        dialog.onKillInstance()
        assert manager.shutdownServer.call_count == 1
        manager.shutdownServer.assert_called_with("devserver", parent=dialog)


def test_model_tester():
    topology = SystemTopology()
    topology.initialize(development_system_hash())
    with singletons(topology=topology):
        dialog = DevelopmentTopologyDialog()
        model = dialog.tree_view.model()
        tester = ModelTester(None)
        tester.check(model)
