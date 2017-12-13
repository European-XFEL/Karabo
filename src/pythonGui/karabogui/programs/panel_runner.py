import argparse
import sys

from PyQt4.QtGui import QApplication

from karabo.common.scenemodel.api import read_scene
from karabogui import icons
from karabogui.controllers.util import populate_controller_registry
from karabogui.events import broadcast_event, KaraboEventSender
from karabogui.panels.configurationpanel import ConfigurationPanel
from karabogui.panels.navigationpanel import NavigationPanel
from karabogui.panels.projectpanel import ProjectPanel
from karabogui.panels.scenepanel import ScenePanel
from karabogui.singletons.api import (
    get_manager, get_mediator, get_panel_wrangler, get_network, get_topology)
from karabogui.util import getOpenFileName


def run_configurator(ns):
    device_id = ns.configurator
    device = get_topology().get_device(device_id)
    broadcast_event(KaraboEventSender.ShowConfiguration, {'proxy': device})
    return ConfigurationPanel(), (600, 800)


def run_navigation():
    return NavigationPanel(), (400, 600)


def run_project():
    return ProjectPanel(), (300, 500)


def run_scene():
    filename = getOpenFileName(filter='*.svg')
    if not filename:
        sys.exit()

    model = read_scene(filename)
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
    elif ns.navigation:
        panel, size = run_navigation()
    elif ns.project:
        panel, size = run_project()
    elif ns.scene:
        panel, size = run_scene()

    # XXX: A hack to keep the toolbar visible
    panel.toolbar.setVisible(True)
    panel.show()
    panel.resize(*size)

    # Init the panel wrangler singleton
    get_panel_wrangler()

    # Connect to the GUI Server
    get_network().connectToServer()
    sys.exit(app.exec_())


def main():
    ap = argparse.ArgumentParser(description='GUI Panel Runner')
    ag = ap.add_mutually_exclusive_group(required=True)
    ag.add_argument('-c', '--configurator', type=str, metavar='DEVICE_ID',
                    help='A device ID should be provided')
    ag.add_argument('-n', '--navigation', action='store_true')
    ag.add_argument('-p', '--project', action='store_true')
    ag.add_argument('-s', '--scene', action='store_true')
    run_panel(ap.parse_args())


if __name__ == '__main__':
    main()
