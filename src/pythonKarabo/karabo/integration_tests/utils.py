import os
import subprocess
import sys
from threading import Thread
from time import sleep
from unittest import TestCase

from karabo.bound import DeviceClient, EventLoop


def start_bound_server(server_id, args, plugin_dir=''):
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

def start_cpp_server(server_id, args, plugin_dir=''):
    """
    Start a C++ device server with given args in its own process.
    Ignore plugin_dir so far
    """
    env = os.environ.copy()  # needed?
    cmd = ["karabo-cppserver", "serverId=" + server_id] + args
    return subprocess.Popen(cmd, env=env)

def start_mdl_server(server_id, args, plugin_dir=''):
    """Start a Middlelayer API device server in its own process
    """
    # set the plugin directory to directory of this file
    # the static .egg-info file located in the test directory
    # assures the the pkg_resources plugin loader will indentify
    # the test device as a valid plugin with an entry point
    env = os.environ.copy()  # needed?
    if plugin_dir:
        env['PYTHONPATH'] = plugin_dir

    # Do we need to something like for bound to get the defined plugins_dir?
    cmd = ["karabo-middlelayerserver", "serverId=" + server_id] + args
    return subprocess.Popen(cmd, env=env)

class BoundDeviceTestCase(TestCase):
    _timeout = 60  # seconds
    _waitTime = 2  # seconds
    _retries = _timeout//_waitTime

    serverProcesses = {}
    dc = None

    def start_server(self, api, server_id, class_ids, plugin_dir='',
                     logLevel='FATAL'):
        """
        Start a server in its own process.
        api: "bound", "cpp" or "mdl"
        class_ids: list of required device classes for this server
        plugin_dir: where extra plugins are looked for (default: empty)
        logLevel: log level of server (default: 'FATAL', i.e. no logging)
        """

        if server_id in self.serverProcesses:
            raise RuntimeError("Server with id {} already started"
                               .format(server_id))

        def classes_loaded():
            classes = self.dc.getClasses(server_id)
            return all(cls_id in classes for cls_id in class_ids)

        server_args = [
            'deviceClasses=' + ','.join(class_ids),
            'visibility=1'
        ]
        if api == "bound":
            server_args.append('Logger.priority={}'.format(logLevel))
            serverProcess = start_bound_server(server_id, server_args,
                                               plugin_dir=plugin_dir)
        elif api == "cpp":
            server_args.append('Logger.priority={}'.format(logLevel))
            serverProcess = start_cpp_server(server_id, server_args,
                                             plugin_dir=plugin_dir)
        elif api == "mdl":
            # How to specify logLevel? Next line gives
            # AttributeError: 'Node' object has no attribute 'level'
            # server_args.append('log.level={}'.format(logLevel))
            serverProcess = start_mdl_server(server_id, server_args,
                                             plugin_dir=plugin_dir)
        else:
            raise RuntimeError("Unknown server api: {}".format(api))

        # wait for classes to appear
        nTries = 0
        while not classes_loaded():
            sleep(self._waitTime)
            if nTries > self._retries:
                if serverProcess is not None:
                    serverProcess.terminate()
                    serverProcess = None
                raise RuntimeError("Waiting for plugin to appear timed out")
            nTries += 1

        self.serverProcesses[server_id] = serverProcess

    def setUp(self):
        # Start the EventLoop so that DeviceClient works properly
        self._eventLoopThread = Thread(target=EventLoop.work)
        self._eventLoopThread.daemon = True
        self._eventLoopThread.start()

        self.dc = DeviceClient()

    def tearDown(self):
        # Stop the servers
        for serverProcess in self.serverProcesses.values():
            serverProcess.terminate()
        self.serverProcesses.clear()

        # Stop the event loop
        EventLoop.stop()
        self._eventLoopThread.join()

        self.dc = None

