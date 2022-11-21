import atexit
import functools
import os
import re
import socket
import weakref
from asyncio import set_event_loop

import IPython

from karabo._version import version
from karabo.native import Node

from . import device_client
from .device_client import DeviceClientBase
from .eventloop import NoEventLoop
from .logger import build_logger_node
from .macro import EventThread, Macro
from .signalslot import coslot


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

    # We don't allow logs coming from command line
    log = Node(
        build_logger_node([]),
        displayedName="No Logger")

    def _initInfo(self):
        info = super(DeviceClient, self)._initInfo()
        info["lang"] = "python"
        info["type"] = "client"
        return info

    async def slotKillDevice(self, message=None):
        """Subclass the `slotKillDevice` to exit the IPython console"""
        await super().slotKillDevice(message)
        ip = IPython.get_ipython()
        if ip is not None:
            ip.ask_exit()

    slotKillDevice = coslot(slotKillDevice, passMessage=True)


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
            loop.create_task(device.slotKillDevice())

    weak = weakref.ref(devices)
    atexit.register(shutdown_hook, weak)

    return thread


def start_ikarabo():
    print("""IKarabo Version {}
    Type karabo? for help
    """.format(version))

    start_device_client()

    ip = IPython.get_ipython()
    ip.set_hook("complete_command", device_completer,
                re_key=".*((get|connect)Device|execute(NoWait)?|"
                       "set(No)?Wait|shutdown(NoWait)?|(get|print)History|"
                       "(getConfiguration|saveConfiguration)FromName|"
                       "listConfigurationFromName|call(NoWait)?|"
                       "getConfiguration(FromPast)?|"
                       "getLastConfiguration|getSchema(FromPast)?|"
                       "compareDeviceWithPast|compareDeviceConfiguration|"
                       "compareConfigurationsFromPast|getTimeInfo)")

    ip.set_hook("complete_command", class_completer,
                re_key=r".*instantiate(NoWait)?\(")
