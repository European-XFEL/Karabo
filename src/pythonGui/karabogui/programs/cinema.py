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
import sys

from qtpy.QtCore import Slot

import karabogui.const as global_constants
from karabo.common.scenemodel.api import SceneTargetWindow
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.messagebox import show_error
from karabogui.programs.base import create_gui_app, init_gui
from karabogui.programs.utils import close_app
from karabogui.singletons.api import get_db_conn, get_network, get_topology


def create_cinema(ns):
    app = create_gui_app(sys.argv)
    init_gui(app, use_splash=not ns.nosplash)

    def trigger_scenes():
        topology.system_tree.on_trait_change(
            trigger_scenes, 'initialized', remove=True)
        domain = ns.domain
        get_db_conn().default_domain = domain
        for uuid in ns.scene_uuid:
            db_scene = {'name': f'{domain}-{uuid}',
                        'target_window': SceneTargetWindow.MainWindow,
                        'target': uuid}
            broadcast_event(KaraboEvent.OpenSceneLink, db_scene)

    global_constants.APPLICATION_MODE = True

    topology = get_topology()
    # Attach to the topology
    topology.system_tree.on_trait_change(
        trigger_scenes, 'initialized')
    network = get_network()

    @Slot()
    def _connect_handler(status):
        """Connect handler for direct connect attempts without dialog"""
        network.signalServerConnectionChanged.disconnect(_connect_handler)
        if not status:
            show_error("Error, could not connect to gui server "
                       f"<b>{ns.host}:{ns.port}</b>. Closing karabo cinema.")
            topology.system_tree.on_trait_change(
                trigger_scenes, 'initialized', remove=True)
            close_app()

    # We might want to connect directly to the gui server
    if ns.host and ns.port:
        network.signalServerConnectionChanged.connect(_connect_handler)
        success = network.connectToServerDirectly(
           hostname=ns.host, port=ns.port)
    else:
        # Connect to the GUI Server via dialog
        success = network.connectToServer()

    return success, app


def run_cinema(ns):
    """The cinema is meant to directly download a scene from the project db

    From the initial scene the operator is allowed to request additional
    scene or device scene links.

    All scenes have the name ProjectDB|SceneName and are not editable!
    """
    success, app = create_cinema(ns)
    if success:
        app.exec()
        app.deleteLater()
        sys.exit()
    else:
        close_app()


def main():
    ap = argparse.ArgumentParser(description='Karabo Cinema')
    ap.add_argument('domain', type=str,
                    help='The domain where to look for the initial scene')
    ap.add_argument('scene_uuid', type=str, nargs='+',
                    help='The uuids of the scenes. This can be either a single'
                         'uuid or a sequence of uuids separated with a space')
    ap.add_argument('-host', '--host', type=str,
                    help='The hostname of the gui server to connect')
    ap.add_argument('-port', '--port', type=int,
                    help='The port number of the gui server to connect')
    ap.add_argument('-nosplash', '--nosplash', action='store_true')
    run_cinema(ap.parse_args())


if __name__ == '__main__':
    main()
