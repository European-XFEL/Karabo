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
import argparse
import re
import sys

from qtpy.QtCore import QObject, QTimer, Slot

import karabogui.const as global_constants
from karabo.common.api import Capabilities
from karabogui import messagebox
from karabogui.programs.base import create_gui_app, init_gui
from karabogui.programs.utils import close_app
from karabogui.request import get_scene_from_server
from karabogui.singletons.api import get_network, get_topology

DEVSCENE_PROG = re.compile(r"([0-9a-zA-Z/_\-]+)\|(.+)")
CAPA = Capabilities.PROVIDES_SCENES


class DeviceWaiter(QObject):
    """
    An helper class that listens to the system topology and acts accordingly.

    The constructor will parse the list of sceneIds for the theather as well
    """

    def __init__(self, scene_ids, timeout, parent=None):
        super().__init__(parent=parent)
        # a dictionary of list of scene names indexed by deviceId
        self.device_scenes = {}
        # track if scenes are open
        self.no_scenes = True
        self.not_capable_devices = []
        self.parse_scene_ids(scene_ids)
        self.timeout = timeout

        # Attach event to topology
        topology = get_topology()
        topology.system_tree.on_trait_change(
            self._topo_update, 'initialized')

    def is_valid(self):
        return len(self.device_scenes) > 0

    def parse_scene_ids(self, scene_ids):
        """parses the sceneIds and will provide a messagebox for errors"""
        for scene_id in scene_ids:
            match = DEVSCENE_PROG.match(scene_id)
            if match and all(match.groups()):
                device_id = match.group(1)
                scene_name = match.group(2)
                scene_names = self.device_scenes.setdefault(device_id, set())
                scene_names.add(scene_name)
        # no scenes are matching the regex, exit!.
        if not self.device_scenes:
            msg = "{} are not valid sceneIds.".format(
                ','.join([scene for scene in scene_ids]))
            messagebox.show_warning(msg, title='Theater')
            close_app()

    def start(self):
        self.timer = QTimer(self)
        # timeout in case devices are not found.
        self.timer.setSingleShot(True)
        self.timer.timeout.connect(self._timeout_handler)
        self.timer.start(self.timeout * 1000)

    def open_scenes(self, device_id, scene_names):
        for scene_name in scene_names:
            get_scene_from_server(device_id, scene_name)
            self.no_scenes = False

    @Slot()
    def _timeout_handler(self):
        # stop listening to the topology
        topology = get_topology()
        topology.system_tree.on_trait_change(
            self._topo_update, 'initialized', remove=True)

        # this dialog building is rather complex but will avoid
        # multiple error dialogs to the user.
        msg = ""
        if self.device_scenes:
            pending_scenes = []
            for device_id, scene_names in self.device_scenes.items():
                pending_scenes.extend([f'{device_id}|{scene}'
                                       for scene in scene_names])
            msg += "Could not open the following scenes: {}. \n".format(
                ',\n'.join(pending_scenes))

        missing = ',\n'.join([device_id for device_id in self.device_scenes])
        if missing:
            msg += "The following deviceIds are not present " \
                   "in the system topology: {}. \n".format(missing)
        incapables = ',\n'.join([device_id
                                 for device_id in self.not_capable_devices])
        if incapables:
            msg += "The following deviceIds do not provide " \
                   "scenes: {}. \n".format(incapables)

        # open a dialog for the missing scenes
        if msg:
            msg += "Please review command line arguments accordingly"
            messagebox.show_warning(msg, title='Theater')
        # no scenes are open after the timeout.
        if self.no_scenes:
            close_app()

    def _topo_update(self):
        topology = get_topology()
        topology.system_tree.on_trait_change(
            self._topo_update, 'initialized', remove=True)

        def alive_visitor(node):
            if node.node_id not in self.device_scenes:
                return
            scenes = self.device_scenes.pop(node.node_id)
            if (node.capabilities & CAPA) == CAPA:
                self.open_scenes(node.node_id, scenes)
            else:
                self.not_capable_devices.append(node.node_id)

        topology.visit_system_tree(alive_visitor)
        # Warning message if provided device is invalid or has no scene.
        non_exisiting_device = False
        no_capable_service = False
        msg = ""
        if self.no_scenes:
            missing = ',\n'.join(
                [device_id for device_id in self.device_scenes])
            if missing:
                msg += "The following deviceIds are not present " \
                       f"in the system topology: '{missing}'. \n\n"
                non_exisiting_device = True
        if self.not_capable_devices:
            incapables = ',\n'.join([device_id for device_id in
                                     self.not_capable_devices])
            if incapables:
                msg += ("The following deviceIds do not provide scenes: "
                        f"'{incapables}'. \n\n")
            no_capable_service = True
        if msg:
            msg += "Please review command line arguments accordingly."
            messagebox.show_warning(msg, title='Theater')
        #  All the specified devices are not available in the topology or have
        #  no scene,  exit!
        if non_exisiting_device and no_capable_service:
            close_app()


def create_theatre(ns):
    app = create_gui_app(sys.argv)
    init_gui(app, use_splash=not ns.nosplash)

    global_constants.APPLICATION_MODE = True
    network = get_network()
    waiter = DeviceWaiter(ns.scene_id, ns.timeout)

    @Slot()
    def _connect_handler(status):
        """Connect handler for direct connect attempts without dialog"""
        network.signalServerConnectionChanged.disconnect(_connect_handler)
        if not status:
            messagebox.show_error("Error, could not connect to gui server "
                                  f"<b>{ns.host}:{ns.port}</b>. "
                                  "Closing karabo theare.")
            close_app()
        else:
            # We are connected and charge a timeout now!
            waiter.start()

    # We might want to connect directly to the gui server
    if ns.host and ns.port:
        network.signalServerConnectionChanged.connect(_connect_handler)
        success = get_network().connectToServerDirectly(
            hostname=ns.host, port=ns.port)
    else:
        # Connect to the GUI Server via dialog
        success = network.connectToServer()

    return success, waiter, app


def run_theatre(ns):
    """
    The theatre downloads and opens scenes provided by devices.

    All scenes have the name deviceId|sceneName and are not editable!
    """
    success, waiter, app = create_theatre(ns)
    if success and waiter.is_valid():
        app.exec()
        app.deleteLater()
        sys.exit()
    else:
        close_app()


def main():
    ap = argparse.ArgumentParser(description='Karabo Theater')
    ap.add_argument('scene_id', type=str, nargs='+',
                    help='The scene ids. Device provided scene '
                         'formatted like: "deviceId|sceneName".')
    ap.add_argument('-host', '--host', type=str,
                    help='The hostname of the gui server to connect')
    ap.add_argument('-port', '--port', type=int,
                    help='The port number of the gui server to connect')
    ap.add_argument('-timeout', '--timeout', type=int, default=10,
                    help='The timeout in seconds to be waited for the devices'
                         'to be visible')
    ap.add_argument('-nosplash', '--nosplash', action='store_true')
    run_theatre(ap.parse_args())


if __name__ == '__main__':
    main()
