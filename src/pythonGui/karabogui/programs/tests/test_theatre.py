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

from karabogui.programs.theatre import DEVSCENE_PROG, create_theatre
from karabogui.singletons.network import Network
from karabogui.testing import singletons, system_hash
from karabogui.topology.api import SystemTopology
from karabogui.util import process_qt_events


def test_device_link_regex():
    match = DEVSCENE_PROG.match('SCOPE_GROUP_COMPONENT-2/TYPE/MEMBER|scene')
    assert (match and all(match.groups()))
    match = DEVSCENE_PROG.match('random-string')
    assert not (match and all(match.groups()))


namespace = namedtuple("namespace", "host port username nosplash "
                                    "scene_id timeout")


def test_theatre_normal(gui_app, mocker):
    """Test the karabo theatre with a topology"""
    ns = namespace("myhost", "myport", "admin", True,
                   ["divvy|scene"], 1)

    network = Network()
    network.connectToServerDirectly = mocker.Mock()
    network.connectToServerDirectly.return_value = True
    network.onSubscribeLogs = mocker.Mock()

    topology = SystemTopology()
    path = "karabogui.programs.theatre.get_scene_from_server"
    with singletons(network=network, topology=topology):
        scene = mocker.patch(path)
        success, waiter, app = create_theatre(ns)
        # Make sure no settings are erased in tests
        app.setOrganizationName("NoXFEL")

        assert success
        assert len(waiter.device_scenes) == 1
        network.signalServerConnectionChanged.emit(True)

        network.onSubscribeLogs.assert_not_called()
        # Initialize the trees
        topology.initialize(system_hash())
        process_qt_events(timeout=1000)
        scene.assert_called_with("divvy", "scene")


def test_theatre_timeout(gui_app, mocker):
    """Test the karabo theatre with a timeout for the topology"""
    ns = namespace("myhost", "myport", "admin", True,
                   ["divvy|scene"], 0)

    network = Network()
    network.connectToServerDirectly = mocker.Mock()
    network.connectToServerDirectly.return_value = True
    network.onSubscribeLogs = mocker.Mock()

    topology = SystemTopology()
    scene_path = "karabogui.programs.theatre.get_scene_from_server"
    box_path = "karabogui.programs.theatre.messagebox"
    with singletons(network=network, topology=topology):
        scene = mocker.patch(scene_path)
        mbox = mocker.patch(box_path)
        success, waiter, app = create_theatre(ns)
        # Make sure no settings are erased in tests
        app.setOrganizationName("NoXFEL")

        assert success
        assert len(waiter.device_scenes) == 1
        network.signalServerConnectionChanged.emit(True)

        process_qt_events(timeout=1000)
        scene.assert_not_called()
        mbox.show_warning.assert_called_once()
