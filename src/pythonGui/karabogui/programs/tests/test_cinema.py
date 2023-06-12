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
from unittest import main, mock

from karabo.common.scenemodel.api import SceneTargetWindow
from karabogui.events import KaraboEvent
from karabogui.programs.cinema import create_cinema
from karabogui.singletons.network import Network
from karabogui.testing import GuiTestCase, singletons, system_hash
from karabogui.topology.api import SystemTopology

namespace = namedtuple("namespace", "host port username domain nosplash "
                                    "scene_uuid timeout")


class TestCinemaApplication(GuiTestCase):

    def test_cinema_normal(self):
        """Test the karabo cinema with a direct connect"""
        ns = namespace("myhost", "myport", "admin", "CTRL", True,
                       ["uuid-1231231321"], 1)

        network = Network()
        network.connectToServerDirectly = mock.Mock()
        network.connectToServerDirectly.return_value = True
        network.onSubscribeLogs = mock.Mock()
        topology = SystemTopology()
        path = "karabogui.programs.cinema.broadcast_event"
        with singletons(network=network, topology=topology):
            with mock.patch(path) as broadcast:
                success, app = create_cinema(ns)
                # Make sure no settings are erased in tests
                app.setOrganizationName("NoXFEL")

                self.assertEqual(success, True)
                network.signalServerConnectionChanged.emit(True)

                network.onSubscribeLogs.assert_not_called()

                # Initialize the trees
                topology.initialize(system_hash())
                broadcast.assert_called_with(
                    KaraboEvent.OpenSceneLink,
                    {'name': 'CTRL-uuid-1231231321',
                     'target_window': SceneTargetWindow.MainWindow,
                     'target': 'uuid-1231231321'})

                # Must be called after topology
                network.onSubscribeLogs.assert_called_with(False)


if __name__ == "__main__":
    main()
