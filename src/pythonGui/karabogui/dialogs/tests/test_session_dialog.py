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

from karabo.native import Hash
from karabogui.dialogs.api import GuiSessionInfo
from karabogui.testing import singletons, system_hash
from karabogui.topology.system_topology import SystemTopology


def test_session_dialog(gui_app, mocker):
    topology = SystemTopology()
    h = system_hash()
    devices = h["device"]
    devices.setElement("guidevice", Hash(), {"host": "BigIron",
                                             "classId": "GuiServerDevice",
                                             "capabilities": 0,
                                             "serverId": "karabo/GuiServer",
                                             "status": "ok"})
    topology.initialize(h)
    network = mocker.Mock()
    with singletons(topology=topology, network=network):
        # 1. Test init without deviceId as argument
        dialog = GuiSessionInfo()
        assert not dialog.isModal()
        assert dialog.list_widget.count() == 1

        item = dialog.list_widget.currentItem()
        assert item.data(Qt.DisplayRole) == "guidevice"
        network.onExecuteGeneric.assert_called_once()

        call_args = network.onExecuteGeneric.call_args
        args = call_args[0]
        assert args[0] == "guidevice"
        assert args[1] == "slotGetClientSessions"

        # Finish dialog!
        dialog.done(1)
