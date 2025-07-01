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
from collections import namedtuple

from karabo.common.scenemodel.api import SceneTargetWindow
from karabogui.events import KaraboEvent
from karabogui.programs.cinema import create_cinema
from karabogui.singletons.network import Network
from karabogui.testing import (
    set_test_organization_info, singletons, system_hash)
from karabogui.topology.api import SystemTopology

namespace = namedtuple("namespace", "host port domain nosplash "
                                    "scene_uuid timeout")


def test_cinema_normal(gui_app, mocker):
    """Test the karabo cinema with a direct connect"""
    ns = namespace("myhost", "myport", "CTRL", True, ["uuid-1231231321"], 1)

    network = Network()
    network.connectToServerDirectly = mocker.Mock()
    network.connectToServerDirectly.return_value = True
    network.onSubscribeLogs = mocker.Mock()
    info = mocker.patch("karabogui.programs.base.set_app_info")
    topology = SystemTopology()
    path = "karabogui.programs.cinema.broadcast_event"
    with singletons(network=network, topology=topology):
        broadcast = mocker.patch(path)
        success, app = create_cinema(ns)
        info.assert_called_with(app, "XFEL", "xfel.eu", "KaraboGUI")
        # Make sure no settings are erased in tests
        set_test_organization_info(app)
        assert success
        network.signalServerConnectionChanged.emit(True)

        network.onSubscribeLogs.assert_not_called()

        # Initialize the trees
        topology.initialize(system_hash())
        broadcast.assert_called_with(
            KaraboEvent.OpenSceneLink,
            {'name': 'CTRL-uuid-1231231321',
             'target_window': SceneTargetWindow.MainWindow,
             'target': 'uuid-1231231321'})
