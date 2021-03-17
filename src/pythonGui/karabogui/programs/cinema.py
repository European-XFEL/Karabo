import argparse
import sys

from karabo.common.scenemodel.api import SceneTargetWindow
from karabogui.events import broadcast_event, KaraboEvent
from karabogui.programs.base import create_gui_app, init_gui
from karabogui.singletons.api import get_db_conn, get_network, get_topology


def run_cinema(ns):
    """The cinema is meant to directly download a scene from the project db

    From the initial scene the operator is allowed to request additional
    scene or device scene links.

    All scenes have the name ProjectDB|SceneName and are not editable!
    """
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

    topology = get_topology()
    # Attach to the topology
    topology.system_tree.on_trait_change(
        trigger_scenes, 'initialized')

    # We might want to connect directly to the gui server
    if ns.host and ns.port:
        success = get_network().connectToServerDirectly(
            username=ns.username, hostname=ns.host, port=ns.port)
    else:
        # Connect to the GUI Server via dialog
        success = get_network().connectToServer()

    if success:
        app.exec_()
        app.deleteLater()
        sys.exit()
    else:
        # If we are not successful in connection, we don't leave a remnant!
        topology.system_tree.on_trait_change(
            trigger_scenes, 'initialized', remove=True)
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
                    help='The user name. Only used when specifying host and '
                         'port. The default user name is `admin`')
    ap.add_argument('-nosplash', '--nosplash', action='store_true')
    run_cinema(ap.parse_args())


if __name__ == '__main__':
    main()
