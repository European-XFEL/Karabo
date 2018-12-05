import os
import os.path as op
import subprocess
import sys
from threading import Thread
from time import sleep
from unittest import TestCase

from karabo.bound import DeviceClient, EventLoop


def start_bound_api_server(server_id, args, plugin_dir=''):
    """Start a Bound API device server in its own process
    """
    # set the plugin directory to directory of this file
    # the static .egg-info file located in the test directory
    # assures the the pkg_resources plugin loader will indentify
    # the test device as a valid plugin with an entry point
    env = os.environ.copy()
    if plugin_dir:
        env['PYTHONPATH'] = plugin_dir

    entrypoint = "from karabo.bound_api.device_server import main;main()"
    cmd = [sys.executable, "-c", entrypoint, "serverId=" + server_id] + args
    return subprocess.Popen(cmd, env=env)


class BoundDeviceTestCase(TestCase):
    _timeout = 60  # seconds
    _waitTime = 2  # seconds
    _retries = _timeout//_waitTime

    def start_server(self, server_id, class_ids, plugin_dir=''):
        def classes_loaded():
            classes = self.dc.getClasses(server_id)
            return all(cls_id in classes for cls_id in class_ids)

        server_args = [
            'deviceClasses=' + ','.join(class_ids),
            'visibility=1', 'Logger.priority=ERROR'
        ]
        self.serverProcess = start_bound_api_server(server_id, server_args,
                                                    plugin_dir=plugin_dir)
        self.dc = DeviceClient()

        # wait for classes to appear
        nTries = 0
        while not classes_loaded():
            sleep(self._waitTime)
            if nTries > self._retries:
                if self.serverProcess is not None:
                    self.serverProcess.terminate()
                    self.serverProcess = None
                raise RuntimeError("Waiting for plugin to appear timed out")
            nTries += 1

    def setUp(self):
        # Start the EventLoop so that DeviceClient works properly
        self._eventLoopThread = Thread(target=EventLoop.work)
        self._eventLoopThread.daemon = True
        self._eventLoopThread.start()

        self.serverProcess = None
        self.dc = None

    def tearDown(self):
        # Stop the server
        if self.serverProcess is not None:
            self.serverProcess.terminate()
        # Stop the event loop
        EventLoop.stop()
        self._eventLoopThread.join()
