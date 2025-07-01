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
from karabogui.testing import (
    set_test_organization_info, singletons, system_hash)
from karabogui.topology.api import SystemTopology
from karabogui.util import process_qt_events


def test_device_link_regex():
    match = DEVSCENE_PROG.match('SCOPE_GROUP_COMPONENT-2/TYPE/MEMBER|scene')
    assert (match and all(match.groups()))
    match = DEVSCENE_PROG.match('random-string')
    assert not (match and all(match.groups()))


namespace = namedtuple("namespace", "host port nosplash "
                                    "scene_id timeout")


def test_theatre_normal(gui_app, mocker):
    """Test the karabo theatre with a topology"""
    ns = namespace("myhost", "myport", True, ["divvy|scene"], 1)

    network = Network()
    network.connectToServerDirectly = mocker.Mock()
    network.connectToServerDirectly.return_value = True
    network.onSubscribeLogs = mocker.Mock()
    info = mocker.patch("karabogui.programs.base.set_app_info")

    topology = SystemTopology()
    path = "karabogui.programs.theatre.get_scene_from_server"
    with singletons(network=network, topology=topology):
        scene = mocker.patch(path)
        success, waiter, app = create_theatre(ns)
        info.assert_called_with(app, "XFEL", "xfel.eu", "KaraboGUI")
        # Make sure no settings are erased in tests
        set_test_organization_info(app)

        assert success
        assert len(waiter.device_scenes) == 1
        network.signalServerConnectionChanged.emit(True)

        network.onSubscribeLogs.assert_not_called()
        # Initialize the trees
        topology.initialize(system_hash())
        process_qt_events(timeout=1000)
        scene.assert_called_with("divvy", "scene")
    network.deleteLater()


def test_theatre_timeout(gui_app, mocker):
    """Test the karabo theatre with a timeout for the topology"""
    ns = namespace("myhost", "myport", True, ["divvy|scene"], 0)

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
        set_test_organization_info(app)

        assert success
        assert len(waiter.device_scenes) == 1
        network.signalServerConnectionChanged.emit(True)

        process_qt_events(timeout=1000)
        scene.assert_not_called()
        mbox.show_warning.assert_called_once()
    network.deleteLater()


def test_missing_device(gui_app, mocker):
    """Verify the warning message is displyed when device is invalid or has
    no scene."""
    ns = namespace("myhost", "myport", True, ["divvy|scene"], 1)

    network = Network()
    network.connectToServerDirectly = mocker.Mock()
    network.connectToServerDirectly.return_value = True
    network.onSubscribeLogs = mocker.Mock()

    topology = SystemTopology()
    waring_dialog = "karabogui.programs.theatre.messagebox.show_warning"
    with singletons(network=network, topology=topology):
        success, waiter, app = create_theatre(ns)
        set_test_organization_info(app)

        mbox = mocker.patch(waring_dialog)

        # Device doesn't exist in the Topology.
        waiter._topo_update()
        msg = ("The following deviceIds are not present in the system "
               "topology: 'divvy'. \n\nPlease review command line arguments "
               "accordingly.")
        assert mbox.call_count == 1
        mbox.assert_called_with(msg, title="Theater")

        mbox.reset_mock()
        waiter.no_scenes = False
        waiter._topo_update()
        assert mbox.call_count == 0

        # Device with no Scene
        waiter.not_capable_devices = ["FooClass"]
        waiter._topo_update()
        msg = ("The following deviceIds do not provide scenes: 'FooClass'. "
               "\n\nPlease review command line arguments accordingly.")
        assert mbox.call_count == 1
        mbox.assert_called_with(msg, title="Theater")
