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

from karabogui.programs.theatre import DEVSCENE_PROG, create_theatre
from karabogui.singletons.network import Network
from karabogui.testing import GuiTestCase, singletons, system_hash
from karabogui.topology.api import SystemTopology


def test_device_link_regex():
    match = DEVSCENE_PROG.match('SCOPE_GROUP_COMPONENT-2/TYPE/MEMBER|scene')
    assert (match and all(match.groups()))
    match = DEVSCENE_PROG.match('random-string')
    assert not (match and all(match.groups()))


namespace = namedtuple("namespace", "host port username nosplash "
                                    "scene_id timeout")


class TestExtraApplication(GuiTestCase):

    def test_theatre_normal(self):
        """Test the karabo theatre with a topology"""
        ns = namespace("myhost", "myport", "admin", True,
                       ["divvy|scene"], 1)

        network = Network()
        network.connectToServerDirectly = mock.Mock()
        network.connectToServerDirectly.return_value = True
        network.onSubscribeLogs = mock.Mock()

        topology = SystemTopology()
        path = "karabogui.programs.theatre.get_scene_from_server"
        with singletons(network=network, topology=topology):
            with mock.patch(path) as scene:
                success, waiter, app = create_theatre(ns)
                # Make sure no settings are erased in tests
                app.setOrganizationName("NoXFEL")

                self.assertTrue(success)
                self.assertEqual(len(waiter.device_scenes), 1)
                network.signalServerConnectionChanged.emit(True)

                network.onSubscribeLogs.assert_not_called()
                # Initialize the trees
                topology.initialize(system_hash())
                self.process_qt_events(1000)
                scene.assert_called_with("divvy", "scene")

                network.onSubscribeLogs.assert_called_with(False)

    def test_theatre_timeout(self):
        """Test the karabo theatre with a timeout for the topology"""
        ns = namespace("myhost", "myport", "admin", True,
                       ["divvy|scene"], 0)

        network = Network()
        network.connectToServerDirectly = mock.Mock()
        network.connectToServerDirectly.return_value = True
        network.onSubscribeLogs = mock.Mock()

        topology = SystemTopology()
        scene_path = "karabogui.programs.theatre.get_scene_from_server"
        box_path = "karabogui.programs.theatre.messagebox"
        with singletons(network=network, topology=topology):
            with mock.patch(scene_path) as scene, mock.patch(box_path) as mbox:
                success, waiter, app = create_theatre(ns)
                # Make sure no settings are erased in tests
                app.setOrganizationName("NoXFEL")

                self.assertTrue(success)
                self.assertEqual(len(waiter.device_scenes), 1)
                network.signalServerConnectionChanged.emit(True)

                self.process_qt_events(1000)
                scene.assert_not_called()
                mbox.show_warning.assert_called_once()
                network.onSubscribeLogs.assert_not_called()


if __name__ == "__main__":
    main()
