from asyncio import coroutine
import os
import socket

from karabo.common.enums import Capabilities, Interfaces
from karabo.common.states import State
from karabo.native import (
    AccessLevel, AccessMode, Assignment, DaqPolicy, Hash)
from karabo.native import (
    Bool, get_timestamp, isSet, Int32, KaraboError, Node,
    TypeHash, TypeSchema, Slot, String)

from karabo import __version__ as karaboVersion

from .alarm import AlarmMixin
from .injectable import InjectMixin
from .logger import Logger
from .signalslot import SignalSlotable, Signal, slot, coslot


class Device(InjectMixin, AlarmMixin, SignalSlotable):
    """This is the base class for all devices.

    It inherits from :class:`~karabo.middlelayer.Configurable` and thus
    you can define expected parameters for it.
    """

    # Version e.g. for classVersion - to be overwritten by external classes
    __version__ = karaboVersion

    _serverId_ = String(
        displayedName="_ServerID_",
        description="Do not set this property, it will be set by the "
                    "device-server",
        requiredAccessLevel=AccessLevel.EXPERT,
        assignment=Assignment.INTERNAL, accessMode=AccessMode.INITONLY,
        defaultValue="__none__",
        daqPolicy=DaqPolicy.OMIT)

    visibility = Int32(
        enum=AccessLevel, displayedName="Visibility",
        description="Configures who is allowed to see this device at all",
        assignment=Assignment.OPTIONAL, defaultValue=AccessLevel.OBSERVER,
        requiredAccessLevel=AccessLevel.EXPERT,
        accessMode=AccessMode.INITONLY,
        daqPolicy=DaqPolicy.OMIT)

    classId = String(
        displayedName="ClassID",
        description="The (factory)-name of the class of this device",
        requiredAccessLevel=AccessLevel.EXPERT,
        accessMode=AccessMode.READONLY,
        daqPolicy=DaqPolicy.OMIT)

    classVersion = String(
        displayedName="Class version",
        description="The version of the class of this device",
        requiredAccessLevel=AccessLevel.ADMIN,
        accessMode=AccessMode.READONLY,
        # No version dependent default value: It would make the static
        # schema version dependent, i.e. introduce fake changes.
        daqPolicy=DaqPolicy.OMIT)

    karaboVersion = String(
        displayedName="Karabo version",
        description="The version of the Karabo framework running this device",
        requiredAccessLevel=AccessLevel.ADMIN,
        accessMode=AccessMode.READONLY,
        # No version dependent default value, see above at "classVersion".
        daqPolicy=DaqPolicy.OMIT)

    serverId = String(
        displayedName="ServerID",
        description="The device-server which this device is running on",
        requiredAccessLevel=AccessLevel.EXPERT,
        accessMode=AccessMode.READONLY,
        daqPolicy=DaqPolicy.OMIT)

    hostName = String(
        displayedName="Host",
        description="Do not set this property, it will be set by the "
                    "device-server",
        requiredAccessLevel=AccessLevel.EXPERT,
        assignment=Assignment.INTERNAL, accessMode=AccessMode.INITONLY,
        daqPolicy=DaqPolicy.OMIT)

    pid = Int32(
        displayedName="Process ID",
        defaultValue=0,
        description="The unix process ID of the device (i.e. of the server)",
        requiredAccessLevel=AccessLevel.EXPERT,
        accessMode=AccessMode.READONLY,
        daqPolicy=DaqPolicy.OMIT)

    state = String(
        displayedName="State", enum=State, displayType='State',
        description="The current state the device is in",
        accessMode=AccessMode.READONLY, assignment=Assignment.OPTIONAL,
        defaultValue=State.UNKNOWN,
        daqPolicy=DaqPolicy.OMIT)

    status = String(
        displayedName="Status",
        description="A more detailed status description",
        accessMode=AccessMode.READONLY, assignment=Assignment.OPTIONAL,
        defaultValue="",
        daqPolicy=DaqPolicy.OMIT)

    lockedBy = String(
        displayedName="Locked By",
        description="The name of the device holding a lock on this one "
                    "(empty if not locked)",
        accessMode=AccessMode.RECONFIGURABLE, assignment=Assignment.OPTIONAL,
        requiredAccessLevel=AccessLevel.EXPERT, defaultValue="",
        daqPolicy=DaqPolicy.OMIT)

    @Slot(displayedName="Clear Lock", requiredAccessLevel=AccessLevel.EXPERT,
          description="Clear the lock on this device")
    @coroutine
    def slotClearLock(self):
        """ Clear the lock on this device """
        self.lockedBy = ""

    lastCommand = String(
        displayedName="Last command",
        defaultValue="",
        description="The last slot called.",
        accessMode=AccessMode.READONLY,
        requiredAccessLevel=AccessLevel.EXPERT,
        daqPolicy=DaqPolicy.OMIT)

    archive = Bool(
        displayedName="Archive",
        description="Decides whether the properties of this device "
                    "will be logged or not",
        requiredAccessLevel=AccessLevel.EXPERT,
        accessMode=AccessMode.INITONLY, assignment=Assignment.OPTIONAL,
        defaultValue=True,
        daqPolicy=DaqPolicy.OMIT)

    log = Node(Logger,
               description="Logging settings",
               displayedName="Logger",
               requiredAccessLevel=AccessLevel.EXPERT)

    signalChanged = Signal(TypeHash(), String())
    signalStateChanged = Signal(TypeHash(), String())
    signalSchemaUpdated = Signal(TypeSchema(), String())

    def __init__(self, configuration):
        super(Device, self).__init__(configuration)
        if not isSet(self.serverId):
            self.serverId = self._serverId_
        if not isSet(self.hostName):
            self.hostName = socket.gethostname().partition('.')[0]

        self.classId = type(self).__name__
        # class version is the package name plus version
        # the latter should be overwritten in inheriting classes
        classPackage = self.__module_orig__.split('.', 1)[0]
        self.classVersion = f"{classPackage}-{type(self).__version__}"
        self.karaboVersion = karaboVersion
        self.pid = os.getpid()
        self._statusInfo = "ok"

    def _initInfo(self):
        info = super(Device, self)._initInfo()
        info["type"] = "device"
        info["classId"] = self.classId.value
        info["serverId"] = self.serverId.value
        info["visibility"] = self.visibility.value
        info["host"] = self.hostName
        info["status"] = self._statusInfo
        info["archive"] = self.archive.value

        # device capabilities are encoded in a bit mask field
        capabilities = 0
        if hasattr(self, "availableScenes"):
            capabilities |= Capabilities.PROVIDES_SCENES
        if hasattr(self, "availableMacros"):
            capabilities |= Capabilities.PROVIDES_MACROS
        if hasattr(self, "interfaces"):
            capabilities |= Capabilities.PROVIDES_INTERFACES

        info["capabilities"] = capabilities

        interfaces = 0
        if hasattr(self, "interfaces"):
            for description in self.interfaces.value:
                if description in Interfaces.__members__:
                    interfaces |= Interfaces[description]
                else:
                    raise NotImplementedError(
                        "Provided interface is not supported: {}".format(
                            description))

            info["interfaces"] = interfaces

        return info

    @coroutine
    def _run(self, **kwargs):
        self._ss.enter_context(self.log.setBroker(self._ss))
        yield from super(Device, self)._run(**kwargs)

    @slot
    def slotGetConfiguration(self):
        return self.configurationAsHash(), self.deviceId

    @slot
    def slotGetTime(self):
        """Return the actual time information of this device

        This slot call return a Hash with key ``time`` and the attributes
        provide an actual timestamp with train Id information.
        """
        h = Hash("time", True)
        h["time", ...] = get_timestamp().toDict()

        return h

    def _checkLocked(self, message):
        """return an error message if device is locked or None if not"""
        lock_clear = ("slotClearLock"
                      in self._ss.get_property(message, "slotFunctions"))
        if (self.lockedBy and self.lockedBy !=
                self._ss.get_property(message, "signalInstanceId") and
                not lock_clear):
            return 'Device locked by "{}"'.format(self.lockedBy)
        return None

    def slotReconfigure(self, reconfiguration, message):
        # This can only be called as a slot
        msg = self._checkLocked(message)
        if msg is not None:
            raise KaraboError(msg)
        yield from super(Device, self).slotReconfigure(reconfiguration)
        self.update()

    slotReconfigure = coslot(slotReconfigure, passMessage=True)

    def update(self):
        """Update the instanceInfo Hash according to the status info
        """
        statusInfo = "error" if self.state == State.ERROR else "ok"
        if statusInfo != self._statusInfo:
            self.updateInstanceInfo(Hash("status", statusInfo))
            self._statusInfo = statusInfo
        super(Device, self).update()

    @slot
    def slotGetSchema(self, onlyCurrentState):
        return self.getDeviceSchema(
            state=self.state if onlyCurrentState else None), self.deviceId

    def _notifyNewSchema(self):
        """Notfiy the network that our schema has changed"""
        self.signalSchemaUpdated(self.getDeviceSchema(), self.deviceId)

    @coslot
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
        yield from self.publishInjectedParameters()

        ret = Hash()
        ret["success"] = success
        ret["instanceId"] = self.deviceId
        ret["updatedSchema"] = self.getDeviceSchema()
        ret["requestedUpdate"] = updates

        return ret
