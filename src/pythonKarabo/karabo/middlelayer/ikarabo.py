# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import atexit
import functools
import os
import re
import socket
import weakref
from asyncio import (
    DefaultEventLoopPolicy, set_event_loop, set_event_loop_policy)
from contextlib import suppress

import IPython

from karabo._version import version
from karabo.native import Node

from . import device_client
from .eventloop import NoEventLoop, set_global_sync
from .logger import build_logger_node
from .macro import EventThread, TopologyMacro
from .signalslot import slot


class NoEventLoopPolicy(DefaultEventLoopPolicy):

    def __init__(self, *args, instance=None, **kwargs):
        super().__init__(*args, **kwargs)
        self.instance = instance

    def get_event_loop(self):
        """Get the event loop.

        This return an instance of `NoEventLoop`.
        """
        loop = NoEventLoop(self.instance)
        return loop


class DeviceClient(TopologyMacro):
    # The command line device client will not wait
    # to come online for collecting topology
    wait_topology = False

    @property
    def __all__(self):
        return list(self.systemTopology["device"])

    def __getattr__(self, name):
        try:
            return super().__getattr__(name)
        except AttributeError:
            if name in self.systemTopology["device"]:
                return getDevice(name)
            else:
                raise AttributeError(f'Unknown device "{name}"')

    # We don't allow logs coming from command line
    log = Node(
        build_logger_node([]),
        displayedName="No Logger")

    def _initInfo(self):
        info = super()._initInfo()
        info["lang"] = "python"
        info["type"] = "client"
        return info

    async def slotKillDevice(self, message=None):
        """Subclass the `slotKillDevice` to exit the IPython console"""
        await super().slotKillDevice(message)
        ip = IPython.get_ipython()
        if ip is not None:
            ip.ask_exit()

    slotKillDevice = slot(slotKillDevice, passMessage=True)


@functools.wraps(device_client.connectDevice)
def connectDevice(device, *, autodisconnect=15, **kwargs):
    return device_client.connectDevice(device, autodisconnect=autodisconnect,
                                       **kwargs)


@functools.wraps(device_client.getDevice)
def getDevice(device, timeout=5):
    return device_client.getDevice(device, initialize=False, timeout=timeout)


def device_completer(self, line):
    return list(devices.systemTopology["device"])


def class_completer(self, line):
    param = first_param.search(line.line)
    if param:
        return devices.systemTopology["server"][param.group(0)[1:-1],
                                                "deviceClasses"]
    return list(devices.systemTopology["server"])


first_param = re.compile("\"[^\"]*\"|'[^']*'")


def start_device_client():
    global devices

    set_global_sync()
    thread = EventThread()
    thread.start()

    hostname = socket.gethostname().replace(".", "_")
    devices = DeviceClient(_deviceId_="ikarabo-{}-{}".format(
        hostname, os.getpid()))

    loop = NoEventLoop(devices)
    set_event_loop(loop)

    def shutdown_hook(device):
        nonlocal loop
        device = device()
        if device is not None:
            # Signal slotable has a timeout protection. We protect
            # against every exception as we are at the very end of lifetime
            with suppress(BaseException):
                loop.sync(device.slotKillDevice(), -1, True)
    weak = weakref.ref(devices)
    atexit.register(shutdown_hook, weak)

    return thread


def start_ikarabo():
    print("""IKarabo Version {}
    Type karabo? for help
    """.format(version))

    start_device_client()
    # Newer ipython versions run_until_complete, hence, we set
    # a policy for the eventloop
    set_event_loop_policy(NoEventLoopPolicy(instance=devices))

    ip = IPython.get_ipython()
    ip.set_hook("complete_command", device_completer,
                re_key=".*((get|connect)Device|execute(NoWait)?|"
                       "set(No)?Wait|shutdown(NoWait)?|(get|print)History|"
                       "getInitConfiguration|saveInitConfiguration|"
                       "listInitConfigurations|call(NoWait)?|"
                       "getConfiguration(FromPast)?|"
                       "getSchema(FromPast)?|instantiateDevice|"
                       "compareDeviceWithPast|compareDeviceConfiguration|"
                       "compareConfigurationsFromPast|getTimeInfo)")

    ip.set_hook("complete_command", class_completer,
                re_key=r".*instantiate(NoWait)?\(")
