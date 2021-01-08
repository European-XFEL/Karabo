from asyncio import set_event_loop
import atexit
import functools
import os
import re
import socket

import IPython

from . import device_client
from .device_client import DeviceClientBase, getDevice
from .eventloop import NoEventLoop
from .macro import EventThread, Macro
from karabo._version import version


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

    def _initInfo(self):
        info = super(DeviceClient, self)._initInfo()
        info["lang"] = "python"
        info["type"] = "client"
        return info


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

    def shutdown_hook():
        global devices
        del devices

    atexit.register(shutdown_hook)

    return thread


def start_ikarabo():
    print("""IKarabo Version {}
    Type karabo? for help
    """.format(version))

    start_device_client()

    ip = IPython.get_ipython()
    ip.set_hook("complete_command", device_completer,
                re_key=".*((get|connect)Device|execute(NoWait)?|"
                       "set(No)?Wait|shutdown(NoWait)?|getHistory|"
                       "(getConfiguration|saveConfiguration)FromName|"
                       "listConfigurationFromName|call(NoWait)?|"
                       "getConfiguration(FromPast)?|"
                       "getLastConfiguration|getSchema(FromPast)?|"
                       "compareDeviceWithPast|compareDeviceConfiguration)")
    ip.set_hook("complete_command", class_completer,
                re_key=r".*instantiate(NoWait)?\(")
