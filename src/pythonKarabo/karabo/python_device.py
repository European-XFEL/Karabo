from asyncio import async, coroutine, Future, gather, sleep, TimeoutError
import threading
import os
import time
import datetime
import sys
import socket
from abc import ABCMeta, abstractmethod
from karabo.async import NewValueTimestamp
from karabo.decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from karabo.hash import BinaryParser, Hash, HashMergePolicy
from karabo.hashtypes import Bool, Int32, UInt32, String, Type
from karabo.logger import Logger
from karabo.schema import Configurable, Schema, Validator, Node
from karabo.signalslot import (SignalSlotable, Signal, ConnectionType, slot,
                               coslot, replySlot)
from karabo.timestamp import Timestamp
from karabo import hashtypes, KaraboError
from karabo.enums import AccessLevel, AccessMode, Assignment
from karabo.registry import Registry
from karabo.eventloop import EventLoop
from karabo.launcher import getClassSchema_async, sameThread, legacy


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("Device", "1.0")
class Device(SignalSlotable):
    """This is the base class for all devices."""
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

    signalChanged = Signal(hashtypes.Hash(), hashtypes.String())
    signalSchemaUpdated = Signal(hashtypes.Schema(), hashtypes.String())

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
            setters = [t.checkedSet(self, v) for t, v in props
                       if isinstance(t, Type)]
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

    def _getStateDependentSchema(self, state):
        with self._stateDependentSchemaLock:
            if state in self._stateDependentSchema:
                return self._stateDependentSchema[state]
            self._stateDependentSchema[state] = self.getClassSchema(
                AssemblyRules(AccessType(READ | WRITE | INIT), state))
            if not self._injectedSchema.empty():
                self._stateDependentSchema[state] += self._injectedSchema
            return self._stateDependentSchema[state]


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("PythonDevice", "1.0")
class PythonDevice(Device):
    abstract = True

    instanceCountPerDeviceServer = dict()

    progress = Int32(
        displayedName="Progress",
        description="The progress of the current action",
        accessMode=AccessMode.READONLY, defaultValue=0)

    launch = classmethod(legacy)

    def __init__(self, configuration):
        if configuration is None:
            raise ValueError("Configuration must be Hash object, not None")
        super(PythonDevice, self).__init__(configuration)
        self.parameters = configuration
        self.parameters["serverId"] = self.serverId
        self.parameters["deviceId"] = self.deviceId

        # Initialize threading locks...
        self._stateChangeLock = threading.Lock()
        self._stateDependentSchemaLock = threading.Lock()
        self._stateDependentSchema = {}
        self._injectedSchema = Schema(self.deviceId)

        # Initialize _client to None (important!)
        self._client = None

       # Initialize Device slots
        self._initDeviceSlots()

    @property
    def signalSlotable(self):
        '''Get SignalSlotable object embeded in PythonDevice instance.'''
        return self._ss

    def run(self):
        super().run()
        validated = self.validatorIntern.validate(self.fullSchema,
                                                  self.parameters)
        self.parameters.merge(validated, HashMergePolicy.REPLACE_ATTRIBUTES)

    def remote(self):
        if self._client is None:
            self._client = OldDeviceClient(self)
        return self._client

    def set(self, *args):
        """
        Updates the state of the device. This function automatically notifies
        any observers. This function supports 3 args: key, value, timestamp or
        2 arg: hash, timestamp. If 1 or more than 3 arguments, it does nothing
        """
        pars = tuple(args)
        with self._stateChangeLock:
            if len(pars) == 0 or len(pars) > 3:
                raise SyntaxError("Number of parameters is wrong: "
                                  "from 1 to 3 arguments are allowed.")

            # key, value, timestamp args
            if len(pars) == 3:
                key, value, stamp = pars
                if type(stamp) is not Timestamp:
                    raise TypeError("The 3rd argument should be Timestamp")
                pars = tuple([Hash(key, value), stamp])

            # hash args
            if len(pars) == 1:
                h = pars[0]
                if type(h) is not Hash:
                    raise TypeError("The only argument should be a Hash")
                pars = tuple([h, Timestamp()])   # add timestamp

            # key, value or hash, timestamp args
            if len(pars) == 2:
                if type(pars[0]) is not Hash:
                    key, value = pars
                    pars = tuple([Hash(key, value), Timestamp()])
                hash, stamp = pars

                try:
                    validated = self.validatorIntern.validate(self.fullSchema,
                                                              hash, stamp)
                except RuntimeError as e:
                    print("Validation Exception (Intern):", e)
                    raise RuntimeError("Validation Exception: " + str(e))

                if not validated.empty():
                    self.parameters.merge(validated,
                                          HashMergePolicy.REPLACE_ATTRIBUTES)
                    self.signalChanged(validated, self.deviceId)

    def __setitem__(self, key, value):
        self.set(key, value, Timestamp())

    def get(self, key):
        with self._stateChangeLock:
            try:
                return self.parameters[key]
            except RuntimeError as e:
                raise AttributeError(
                    "Error while retrieving '{}' from device".format(key))

    def __getitem__(self, key):
        return self.get(key)

    def getFullSchema(self):
        return self.fullSchema

    def updateSchema(self, schema):
        validator = Validator(injectDefaults=True)
        validated = validator.validate(schema, Hash())
        with self._stateChangeLock:
            for path in self._injectedSchema.hash.paths():
                if (path in self.parameters and
                        not path in self.staticSchema.has(path)):
                    del self.parameters[path]
            self._stateDependentSchema = {}
            self._injectedSchema.copy(schema)
            self.fullSchema.copy(self.staticSchema)
            self.fullSchema.hash.merge(self._injectedSchema.hash)
        # notify the distributed system...
        self.signalSchemaUpdated(self.fullSchema, self.deviceId)
        self.set(validated)
        self.log.INFO("Schema updated")

    def appendSchema(self, schema):
        rules = ValidatorValidationRules()
        rules.allowAdditionalKeys = True
        rules.allowMissingKeys = True
        rules.allowUnrootedConfiguration = True
        rules.injectDefaults = True
        rules.injectTimestamps = True
        validator = Validator()
        validator.setValidationRules(rules)
        validated = validator.validate(schema, self.parameters)
        with self._stateChangeLock:
            for key in self._injectedSchema.getKeys():
                self.parameters.erase(key)
            self._stateDependentSchema = {}
            self._injectedSchema += schema
            self.fullSchema.copy(self.staticSchema)
            self.fullSchema += self._injectedSchema
            self.parameters.merge(validated,
                                  HashMergePolicy.REPLACE_ATTRIBUTES)
            self.fullSchema.updateAliasMap()
        # notify the distributed system...
        self.signalSchemaUpdated(self.fullSchema, self.deviceId)
        self.log.INFO("Schema appended")

    getClassSchema_async = classmethod(getClassSchema_async)

    def setProgress(self, value, associatedText=""):
        v = self.progressMin + value / (self.progressMax - self.progressMin)
        self.progress = v

    def resetProgress(self):
        self.progress = self.progressMin

    def setProgressRange(self, minimum, maximum):
        self.progressMin, self.progressMax = minimum, maximum

    def getAliasFromKey(self, key, aliasReferenceType):
        try:
            return self.fullSchema.getAliasFromKey(key, aliasReferenceType)
        except RuntimeError as e:
            raise AttributeError(
                "Error while retrieving alias from parameter ({}): {}".
                format(key, e))

    def getKeyFromAlias(self, alias):
        try:
            return self.fullSchema.getKeyFromAlias(alias)
        except RuntimeError as e:
            raise AttributeError(
                "Error while retrieving parameter from alias ({}): {}".
                format(alias, e))

    def aliasHasKey(self, alias):
        return self.fullSchema.aliasHasKey(key)

    def keyHasAlias(self, key):
        return self.fullSchema.keyHasAlias(key)

    def getValueType(self, key):
        return self.fullSchema.getValueType(key)

    def getCurrentConfiguration(self, tag=None):
        if tag is None:
            return self.parameters

        s = self.fullSchema.hash
        ret = Hash()

        for k, v in self.parameters.items():
            a = s[k, ...]
            if tag in a.get("tags", ""):
                ret[k] = v
        return ret

    def getServerId(self):
        return self.serverId

    def errorFoundAction(self, shortMessage, detailedMessage):
        self.log.ERROR("Error Found Action: {} -- {}".format(
                       shortMessage, detailedMessage))
        self.signalNotification("ERROR", shortMessage, detailedMessage,
                                self.deviceId)

    def preReconfigure(self, incomingReconfiguration):
        pass

    def postReconfigure(self):
        pass

    def updateState(self, currentState):
        self.log.DEBUG("onStateUpdate: {}".format(currentState))
        if self.state != currentState:
            self.state = currentState
        return currentState  # reply new state to interested event initiators

    def onStateUpdate(self, currentState):
        print("onStateUpdate() is deprecated, use updateState() instead")
        self.updateState(currentState)

    def exceptionFound(self, shortMessage, detailedMessage):
        self.log.ERROR(shortMessage)
        self._ss.emit("signalNotification", "EXCEPTION", shortMessage,
                      detailedMessage)

    def noStateTransition(self):
        self.signalNoTransition("No state transition possible", self.deviceId)

    signalNoTransition = Signal(hashtypes.String(), hashtypes.String())
    signalProgressUpdated = Signal(hashtypes.Int32(), hashtypes.String(),
                                   hashtypes.String())
    signalNotification = Signal(hashtypes.String(), hashtypes.String(),
                                hashtypes.String(), hashtypes.String())

    def _initDeviceSlots(self):
        self.signalChanged.connect("*", "slotChanged", ConnectionType.NO_TRACK)
        self.signalNoTransition.connect("*", "slotNoTransition",
                                        ConnectionType.NO_TRACK)
        self.signalNotification.connect("*", "slotNotification",
                                        ConnectionType.NO_TRACK)
        self.signalSchemaUpdated.connect("*", "slotSchemaUpdated",
                                         ConnectionType.NO_TRACK)

    def triggerError(self, s, d):
        print(
            "The triggerError() function is deprecated, use execute() instead")
        self.exceptionFound(s, d)

    def execute(self, command, *args):
        async(getattr(self, command)(*args), loop=self._ss.loop)

    @slot
    def slotGetConfiguration(self):
        r = Hash(self.parameters)
        r.merge(super().slotGetConfiguration()[0])
        return r, self.deviceId

    @slot
    def slotReconfigure(self, reconfiguration):
        try:
            self.preReconfigure(reconfiguration)
            todel = []
            for k, v in reconfiguration.items():
                t = getattr(type(self), k, None)
                if isinstance(t, Type):
                    setattr(self, k, t.cast(v))
                    todel.append(k)
            for k in todel:
                del reconfiguration[k]
            with self._stateChangeLock:
                self.parameters.merge(reconfiguration)
            self.signalChanged(reconfiguration, self.deviceId)
            self.postReconfigure()
            return True, ""
        except Exception as e:
            return False, str(e)

    @slot
    def slotPingAnswer(self, instanceId, info):
        if self._client is None:
            return
        self._client.addInstance(instanceId, info)
        self._ss.connect(instanceId, "signalHeartbeat", self.slotHeartbeat)
    slotInstanceNew = slotPingAnswer

    @slot
    def slotHeartbeat(self, instanceId, interval, info):
        if self._client is not None:
            self._client.addInstance(instanceId, info, interval)

    @slot
    def slotInstanceGone(self, instanceId, info):
        if self._client is not None:
            self._client.removeInstance(instanceId)

    @staticmethod
    def loadConfiguration(xmlfile):
        r = BinaryParser()
        with open(xmlfile, 'r') as file:
            return r.read(file.read())
        os.remove(xmlfile)


