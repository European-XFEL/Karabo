from collections import namedtuple
from pathlib import Path

from qtpy.QtCore import QObject, Signal

from karabo.common.scenemodel.api import SceneTargetWindow
from karabogui.events import KaraboEvent
from karabogui.programs.concert import create_concert, run_concert
from karabogui.testing import singletons, system_hash
from karabogui.topology.api import SystemTopology

namespace = namedtuple("namespace", "yaml_file")

DATA = {
    "domain": "CAS_INTERNAL",
    "scenes": [{"uuid": "9d26d07b-d79e-4f71-99f1-469344da89ed",
                "x": 1458, "y": 93},
               {"uuid": "9b73efec-4523-4a47-b88f-90edc2e9ed89",
                "x": 2332, "y": 361}],
    "host": "exflqr46957", "port": 44444}


def test_run_concert(gui_app, mocker):
    file_path = Path(__file__).parent / "concert.yml"
    args = namespace(file_path)

    mock_concert = mocker.patch("karabogui.programs.concert.create_concert",
                                return_value=(False, "dummyApp"))
    run_concert(args)
    assert mock_concert.call_count == 1
    mock_concert.assert_called_with(DATA)


class NetworkMock(QObject):
    """A basic mock to avoid using the singleton class"""
    signalServerConnectionChanged = Signal(bool)
    signalReceivedData = Signal(object)
    signalNetworkPerformance = Signal(float, bool)

    def __init__(self, parent=None):
        super().__init__(parent)


def test_concert(gui_app, mocker):
    network = NetworkMock()
    network.connectToServerDirectly = mocker.Mock()
    network.connectToServerDirectly.return_value = True
    network.onSubscribeLogs = mocker.Mock()
    topology = SystemTopology()

    path = "karabogui.programs.concert.broadcast_event"
    concert_app = mocker.patch("karabogui.programs.concert.create_gui_app")
    concert_app.return_value = gui_app
    assert gui_app.organizationName() == "TestFacility"
    # Patch the gui_app to avoid collisions between tests
    with singletons(network=network, topology=topology):
        broadcast = mocker.patch(path)
        success, app = create_concert(DATA)
        concert_app.assert_called_once()
        assert success

        network.signalServerConnectionChanged.emit(True)
        network.onSubscribeLogs.assert_not_called()

        # Initialize the trees
        topology.initialize(system_hash())
        expected_args1 = {
            "name": "CAS_INTERNAL-9d26d07b-d79e-4f71-99f1-469344da89ed",
            "target": "9d26d07b-d79e-4f71-99f1-469344da89ed",
            "position": [1458, 93],
            "target_window": SceneTargetWindow.Dialog}
        expected_args2 = {
            "name": "CAS_INTERNAL-9b73efec-4523-4a47-b88f-90edc2e9ed89",
            "target": "9b73efec-4523-4a47-b88f-90edc2e9ed89",
            "position": [2332, 361],
            "target_window": SceneTargetWindow.Dialog}

        assert broadcast.call_count == 2
        args1, args2 = broadcast.call_args_list
        assert args1[0] == (KaraboEvent.OpenSceneLink, expected_args1)
        assert args2[0] == (KaraboEvent.OpenSceneLink, expected_args2)
