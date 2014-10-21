""" Everything you need to do asynchronous calls. This is primarily
for macros.

.. autoclass:: Macro

.. autoclass:: ClientBase
   :members:

.. autoclass:: DeviceProxy
   :members:
"""

import manager
from network import network
from schema import Schema, Type, Object, Box

from karabo.hashtypes import Slot, String

from asyncio import AbstractEventLoop, Handle, Transport, Future, coroutine
from collections import OrderedDict
from quamash import QEventLoop
from PyQt4.QtGui import QApplication, QMessageBox
from PyQt4.QtCore import QTimer, pyqtSlot
import sys


class FutureSlot:
    def __init__(self, signal, *, loop=None):
        self.loop = loop
        self.signal = signal


    def __enter__(self):
        self.signal.connect(self.slot)
        self.args = [ ]
        self.future = None
        return self


    def __exit__(self, a, b, c):
        self.signal.disconnect(self.slot)


    @coroutine
    def __iter__(self):
        if self.args:
            return self.args.pop(0)
        self.future = Future(loop=self.loop)
        return (yield from self.future)


    def slot(self, *args):
        self.args.append(args)
        if self.future is not None:
            self.future.set_result(self.args.pop(0))
            self.future = None


class DeviceProxy(object):
    """This is a proxy to a real device.

    All calls to this device are forwarded to the actual device.
    They are typically created by calling :meth:`~ClientBase.getDevice`.
    Examples of usage:

    ::

        device = yield from self.getDevice("someDevice")
        device.speed = 5 # set a property
        device.start() # call a slot

    """
    def __init__(self, device):
        self.__dict__["_device"] = device
        device.__enter__()


    def __dir__(self):
        return list(self._device.__box__.descriptor.dict.keys())


    def __getattr__(self, attr):
        return getattr(self._device, attr)


    def __setattr__(self, attr, value):
        setattr(self._device, attr, value)


    def __del__(self):
        self._device.__exit__(None, None, None)


class MetaMacro(type(Object)):
    @staticmethod
    def __prepare__(name, bases):
        return OrderedDict()


    def __init__(self, name, bases, dict):
        super().__init__(name, bases, dict)
        d = OrderedDict()
        for k, v in dict.items():
            if isinstance(v, (Type, Slot)):
                d[k] = v
                if v.displayedName is None:
                    v.displayedName = k
                v.key = k

        self.__properties__ = d


class ClientBase(object):
    @coroutine
    def getDevice(self, deviceId):
        """get a proxy object for a device

        This coroutine searches for the device and waits until its
        schema and configuration have arrived. It then returns a
        :class:`DeviceProxy` for this device."""
        device = manager.getDevice(deviceId)
        if device.status == "alive":
            return DeviceProxy(device.value)
        else:
            with FutureSlot(device.statusChanged) as fs:
                device.addVisible()
                try:
                    while device.status != "alive":
                        yield from fs
                    return DeviceProxy(device.value)
                finally:
                    device.removeVisible()


    @coroutine
    def getClass(self, serverId, classId):
        """get a class from a server

        return the class *classId* from server *serverId*.
        The result is an object that knows about parameters of this
        device, so that you can modify them.

        ::

            cls = yield from self.getClass("someServer", "someClass")
            cls.someParameter = 7
            self.startDevice("someDeviceName", cls)
        """
        cls = manager.getClass(serverId, classId)
        with FutureSlot(cls.statusChanged) as fs:
            while cls.status != "schema":
                yield from fs
        return cls.value


    @coroutine
    def startDevice(self, deviceId, cls):
        """start a device

        Start the device *deviceId* with a *cls* retrieved with :meth:`getClass`.
        """
        serverId, classId = cls.__box__.id.split(".")
        network.onInitDevice(serverId, classId, deviceId, cls.__box__.toHash())
        return (yield from self.getDevice(deviceId))


class DeviceClient(ClientBase):
    pass


class Macro(ClientBase, Object, metaclass=MetaMacro):
    """This is the parent class for all macros.

    It assures that the macro is properly listed in the GUI.
    Most of its functionality is found in its base class, :class:`ClientBase`"""
    instance = None
    type = "macro"


    def __init__(self, box):
        Object.__init__(self, box)
        self.__dict__['state'] = Box(box.path + ("state",), String(),
                                     box.configuration)
        self.exclusive = None


    @classmethod
    def getSchema(cls):
        schema = Schema(cls.__name__)
        schema.dict = cls.__properties__
        schema.cls = cls
        return schema


class EventLoop(QEventLoop):
    def __init__(self, argv):
        self.app = QApplication(argv)
        QEventLoop.__init__(self, self.app)
        self.running = False


    @coroutine
    def getValue(self, device, attr):
        box = getattr(device.__box__.boxvalue, attr)
        with FutureSlot(box.signalUpdateComponent, loop=self) as fs:
            return (yield from fs)


    @coroutine
    def waitForChanges(self):
        with FutureSlot(network.tcpSocket.readyRead) as fs:
            yield from fs
            while network.isDataPending():
                yield from fs
