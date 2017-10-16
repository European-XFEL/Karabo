import os.path as op
from threading import Thread
from time import sleep
from unittest import TestCase

from karabo.bound import DeviceClient, EventLoop, Hash
from karabo.common.states import State
from karabo.integration_tests.utils import start_bound_api_server

SERVER_ID = "testServerRCG"
DEVICE_ID = "testRcg"


def _get_experts():
    experts = []
    s1 = Hash("source", "SASE1/SPB/SAMP/INJ_FLOW",
              "type", "control",
              "behavior", "read-only",
              "monitored", False)
    s1.setAttribute("source", "pipeline", False)
    experts.append(s1)

    s2 = Hash("source", "SASE1/SPB/SAMP/INJ_CAM_1",
              "type", "control",
              "behavior", "read-only",
              "monitored", False)
    s2.setAttribute("source", "pipeline", False)
    experts.append(s2)

    s3 = Hash("source", "SASE1/SPB/SAMP/INJ_CAM_1:ch1",
              "type", "control",
              "behavior", "init",
              "monitored", True)
    s3.setAttribute("source", "pipeline", True)
    experts.append(s3)
    return experts


def _get_users():
    users = []
    s1 = Hash("source", "SASE1/SPB/SAMP/INJ_TEMP_1",
              "type", "control",
              "behavior", "read-only",
              "monitored", False)
    s1.setAttribute("source", "pipeline", False)
    users.append(s1)

    s2 = Hash("source", "SASE1/SPB/SAMP/INJ_TEMP_2",
              "type", "control",
              "behavior", "read-only",
              "monitored", False)
    s2.setAttribute("source", "pipeline", False)
    users.append(s2)
    return users


class TestRunConfigurationGroup(TestCase):
    _timeout = 60  # seconds
    _waitTime = 2  # seconds
    _retries = _timeout//_waitTime

    def setUp(self):
        # Note where we are for later cleanip
        self._ownDir = str(op.dirname(op.abspath(__file__)))

        # Start the EventLoop so that DeviceClient works properly
        self._eventLoopThread = Thread(target=EventLoop.work)
        self._eventLoopThread.daemon = True
        self._eventLoopThread.start()

        server_args = ["deviceClasses=RunConfigurationGroup", "visibility=1",
                       "Logger.priority=ERROR"]
        self.serverProcess = start_bound_api_server(SERVER_ID, server_args,
                                                    plugin_dir=self._ownDir)
        self.dc = DeviceClient()

        # wait for plugin to appear
        nTries = 0
        while "RunConfigurationGroup" not in self.dc.getClasses(SERVER_ID):
            sleep(self._waitTime)
            if nTries > self._retries:
                raise RuntimeError("Waiting for plugin to appear timed out")
            nTries += 1

        config = Hash("Logger.priority", "ERROR",
                      "deviceId", DEVICE_ID,
                      "group", Hash("id", "Sample Environment",
                                    "description", "Bla Bla Bla",
                                    "expert", _get_experts(),
                                    "user", _get_users()))

        classConfig = Hash("classId", "RunConfigurationGroup",
                           "deviceId", DEVICE_ID,
                           "configuration", config)

        self.dc.instantiate(SERVER_ID, classConfig)

        # XXX: Bugs in the DeviceClient mean that immediately trying to read
        # our device's configuration could fail
        sleep(10)

        # wait for device to init
        state = None
        nTries = 0
        while state != State.NORMAL:
            try:
                state = self.dc.get(DEVICE_ID, "state")
            # A RuntimeError will be raised up to device init
            except RuntimeError:
                sleep(self._waitTime)
                if nTries > self._retries:
                    raise RuntimeError("Waiting for device to init timed out")
                nTries += 1

    def tearDown(self):
        # Stop the server
        self.serverProcess.terminate()
        # Stop the event loop
        EventLoop.stop()
        self._eventLoopThread.join(5)

    def test_rcg_sources(self):
        group = self.dc.get(DEVICE_ID, "group")

        assert group.get("id") == "Sample Environment"
        assert group.get("description") == "Bla Bla Bla"

        assert group.get("expert[0].source") == "SASE1/SPB/SAMP/INJ_FLOW"
        assert group.get("expert[0].type") == "control"
        assert group.get("expert[0].behavior") == "read-only"
        assert not group.get("expert[0].monitored")
        assert not group.getAttribute("expert[0].source", "pipeline")

        assert group.get("expert[1].source") == "SASE1/SPB/SAMP/INJ_CAM_1"
        assert group.get("expert[1].type") == "control"
        assert group.get("expert[1].behavior") == "read-only"
        assert not group.get("expert[1].monitored")
        assert not group.getAttribute("expert[1].source", "pipeline")

        assert group.get("expert[2].source") == "SASE1/SPB/SAMP/INJ_CAM_1:ch1"
        assert group.get("expert[2].type") == "control"
        assert group.get("expert[2].behavior") == "init"
        assert group.get("expert[2].monitored")
        assert group.getAttribute("expert[2].source", "pipeline")

        assert group.get("user[0].source") == "SASE1/SPB/SAMP/INJ_TEMP_1"
        assert group.get("user[0].type") == "control"
        assert group.get("user[0].behavior") == "read-only"
        assert not group.get("user[0].monitored")
        assert not group.getAttribute("user[0].source", "pipeline")

        assert group.get("user[1].source") == "SASE1/SPB/SAMP/INJ_TEMP_2"
        assert group.get("user[1].type") == "control"
        assert group.get("user[1].behavior") == "read-only"
        assert not group.get("user[1].monitored")
        assert not group.getAttribute("user[1].source", "pipeline")
