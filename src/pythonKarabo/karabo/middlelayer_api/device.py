from asyncio import coroutine
import socket

from .enums import AccessLevel, AccessMode, Assignment
from .exceptions import KaraboError
from .hash import Bool, Hash, HashType, Int32, Schema, SchemaHashType, String
from .logger import Logger
from .schema import Validator, Node
from .signalslot import SignalSlotable, Signal, slot, coslot


class Device(SignalSlotable):
    """This is the base class for all devices.

    It inherits from :class:`~karabo.middlelayer.Configurable` and thus
    you can define expected parameters for it.
    """

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

    def _initInfo(self):
        info = super()._initInfo()
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
        yield from super()._run()

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

    @coslot
    def slotReconfigure(self, reconfiguration):
        try:
            yield from super().slotReconfigure(reconfiguration)
        except KaraboError as e:
            self.logger.exception("Failed to set property")
            return False, str(e)
        self.signalChanged(self.configurationAsHash(), self.deviceId)
        return True, ""

    @slot
    def slotGetSchema(self, onlyCurrentState):
        return self.getDeviceSchema(
            state=self.state if onlyCurrentState else None), self.deviceId

    @slot
    def slotInstanceNew(self, instanceId, info):
        pass

    @slot
    def slotInstanceUpdated(self, instanceId, info):
        pass

    @slot
    def slotInstanceGone(self, instanceId, info):
        pass
