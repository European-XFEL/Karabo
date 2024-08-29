import argparse
import sys

import yaml
from qtpy.QtCore import Slot

import karabogui.const as global_constants
from karabo.common.scenemodel.api import SceneTargetWindow
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.messagebox import show_error
from karabogui.programs.base import create_gui_app, init_gui
from karabogui.programs.utils import close_app
from karabogui.singletons.api import get_db_conn, get_network, get_topology


def create_concert(data: dict):
    app = create_gui_app(sys.argv)
    init_gui(app, use_splash=True)

    def trigger_scenes():
        topology.system_tree.on_trait_change(
            trigger_scenes, "initialized", remove=True)
        domain = data["domain"]
        get_db_conn().default_domain = domain

        for scene in data["scenes"]:
            uuid = scene["uuid"]
            position = [int(scene["x"]), int(scene["y"])]
            db_scene = {"name": f"{domain}-{uuid}",
                        "target": uuid,
                        "position": position,
                        "target_window": SceneTargetWindow.Dialog
                        }
            broadcast_event(KaraboEvent.OpenSceneLink, db_scene)

    global_constants.APPLICATION_MODE = True
    topology = get_topology()
    # Attach to the topology
    topology.system_tree.on_trait_change(
        trigger_scenes, "initialized")
    network = get_network()

    @Slot()
    def _connect_handler(status):
        """Connect handler for direct connect attempts without dialog"""
        network.signalServerConnectionChanged.disconnect(_connect_handler)
        if not status:
            show_error("Error, could not connect to gui server "
                       f"<b>{data['host']}:{data['port']}</b>. Ending "
                       f"Concert.")
            topology.system_tree.on_trait_change(
                trigger_scenes, "initialized", remove=True)
            close_app()

    # We might want to connect directly to the gui server if all are provided
    if "host" in data and "port" in data:
        network.signalServerConnectionChanged.connect(_connect_handler)
        success = network.connectToServerDirectly(
            hostname=data["host"],
            port=data["port"])
    else:
        # Connect to the GUI Server via dialog"""
        success = network.connectToServer()
    return success, app


def run_concert(ns):
    """The concert is meant to load a yaml file for scene and network
    configuration

    Similar to the cinema, the scenes are spawned, but with positions.
    All scenes have the name ProjectDB|SceneName and are not editable!
    """
    with open(ns.yaml_file) as fh:
        data = yaml.safe_load(fh)
    success, app = create_concert(data)

    if success:
        app.exec()
        app.deleteLater()
        sys.exit()
    else:
        close_app()


help_message = """Path to yaml file. Example of the file content.

domain: CAS_INTERNAL
scenes:
  - uuid: '9d26d07b-d79e-4f71-99f1-469344da89ed'
    x: 1458
    y: 93
  - uuid: '9b73efec-4523-4a47-b88f-90edc2e9ed89'
    x: 2332
    y: 361
host: exflqr46957
port: 44444
"""


def main():
    arguments = argparse.ArgumentParser(
        description="Karabo Concert",
        formatter_class=argparse.RawTextHelpFormatter)
    arguments.add_argument("yaml_file", type=str, help=help_message)
    run_concert(arguments.parse_args())


if __name__ == "__main__":
    main()