class Macro(PythonDevice):
    abstract = True

    @coroutine
    def runit(self):
        yield from getattr(self, method)()

    @classmethod
    def main(cls):
        h = Hash(*chain.from_iterable(s.split("=") for s in sys.argv[2:]))
        v = Validator()
        o = cls(v.validate(cls.getClassSchema(), h))
        loop = get_event_loop()
        loop.run_until_complete(o.runit(sys.argv[1]))


class OldDeviceClient(PythonDevice):
    abstract = True

    def __init__(self, parent):
        self.parent = parent
        self.parent._ss.emit("call", {"*": ["slotPing"]},
                             self.parent.deviceId, False, False)
        self.devices = {}
        async(self.run(), loop=self.parent._ss.loop)

    def sync(self, coro, timeout=-1):
        lock = threading.Lock()
        lock.acquire()
        future = async(coro, loop=self.parent._ss.loop)
        future.add_done_callback(lambda x: lock.release())
        lock.acquire(timeout=timeout)
        if future.done():
            return future.result()
        else:
            raise TimeoutError

    @coroutine
    def run(self):
        while True:
            now = time.monotonic()
            dead = [k for k, (info, interval, last) in self.devices.items()
                    if last + 3 * interval < now]
            if dead:
                print("SILENTLY DIED:", dead)
            for d in dead:
                del self.devices[d]
            yield from sleep(10)

    def getDevices(self):
        return list(self.devices)

    def registerPropertyMonitor(self, instanceId, key, callback):
        device = self.sync(self.parent.getDevice(instanceId))
        loop = self.parent._ss.loop

        @coroutine
        def caller():
            while True:
                value, ts = yield from getattr(NewValueTimestamp(device), key)
                yield from loop.run_in_executor(None, callback, instanceId,
                                                key, value, ts)
        async(caller(), loop=self.parent._ss.loop)

    def registerDeviceMonitor(self, instanceId, callback):
        device = self.sync(self.parent.getDevice(instanceId))
        loop = self.parent._ss.loop

        @coroutine
        def caller():
            while True:
                f = device._futures.setdefault(None, Future(loop=loop))
                h = yield from f
                yield from loop.run_in_executor(None, callback, instanceId, h)
        async(caller(), loop=self.parent._ss.loop)

    def addInstance(self, instanceId, info, interval=10):
        self.devices[instanceId] = info, interval, time.monotonic()

    def removeInstance(self, instanceId):
        del self.devices[instanceId]

    def instantiateNoWait(self, server, cls, configuration):
        self.parent._ss.emit(
            "call", {server: ["slotStartDevice"]},
            Hash("classId", cls, "deviceId", configuration.get("deviceId", ""),
                 "configuration", configuration))

    def exists(self, instanceId):
        try:
            self.sync(self.parent.call("slotPing", instanceId, instanceId,
                                       True, False), 0.3)
            return True
        except TimeoutError:
            return False

    def set(self, instanceId, key, value, separator=None, timeout=3):
        try:
            return self.sync(self.parent.call("slotReconfigure",
                                              instanceId, Hash(key, value)),
                             timeout=timeout)
        except TimeoutError:
            return False, ""

    def executeNoWait(self, instanceId, slot, *args):
        self.parent._ss.emit("call", {instanceId: [slot]}, *args)

    def execute(self, instanceId, slot, *args, timeout=3):
        try:
            return self.sync(self.parent.call(slot, instanceId, *args),
                             timeout=timeout)
        except TimeoutError:
            return False, ""


class DeviceClient(PythonDevice):
    abstract = True

    def __init__(self):
        h = Hash("_deviceId_", "DeviceClient")
        v = Validator()
        super().__init__(v.validate(self.getClassSchema(), h))
        self.run()

EventLoop.Macro = Macro


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
