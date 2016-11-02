from asyncio import coroutine
import socket

from karabo.common.states import State
from .alarm import AlarmMixin
from .enums import AccessLevel, AccessMode, Assignment
from .exceptions import KaraboError
from .hash import Bool, Hash, HashType, Int32, SchemaHashType, String
from .logger import Logger
from .schema import Node
from .signalslot import SignalSlotable, Signal, slot, coslot


class Device(AlarmMixin, SignalSlotable):
    """This is the base class for all devices.

    It inherits from :class:`~karabo.middlelayer.Configurable` and thus
    you can define expected parameters for it.
    """

    __version__ = "1.3"

    _serverId_ = String(
        displayedName="_ServerID_",
        description="Do not set this property, it will be set by the "
                    "device-server",
        requiredAccessLevel=AccessLevel.EXPERT,
        assignment=Assignment.INTERNAL, accessMode=AccessMode.INITONLY,
        defaultValue="__none__")

    visibility = Int32(
        enum=AccessLevel, displayedName="Visibility",
        description="Configures who is allowed to see this device at all",
        assignment=Assignment.OPTIONAL, defaultValue=AccessLevel.OBSERVER,
        requiredAccessLevel=AccessLevel.EXPERT,
        accessMode=AccessMode.RECONFIGURABLE)

    classId = String(
        displayedName="ClassID",
        description="The (factory)-name of the class of this device",
        requiredAccessLevel=AccessLevel.EXPERT,
        accessMode=AccessMode.READONLY)

    serverId = String(
        displayedName="ServerID",
        description="The device-server which this device is running on",
        requiredAccessLevel=AccessLevel.EXPERT,
        accessMode=AccessMode.READONLY)

    hostName = String(
        displayedName="Host",
        description="The name of the host where this device runs",
        requiredAccessLevel=AccessLevel.EXPERT,
        accessMode=AccessMode.READONLY,
        defaultValue=socket.gethostname().partition('.')[0])

    state = String(
        displayedName="State", enum=State,
        description="The current state the device is in",
        accessMode=AccessMode.READONLY, assignment=Assignment.OPTIONAL,
        defaultValue=State.UNKNOWN)

    lockedBy = String(
        displayedName="Locked By",
        description="The name of the device holding a lock on this one "
                    "(empty if not locked)",
        accessMode=AccessMode.RECONFIGURABLE, assignment=Assignment.OPTIONAL,
        requiredAccessLevel=AccessLevel.EXPERT, defaultValue="")

    archive = Bool(
        displayedName="Archive",
        description="Decides whether the properties of this device "
                    "will be logged or not",
        requiredAccessLevel=AccessLevel.EXPERT,
        accessMode=AccessMode.RECONFIGURABLE, assignment=Assignment.OPTIONAL,
        defaultValue=True)

    log = Node(Logger,
               description="Logging settings",
               displayedName="Logger",
               requiredAccessLevel=AccessLevel.EXPERT)

    signalChanged = Signal(HashType(), String())
    signalStateChanged = Signal(HashType(), String())
    signalSchemaUpdated = Signal(SchemaHashType(), String())

    subclasses = {}

    def __init__(self, configuration):
        super(Device, self).__init__(configuration)
        if not hasattr(self, "serverId"):
            self.serverId = self._serverId_

        self.hostname, _, self.domainname = socket.gethostname().partition('.')
        self.classId = type(self).__name__

    @classmethod
    def register(cls, name, dict):
        super(Device, cls).register(name, dict)
        if "abstract" not in dict:
            Device.subclasses[name] = cls

    def _initInfo(self):
        info = super(Device, self)._initInfo()
        info["type"] = "device"
        info["classId"] = self.classId.value
        info["serverId"] = self.serverId.value
        info["visibility"] = self.visibility.value
        info["compatibility"] = self.__class__.__version__
        info["host"] = self.hostname
        info["status"] = "ok"
        info["archive"] = self.archive.value
        return info

    @coroutine
    def _run(self):
        yield from super(Device, self)._run()

        self._ss.enter_context(self.log.setBroker(self._ss))
        self.logger = self.log.logger

    @slot
    def slotGetConfiguration(self):
        return self.configurationAsHash(), self.deviceId

    def configurationAsHash(self):
        r = Hash()
        for k in self._allattrs:
            a = getattr(self, k, None)
            if a is not None:
                v = getattr(type(self), k)
                value, attrs = v.toDataAndAttrs(a)
                r[k] = value
                r[k, ...].update(attrs)
        return r

    def _checkLocked(self, message):
        """return an error message if device is locked or None if not"""
        if (self.lockedBy and self.lockedBy !=
                message.properties["signalInstanceId"].decode("ascii")):
            return 'Device locked by "{}"'.format(self.lockedBy)

    def slotReconfigure(self, reconfiguration, message):
        """This can only be called as a slot"""
        msg = self._checkLocked(message)
        if msg is not None:
            return False, msg
        try:
            yield from super(Device, self).slotReconfigure(reconfiguration)
        except KaraboError as e:
            self.logger.warn(e.args[0])
            return False, e.args[0]
        self.signalChanged(self.configurationAsHash(), self.deviceId)
        return True, ""

    slotReconfigure = coslot(slotReconfigure, passMessage=True)

    @slot
    def slotGetSchema(self, onlyCurrentState):
        return self.getDeviceSchema(
            state=self.state if onlyCurrentState else None), self.deviceId
