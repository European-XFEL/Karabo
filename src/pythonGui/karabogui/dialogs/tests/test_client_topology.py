# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from pytestqt.modeltest import ModelTester
from qtpy.QtCore import QItemSelectionModel, QPoint, Qt

from karabogui.dialogs.api import ClientTopologyDialog
from karabogui.testing import singletons, system_hash
from karabogui.topology.system_topology import SystemTopology


def test_basic_dialog(gui_app, mocker):
    topology = SystemTopology()
    topology.initialize(system_hash())
    manager = mocker.Mock()
    with singletons(topology=topology, manager=manager):
        dialog = ClientTopologyDialog()
        assert not dialog.isModal()
        model = dialog.tree_view.model()
        assert model is not None
        # We have 1 client
        assert model.rowCount() == 1
        client_index = model.index(0, 0)
        assert client_index.data() == "charlie"

        assert model.headerData(
            0, Qt.Horizontal, Qt.DisplayRole) == 'Client Id'
        assert model.headerData(1, Qt.Horizontal, Qt.DisplayRole) == 'Host'

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
        manager.shutdownDevice.assert_called_with("charlie",
                                                  parent=dialog)

        # Check context menu with selection!
        pos = QPoint(0, 0)
        dialog._context_menu(pos)
        menu.assert_called_once()


def test_model_tester():
    topology = SystemTopology()
    topology.initialize(system_hash())
    with singletons(topology=topology):
        dialog = ClientTopologyDialog()
        model = dialog.tree_view.model()
        tester = ModelTester(None)
        tester.check(model)
