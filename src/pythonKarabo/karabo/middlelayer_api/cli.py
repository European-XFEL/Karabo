# this script is loaded at the beginning of a cli session
# it has side-effects while importing (like showing a banner)
# so don't use it for anything else

# when expanding this script, assure that afterwards it is still
# easily importable, which is important for the tests. No side effects
# should happend if this script is just imported as a module, so
# all side effects should be added to the if statement below.

from asyncio import set_event_loop
import functools
import os
import os.path as osp
import re
import socket
import sys

import IPython

import karabo
from karabo.common.states import State
from karabo.usermacros import (
    AcquiredFromLog, AcquiredOffline, AScan, AMesh, AMove, APathScan,
    Closable, Coolable, DScan, DMesh, DMove, GenericProxy, Movable,
    plot, Pumpable, Sensible, TScan)
from . import device_client
from .device_client import (
    DeviceClientBase, disconnectDevice, execute, executeNoWait, getClasses,
    getDevice, getDevices, getHistory, getServers, instantiate,
    instantiateNoWait, setWait, setNoWait, shutdown, shutdownNoWait,
    waitUntil, waitUntilNew)
from .eventloop import NoEventLoop
from .macro import EventThread, Macro
from .synchronization import sleep

# NOTE: This is the namespace for ikarabo
__all__ = ["connectDevice", "disconnectDevice", "execute",
           "executeNoWait", "getClasses", "getDevice", "getDevices",
           "getHistory", "getServers", "instantiate", "instantiateNoWait",
           "karabo", "setWait", "setNoWait", "shutdown",
           "shutdownNoWait", "sleep", "State", "waitUntil", "waitUntilNew",
           "AcquiredFromLog", "AcquiredOffline", "AScan", "AMesh", "AMove",
           "APathScan", "Closable", "Coolable", "DScan", "DMesh", "DMove",
           "GenericProxy", "Movable", "plot", "Pumpable", "Sensible",
           "TScan"]


class DeviceClient(Macro, DeviceClientBase):
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
                raise AttributeError('Unknown device "{}"'.format(name))


@functools.wraps(device_client.connectDevice)
def connectDevice(device, *, autodisconnect=15, **kwargs):
    return device_client.connectDevice(device, autodisconnect=autodisconnect,
                                       **kwargs)


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

    thread = EventThread()
    thread.start()

    hostname = socket.gethostname().replace(".", "_")
    devices = DeviceClient(_deviceId_="ikarabo-{}-{}".format(
        hostname, os.getpid()))

    set_event_loop(NoEventLoop(devices))

    return thread

# here starts the main part of this script. It does not use the
# standard if __name__ == "__main__" idiom, instead it checks whether
# this script is started from within ipython, in which case it installs
# everything needed into the ipython kernel.


ip = IPython.get_ipython()
if ip is not None:
    with open(osp.join(osp.dirname(sys.base_prefix), "VERSION"), "r") as fin:
        version = fin.read()

    print("""IKarabo Version {}
    Type karabo? for help
    """.format(version))

    start_device_client()

    ip.set_hook("complete_command", device_completer,
                re_key=".*((get|connect)Device|execute(NoWait)?|"
                       "set(No)?Wait|shutdown(NoWait)?|getHistory)\(")
    ip.set_hook("complete_command", class_completer,
                re_key=".*instantiate(NoWait)?\(")
