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

from qtpy.QtWidgets import QApplication

from karabo.common.scenemodel.api import (
    DeviceSceneLinkModel, SceneModel, read_scene)
from karabogui import icons
from karabogui.controllers.api import populate_controller_registry
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.panels.configurationpanel import ConfigurationPanel
from karabogui.panels.navigationpanel import TopologyPanel
from karabogui.panels.projectpanel import ProjectPanel
from karabogui.panels.scenepanel import ScenePanel
from karabogui.singletons.api import (
    get_manager, get_mediator, get_network, get_panel_wrangler, get_topology)
from karabogui.util import getOpenFileName


def run_configurator(ns):
    device_id = ns.configurator
    device = get_topology().get_device(device_id)
    broadcast_event(KaraboEvent.ShowConfiguration, {'proxy': device})
    return ConfigurationPanel(), (600, 800)


def run_navigation():
    return TopologyPanel(), (400, 600)


def run_project():
    return ProjectPanel(), (300, 500)


def run_scene():
    filename = getOpenFileName(filter='*.svg')
    if not filename:
        sys.exit()
    return run_scene_file(filename)


def run_scene_file(filename):
    model = read_scene(filename)
    panel = ScenePanel(model, True)

    return panel, (1024, 768)


def run_scene_link(ns):
    device_id, scene_name = ns.dev_scene_link.split('|')
    keys = [f"{device_id}.availableScenes"]
    link = DeviceSceneLinkModel(
        keys=keys, target=scene_name, text=ns.dev_scene_link, width=200)
    model = SceneModel(children=[link])
    panel = ScenePanel(model, True)

    return panel, (1024, 768)


def run_panel(ns):
    app = QApplication(sys.argv)
    # Run the lazy initializers (icons, widget controllers)
    icons.init()
    populate_controller_registry()

    # Init some singletons
    get_mediator()
    get_manager()

    if ns.configurator:
        panel, size = run_configurator(ns)
    elif ns.dev_scene_link:
        panel, size = run_scene_link(ns)
    elif ns.navigation:
        panel, size = run_navigation()
    elif ns.project:
        panel, size = run_project()
    elif ns.scene_chooser:
        panel, size = run_scene()
    elif ns.scene_file:
        panel, size = run_scene_file(
            ns.scene_file)

    # XXX: A hack to keep the toolbar visible
    panel.toolbar.setVisible(True)
    panel.show()
    panel.resize(*size)

    # Init the panel wrangler singleton
    get_panel_wrangler()

    # Connect to the GUI Server
    get_network().connectToServer()
    sys.exit(app.exec())


def main():
    ap = argparse.ArgumentParser(description='GUI Panel Runner')
    ag = ap.add_mutually_exclusive_group(required=True)
    ag.add_argument('-c', '--configurator', type=str, metavar='DEVICE_ID',
                    help='A device ID should be provided')
    ag.add_argument(
        '-l', '--dev_scene_link', type=str, metavar='DEVICE|SCENENAME')
    ag.add_argument('-n', '--navigation', action='store_true')
    ag.add_argument('-p', '--project', action='store_true')
    ag.add_argument('-s', '--scene_chooser', action='store_true')
    ag.add_argument('-S', '--scene_file', type=str, metavar='FILENAME')
    run_panel(ap.parse_args())


if __name__ == '__main__':
    main()
