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
from qtpy.QtCore import QItemSelectionModel

from karabogui.testing import click_button, singletons, system_hash
from karabogui.topology.api import SystemTopology

from ..topology_device_dialog import TopologyDeviceDialog


def test_device_dialog(gui_app):
    topology = SystemTopology()
    topology.initialize(system_hash())
    with singletons(topology=topology):
        dialog = TopologyDeviceDialog()
        assert dialog.device_id == ""
        assert dialog.filter_model.rowCount() == 2
        selection = dialog.ui_devices_view.selectionModel()
        model = dialog.filter_model
        index = model.index(0, 0)
        selection.setCurrentIndex(index, QItemSelectionModel.ClearAndSelect)
        dialog.done(1)
        assert dialog.device_id == "divvy"

        dialog.filter_model.setFilterFixedString("nooo")
        assert dialog.filter_model.rowCount() == 0
        # Clear to get previous selection
        click_button(dialog.ui_clear)
        assert dialog.filter_model.rowCount() == 2
        dialog.ui_interface.setCurrentIndex(1)
        # Interface, all filtered
        assert dialog.filter_model.rowCount() == 0
        click_button(dialog.ui_clear)
        assert dialog.filter_model.rowCount() == 2
