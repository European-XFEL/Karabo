# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
                     logLevel='FATAL', namespace=None,
                     skip_plugins_check=False, **kw_server_args):
        """
        Start a server in its own process.
        api: "bound", "cpp" or "mdl"
        class_ids: list of required device classes for this server
        plugin_dir: where extra plugins are looked for (default: empty)
        logLevel: log level of server (default: 'FATAL', i.e. no logging)
        namespace: if not None, it will set the `pluginNamespace` option
                   in the `bound` and `mdl` apis.
        skip_plugins_check: if True, the step that checks that plugins have
                            been loaded is skipped.

        Further arguments to the server can be given via kw_server_args and
        will be passed as "<key>=str(<value>)" to the server command line.
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
        for k, v in kw_server_args.items():
            server_args.append(k + "=" + str(v))

        if api == "bound":
            if namespace is not None:
                server_args = [f"pluginNamespace={namespace}"] + server_args
            server_args.append('Logger.priority={}'.format(logLevel))
            serverProcess = start_bound_server(server_id, server_args,
                                               plugin_dir=plugin_dir)
        elif api == "cpp":
            server_args.append('Logger.priority={}'.format(logLevel))
            serverProcess = start_cpp_server(server_id, server_args,
                                             plugin_dir=plugin_dir)
        elif api == "mdl":
            if namespace is not None:
                server_args = [f"pluginNamespace={namespace}"] + server_args
            server_args.append(f'log.level={logLevel}')
            serverProcess = start_mdl_server(server_id, server_args,
                                             plugin_dir=plugin_dir)
        else:
            raise RuntimeError("Unknown server api: {}".format(api))

        # wait for classes to appear
        if not skip_plugins_check:
            nTries = 0
            while not classes_loaded():
                sleep(self._waitTime)
                if nTries > self._retries:
                    if serverProcess is not None:
                        serverProcess.terminate()
                        serverProcess = None
                    raise RuntimeError(
                        "Waiting for plugin to appear timed out")
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

        # Get rid of client
        self.dc = None

        # Stop the event loop
        EventLoop.stop()
        # Simple join() hung e.g. in
        # https://git.xfel.eu/Karabo/Framework/-/jobs/264237
        self._eventLoopThread.join(timeout=10)
        if self._eventLoopThread.is_alive():
            print("Joining event loop thread failed, event loop keeps "
                  f"{EventLoop.getNumberOfThreads()} threads.")

    def waitUntilTrue(self, condition, maxTimeoutSec, maxTries=20):
        """
        Wait until condition() gets True.
        :param condition method to become True
        :param maxTimeoutSec maximum time in seconds to wait for it to get True
        :maxTries up to how many times condition() is evaluated within
                   maxTimeoutSec

        :return whether condition() became True
        """
        interval = maxTimeoutSec / maxTries
        counter = maxTries
        while counter > 0:
            print("waitUntilTrue", counter)
            if condition():
                return True
            else:
                counter -= 1
                sleep(interval)
        # Give up:
        return False
