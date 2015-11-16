from asyncio import async, coroutine, Future, gather, sleep, TimeoutError
import threading
import os
import time
import datetime
import sys
import socket
from abc import ABCMeta, abstractmethod

from karabo.enums import AccessLevel, AccessMode, Assignment
from karabo.eventloop import EventLoop
from karabo.exceptions import KaraboError
from karabo.hash import (BinaryParser, Bool, Hash, HashType, Int32, UInt32,
                         Schema, SchemaHashType, String, Type, HashMergePolicy)
from karabo.launcher import getClassSchema_async, sameThread, legacy
from karabo.logger import Logger
from karabo.schema import Validator, Node
from karabo.signalslot import (SignalSlotable, Signal, ConnectionType, slot,
                               coslot, replySlot)
from karabo.timestamp import Timestamp


class Device(SignalSlotable):
    """This is the base class for all devices."""

    __version__ = "1.3"

    visibility = Int32(
        enum=AccessLevel, displayedName="Visibility",
        description="Configures who is allowed to see this device at all",
        assignment=Assignment.OPTIONAL, defaultValue=AccessLevel.OBSERVER,
        requiredAccessLevel=AccessLevel.EXPERT,
        accessMode=AccessMode.RECONFIGURABLE)

    compatibility = String(
        displayedName="Compatibility",
        description="The compatibility of this device to the Karabo framework",
        accessMode=AccessMode.RECONFIGURABLE, assignment=Assignment.OPTIONAL)

    state = String(
        displayedName="State",
        description="The current state the device is in",
        accessMode=AccessMode.READONLY, assignment=Assignment.OPTIONAL,
        defaultValue="uninitialized")

    archive = Bool(
        displayedName="Archive",
        description="Decides whether the properties of this device "
                    "will be logged or not",
        accessMode=AccessMode.RECONFIGURABLE, assignment=Assignment.OPTIONAL,
        defaultValue=True)

    _serverId_ = String(
        displayedName="_ServerID_",
        description="Do not set this property, it will be set by the "
                    "device-server",
        requiredAccessLevel=AccessLevel.EXPERT,
        assignment=Assignment.INTERNAL, accessMode=AccessMode.INITONLY,
        defaultValue="__none__")

    serverId = String(
        displayedName="ServerID",
        description="The device-server which this device is running on",
        requiredAccessLevel=AccessLevel.EXPERT,
        accessMode=AccessMode.READONLY)

    classId = String(
        displayedName="ClassID",
        description="The (factory)-name of the class of this device",
        requiredAccessLevel=AccessLevel.EXPERT,
        accessMode=AccessMode.READONLY)

    log = Node(Logger,
               description="Logging settings",
               displayedName="Logger",
               requiredAccessLevel=AccessLevel.EXPERT)

    signalChanged = Signal(HashType(), String())
    signalStateChanged = Signal(HashType(), String())
    signalSchemaUpdated = Signal(SchemaHashType(), String())

    subclasses = {}

    launch = classmethod(sameThread)

    def __init__(self, configuration):
        super(Device, self).__init__(configuration)
        if not hasattr(self, "serverId"):
            self.serverId = self._serverId_

        # host & domain names
        self.hostname, _, self.domainname = socket.gethostname().partition('.')

        # Setup the validation classes
        self.validatorIntern = Validator(injectDefaults=False)
        self.validatorExtern = Validator(injectDefaults=False)

        self.classId = type(self).__name__

    @classmethod
    def register(cls, name, dict):
        super(Device, cls).register(name, dict)
        if "abstract" not in dict:
            Device.subclasses[name] = cls

    def initInfo(self):
        info = self.info
        info["type"] = "device"
        info["classId"] = self.classId
        info["serverId"] = self.serverId
        info["visibility"] = self.visibility.value
        info["compatibility"] = self.__class__.__version__
        info["host"] = self.hostname
        info["status"] = "ok"
        info["archive"] = self.archive

    def initSchema(self):
        self.staticSchema = self.getClassSchema()
        self.fullSchema = Schema(self.classId)
        self.fullSchema.copy(self.staticSchema)

    def run(self):
        self.log.setBroker(self._ss)
        self.logger = self.log.logger
        self.initSchema()
        self.initInfo()
        super().run()

    @replySlot("slotChanged")
    def slotGetConfiguration(self):
        return self.configurationAsHash(), self.deviceId

    def configurationAsHash(self):
        r = Hash()
        for k in self._allattrs:
            a = getattr(self, k, None)
            if a is not None:
                r[k] = getattr(type(self), k).asHash(a)
        return r

    @coslot
    def slotReconfigure(self, reconfiguration):
        props = ((getattr(self.__class__, k), v)
                 for k, v in reconfiguration.items())
        try:
            setters = [t.checkedSet(self, v) for t, v in props]
            yield from gather(*setters)
        except KaraboError as e:
            self.logger.exception("Failed to set property")
            return False, str(e)
        self.signalChanged(self.configurationAsHash(), self.deviceId)
        return True, ""

    @slot
    def slotGetSchema(self, onlyCurrentState):
        if onlyCurrentState:
            currentState = self.state
            return self._getStateDependentSchema(currentState), self.deviceId
        else:
            return self.fullSchema, self.deviceId

    @slot
    def slotInstanceNew(self, instanceId, info):
        pass

    @slot
    def slotInstanceUpdated(self, instanceId, info):
        pass

    @slot
    def slotInstanceGone(self, instanceId, info):
        pass

    def _getStateDependentSchema(self, state):
        with self._stateDependentSchemaLock:
            if state in self._stateDependentSchema:
                return self._stateDependentSchema[state]
            self._stateDependentSchema[state] = self.getClassSchema(
                AssemblyRules(AccessType(READ | WRITE | INIT), state))
            if not self._injectedSchema.empty():
                self._stateDependentSchema[state] += self._injectedSchema
            return self._stateDependentSchema[state]


def launchPythonDevice():
    script, modname, classid, xmlfile = tuple(sys.argv)
    config = PythonDevice.loadConfiguration(xmlfile)

    try:
        device = PythonDevice.subclasses[classid](config)
        device.run()
    except Exception as e:
        print("Exception caught:", str(e))
        raise
    os._exit(77)
