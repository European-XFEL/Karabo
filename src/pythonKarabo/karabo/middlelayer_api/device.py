from asyncio import coroutine
import socket

from karabo.common.enums import Capabilities
from karabo.common.states import State
from .alarm import AlarmMixin
from .basetypes import isSet
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
        accessMode=AccessMode.READONLY)

    state = String(
        displayedName="State", enum=State,
        description="The current state the device is in",
        accessMode=AccessMode.READONLY, assignment=Assignment.OPTIONAL,
        defaultValue=State.UNKNOWN)

    status = String(
        displayedName="Status",
        description="A more detailed status description",
        accessMode=AccessMode.READONLY, assignment=Assignment.OPTIONAL,
        defaultValue="")

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

    def __init__(self, configuration):
        super(Device, self).__init__(configuration)
        if not isSet(self.serverId):
            self.serverId = self._serverId_

        self.hostName, _, self.domainname = socket.gethostname().partition('.')
        self.classId = type(self).__name__

    def _initInfo(self):
        info = super(Device, self)._initInfo()
        info["type"] = "device"
        info["classId"] = self.classId.value
        info["serverId"] = self.serverId.value
        info["visibility"] = self.visibility.value
        info["compatibility"] = self.__class__.__version__
        info["host"] = self.hostName
        info["status"] = "ok"
        info["archive"] = self.archive.value

        # device capabilities are encoded in a bit mask field
        capabilities = 0
        if hasattr(self, "availableScenes"):
            capabilities |= Capabilities.PROVIDES_SCENES
        info["capabilities"] = capabilities

        return info

    @coroutine
    def _run(self, **kwargs):
        self._ss.enter_context(self.log.setBroker(self._ss))
        yield from super(Device, self)._run(**kwargs)

    @slot
    def slotGetConfiguration(self):
        return self.configurationAsHash(), self.deviceId

    def _checkLocked(self, message):
        """return an error message if device is locked or None if not"""
        if (self.lockedBy and self.lockedBy !=
                message.properties["signalInstanceId"].decode("ascii")):
            return 'Device locked by "{}"'.format(self.lockedBy)

    def slotReconfigure(self, reconfiguration, message):
        # This can only be called as a slot
        msg = self._checkLocked(message)
        if msg is not None:
            raise KaraboError(msg)
        yield from super(Device, self).slotReconfigure(reconfiguration)
        self.update()

    slotReconfigure = coslot(slotReconfigure, passMessage=True)

    @slot
    def slotGetSchema(self, onlyCurrentState):
        return self.getDeviceSchema(
            state=self.state if onlyCurrentState else None), self.deviceId

    def _notifyNewSchema(self):
        """Notfiy the network that our schema has changed"""
        self.signalSchemaUpdated(self.getDeviceSchema(), self.deviceId)
