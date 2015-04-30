# this script is loaded at the beginning of a cli session

import karabo

from asyncio import set_event_loop
import re

import IPython

from karabo.device_client import (
    getDevice, waitUntil, waitUntilNew, setWait, setNoWait, execute,
    executeNoWait, DeviceClientBase, getDevices, getClasses, getServers,
    instantiate, connectDevice, shutdown, shutdownNoWait, instantiateNoWait)
from karabo.eventloop import NoEventLoop
from karabo.macro import Macro


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


devices = DeviceClient()
set_event_loop(NoEventLoop(devices))

ip = IPython.get_ipython()


def device_completer(self, line):
    #print("device")
    return list(devices.systemTopology["device"])


def class_completer(self, line):
    param = first_param.search(line.line)
    if param:
        return devices.systemTopology["server"][param.group(0)[1:-1],
                                                "deviceClasses"]
    return list(devices.systemTopology["server"])
first_param = re.compile("\"[^\"]*\"|'[^']*'")


ip.set_hook("complete_command", device_completer,
            re_key=".*((get|connect)Device|execute(NoWait)?|"
                   "set(No)?Wait|shutdown(NoWait)?)\(")
ip.set_hook("complete_command", class_completer,
            re_key=".*instantiate(NoWait)?\(")


__all__ = ["getDevice", "waitUntil", "waitUntilNew", "setWait", "setNoWait",
           "execute", "executeNoWait", "getDevices", "getClasses",
           "getServers", "instantiate", "connectDevice", "shutdown",
           "shutdownNoWait", "instantiateNoWait"]
