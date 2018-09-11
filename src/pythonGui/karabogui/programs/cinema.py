import argparse
import sys

from PyQt4.QtGui import QApplication

from karabo.common.scenemodel.api import SceneTargetWindow
from karabogui import icons
from karabogui.controllers.api import populate_controller_registry
from karabogui.events import broadcast_event, KaraboEventSender
from karabogui.singletons.api import (
    get_db_conn, get_manager, get_mediator, get_panel_wrangler, get_network)


def run_cinema(ns):
    """The cinema is meant to directly download a scene from the project db

    From the initial scene the operator is allowed to request additional
    scene or device scene links.

    All scenes have the name ProjectDB|SceneName and are not editable!
    """
    app = QApplication(sys.argv)
    # Run the lazy initializers (icons, widget controllers)
    icons.init()
    populate_controller_registry()

    # Init some singletons
    get_mediator()
    get_manager()

    # Init the panel wrangler singleton
    get_panel_wrangler()

    # Connect to the GUI Server
    success = get_network().connectToServer()
    if success:
        get_db_conn().default_domain = ns.domain
        db_scene = {'name': "Cinema",
                    'target_window': SceneTargetWindow.MainWindow,
                    'target': ns.scene_uuid}
        broadcast_event(KaraboEventSender.OpenSceneLink, db_scene)

        sys.exit(app.exec_())
    else:
        # If we are not successful in connection, we don't leave a remnant!
        app.quit()


def main():
    ap = argparse.ArgumentParser(description='Karabo Cinema')
    ap.add_argument('domain', type=str,
                    help='The domain where to look for the initial scene')
    ap.add_argument('scene_uuid', type=str,
                    help='The uuid of the initial scene')
    run_cinema(ap.parse_args())


if __name__ == '__main__':
    main()
