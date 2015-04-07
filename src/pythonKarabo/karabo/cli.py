# this script is loaded at the beginning of a cli session

import karabo

from asyncio import set_event_loop

import IPython

from karabo.device_client import (
    getDevice, waitUntil, waitUntilNew, setNoWait, executeNoWait,
    DeviceClientBase)
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


def completer(self, line):
    return list(devices.systemTopology["device"])

devices = DeviceClient()
set_event_loop(NoEventLoop(devices))

ip = IPython.get_ipython()
ip.set_hook("complete_command", completer, re_key=".*getDevice")


__all__ = ["getDevice", "waitUntil", "waitUntilNew", "setNoWait",
           "executeNoWait"]
