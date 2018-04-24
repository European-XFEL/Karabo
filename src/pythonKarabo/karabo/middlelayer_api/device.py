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

    __version__ = "2.2"

    _serverId_ = String(
        displayedName="_ServerID_",
        description="Do not set this property, it will be set by the "
                    "device-server",
        requiredAccessLevel=AccessLevel.EXPERT,
        assignment=Assignment.INTERNAL, accessMode=AccessMode.INITONLY,
        defaultValue="__none__")

    @Int32(
        enum=AccessLevel, displayedName="Visibility",
        description="Configures who is allowed to see this device at all",
        assignment=Assignment.OPTIONAL, defaultValue=AccessLevel.OBSERVER,
        requiredAccessLevel=AccessLevel.EXPERT,
        accessMode=AccessMode.RECONFIGURABLE)
    def visibility(self, newValue):
        # This setter is already called during initialisation and then there is
        # no need to publish yet:
        if newValue != self.visibility and self._ss is not None:
            self.updateInstanceInfo(Hash("visibility", newValue))
        self.visibility = newValue

    classId = String(
        displayedName="ClassID",
        description="The (factory)-name of the class of this device",
        requiredAccessLevel=AccessLevel.EXPERT,
        accessMode=AccessMode.READONLY)

    classVersion = String(
        displayedName="Class version",
        description="The version of the class of this device",
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
        displayedName="State", enum=State, displayType='State',
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

    lastCommand = String(
        displayedName="Last command",
        defaultValue="",
        description="The last slot called.",
        accessMode=AccessMode.READONLY,
        requiredAccessLevel=AccessLevel.EXPERT)

    @Bool(
        displayedName="Archive",
        description="Decides whether the properties of this device "
                    "will be logged or not",
        requiredAccessLevel=AccessLevel.EXPERT,
        accessMode=AccessMode.RECONFIGURABLE, assignment=Assignment.OPTIONAL,
        defaultValue=True)
    def archive(self, newValue):
        # This setter is already called during initialisation and then there is
        # no need to publish yet:
        if newValue != self.archive and self._ss is not None:
            self.updateInstanceInfo(Hash("archive", newValue))
        self.archive = newValue

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
        self.classVersion = type(self).__version__

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
        if hasattr(self, "availableMacros"):
            capabilities |= Capabilities.PROVIDES_MACROS
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

    @slot
    def slotUpdateSchemaAttributes(self, updates):
        """Apply runtime attribute updates to the device

        This method will apply all attributes to the device. If a single
        attribute cannot be set, the success return boolean is set to False.
        However, even with a single failure, all other remaining attributes
        are still tried to be set.

        :param updates: List of Hashes with "path", "attribute" and "value"
                        as keys
        """
        success = self.applyRuntimeUpdates(updates)
        if success:
            self._notifyNewSchema()

        ret = Hash()
        ret["success"] = success
        ret["instanceId"] = self.deviceId
        ret["updatedSchema"] = self.getDeviceSchema()
        ret["requestedUpdate"] = updates

        return ret
