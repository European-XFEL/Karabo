import argparse
import os.path as op
import sys

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QApplication, QIcon, QPixmap, QSplashScreen

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
    # set a nice app logo
    logo_path = op.join(op.dirname(__file__), '..', "icons", "app_logo.png")
    app.setWindowIcon(QIcon(logo_path))

    # These should be set to simplify QSettings usage
    app.setOrganizationName('XFEL')
    app.setOrganizationDomain('xfel.eu')
    app.setApplicationName('KaraboGUI')

    splash_path = op.join(op.dirname(__file__), '..', "icons", "splash.png")
    splash_img = QPixmap(splash_path)
    splash = QSplashScreen(splash_img, Qt.WindowStaysOnTopHint)
    splash.setMask(splash_img.mask())
    splash.show()
    app.processEvents()

    # This is needed to make the splash screen show up...
    splash.showMessage(" ")
    app.processEvents()
    # Run the lazy initializers (icons, widget controllers)
    icons.init()
    populate_controller_registry()

    # Init some singletons
    get_mediator()
    get_manager()

    # Init the panel wrangler singleton
    get_panel_wrangler().use_splash_screen(splash)

    # We might want to connect directly to the gui server
    if ns.host and ns.port:
        success = get_network().connectToServerDirectly(
            username=ns.username, hostname=ns.host, port=ns.port)
    else:
        # Connect to the GUI Server via dialog
        success = get_network().connectToServer()

    if success:
        get_db_conn().default_domain = ns.domain
        for uuid in ns.scene_uuid:
            db_scene = {'name': "Cinema",
                        'target_window': SceneTargetWindow.MainWindow,
                        'target': uuid}
            broadcast_event(KaraboEventSender.OpenSceneLink, db_scene)

        sys.exit(app.exec_())
    else:
        # If we are not successful in connection, we don't leave a remnant!
        app.quit()


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
    ap.add_argument('-username', '--username', type=str, default='admin',
                    help='The user name. Only used when specifiying host and '
                         'port. The default user name is `admin`')
    run_cinema(ap.parse_args())


if __name__ == '__main__':
    main()
