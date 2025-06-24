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

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QStyle

from karabo.native import Hash
from karabogui.binding.api import ProxyStatus
from karabogui.dialogs.api import ProjectTopologyDialog
from karabogui.testing import singletons, system_hash
from karabogui.topology.api import SystemTopology


def test_project_topology_dialog(gui_app):
    topology = SystemTopology()
    topology.initialize(system_hash())
    with singletons(topology=topology):
        dialog = ProjectTopologyDialog()

        devices = [Hash(
            "device_name", "divvy",
            "device_class", "Fooclass",
            "server_name", "swerver",
            "device_uuid", "test-uuid",
            "project_name", "KaraboRocks")]

        h = Hash("items", devices)
        dialog.initialize(h)
        assert dialog.table_model.rowCount() == 1
        assert dialog.table_model.columnCount() == 6

        assert dialog.filter_model.rowCount() == 1
        assert dialog.table_model.index(0, 0).data(
            Qt.UserRole) == ProxyStatus.NOPLUGIN
        assert dialog.table_model.index(0, 1).data(Qt.UserRole) == "divvy"
        assert dialog.table_model.index(0, 1).data(Qt.DisplayRole) == "divvy"

        index = dialog.table_model.index(0, 1)
        state = dialog.delegate.button_state(index)
        assert state == QStyle.State_On
