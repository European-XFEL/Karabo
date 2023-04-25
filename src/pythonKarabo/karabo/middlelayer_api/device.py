# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import os
import socket
from asyncio import sleep

from karabo import __version__ as karaboVersion
from karabo.common.enums import Capabilities, Interfaces
from karabo.common.states import State
from karabo.native import (
    AccessLevel, AccessMode, Assignment, DaqPolicy, Hash, Int32, KaraboError,
    Node, Slot, String, TimeMixin, TypeHash, TypeSchema, get_timestamp, isSet)

from .alarm import AlarmMixin
from .injectable import InjectMixin
from .logger import build_logger_node
from .pipeline import OutputChannel
from .signalslot import Signal, SignalSlotable, slot
from .utils import get_property_hash


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
    async def slotClearLock(self):
        """ Clear the lock on this device """
        self.lockedBy = ""

    lastCommand = String(
        displayedName="Last command",
        defaultValue="",
        description="The last slot called.",
        accessMode=AccessMode.READONLY,
        requiredAccessLevel=AccessLevel.EXPERT,
        daqPolicy=DaqPolicy.OMIT)

    log = Node(
        build_logger_node(),
        description="Logging settings",
        displayedName="Logger",
        requiredAccessLevel=AccessLevel.EXPERT)

    signalChanged = Signal(TypeHash(), String())
    signalStateChanged = Signal(TypeHash(), String())
    signalSchemaUpdated = Signal(TypeSchema(), String())

    @property
    def is_initialized(self):
        """Check if the device is online and has passed onInitialization"""
        return super().is_initialized

    def getLocalDevice(self, instanceId):
        """Returns the instance of a device if present in the local server

        This device instance can be used to shortcut broker communication.

        This returns a strong reference, create a weakref reference if
        really needed. You can also provide a callback when the local device
        is destroyed::

            async def onInitialization(self):
                device = self.getLocalDevice(instanceId)

                def delete(weak_dev):
                    # Handle device being removed

                self.local_device = weakref.ref(device, delete)

        :returns: Device instance or `None` if not found.

        Note: Added in Karabo 2.15.
        """
        server = super().device_server
        return server.deviceInstanceMap.get(instanceId)

    def __init__(self, configuration):
        self.__timeServerId = configuration.pop("__timeServerId", "None")
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
        self._statusInfo = self.__get_status_info()

    def __get_status_info(self):
        if self.state == State.UNKNOWN:
            status = "unknown"
        elif self.state == State.ERROR:
            status = "error"
        else:
            status = "ok"
        return status

    def _initInfo(self):
        info = super(Device, self)._initInfo()
        info["type"] = "device"
        info["classId"] = self.classId.value
        info["serverId"] = self.serverId.value
        info["visibility"] = self.visibility.value
        info["host"] = self.hostName
        info["status"] = self._statusInfo

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

    async def _run(self, **kwargs):
        self._ss.enter_context(self.log.setBroker(self._ss))
        await super(Device, self)._run(**kwargs)

    @slot
    def slotGetConfiguration(self):
        return self.configurationAsHash(), self.deviceId

    @slot
    def slotGetConfigurationSlice(self, info):
        """Public slot to retrieve the values of a list of `paths`

        :param info: Hash with {key: `paths`, value: list of strings}

        returns: A Hash with the values and attributes for each requested
        property key.
        """
        return get_property_hash(self, info["paths"])

    @slot
    def slotGetTime(self, info=None):
        """Return the actual time information of this device

        This slot call return a Hash with key ``time`` and the attributes
        provide an actual timestamp with train Id information.

        The slot call further provides a reference time information via key
        ``reference`` and the attributes provide the train id.

        This method has an empty input argument to allow a generic protocol.
        """
        h = Hash("time", True)
        h["time", ...] = get_timestamp().toDict()
        h["timeServerId"] = self.__timeServerId
        h["reference"] = True
        h["reference", ...] = TimeMixin.toDict()

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

    async def slotReconfigure(self, reconfiguration, message):
        # This can only be called as a slot
        msg = self._checkLocked(message)
        if msg is not None:
            raise KaraboError(msg)
        await super(Device, self).slotReconfigure(reconfiguration)
        self.update()

    slotReconfigure = slot(slotReconfigure, passMessage=True)

    def update(self):
        """Update the instanceInfo Hash according to the status info
        """
        statusInfo = self.__get_status_info()
        if statusInfo != self._statusInfo:
            self.updateInstanceInfo(Hash("status", statusInfo))
            self._statusInfo = statusInfo
        super(Device, self).update()

    async def setOutputSchema(self, *args):
        """This is a wrapper function to change the schema of existing output
        channels.

        For each output channel that is to be changed its key name and the
        new Schema have to be provided, e.g.

            await self.setOutputSchema("output1", schema1,
                                       "output2", schema2,
                                       ...)

        Note: The existing output channel is closed while changing the schema.
        """
        if len(args) % 2 > 0:
            raise RuntimeError("Arguments passed in setOutputSchema "
                               "need to be pairs")

        async def _build_output(key, schema):
            """Rebuild a single output channel for `key` and `schema`"""
            assert "." not in key, "Nested output channels are not allowed!"

            channel_desc = getattr(type(self), key, None)
            assert type(channel_desc) is OutputChannel
            _, attrs = channel_desc.toSchemaAndAttrs(None, None)
            # Obtain previous configuration. This is safe
            # since output channel schema nodes do not have a configuration
            channel = getattr(self, key)
            config_hash = channel.configurationAsHash()
            config = {k: v for k, v in config_hash.items()}
            # Note: We have to close the output channel and all related socket
            # connections! This will lead to all input channels closing
            # and asking for a new schema!
            await channel.close()
            output_channel = OutputChannel(schema, strict=False, **attrs)
            setattr(self.__class__, key, output_channel)

            return config

        configurations = []
        for key, schema in zip(args[::2], args[1::2]):
            config = await _build_output(key, schema)
            configurations.extend([key, config])

        await self.publishInjectedParameters(*configurations)

    @slot
    def slotGetSchema(self, onlyCurrentState):
        return self.getDeviceSchema(
            state=self.state if onlyCurrentState else None), self.deviceId

    def _notifyNewSchema(self):
        """Notfiy the network that our schema has changed"""
        self.signalSchemaUpdated(self.getDeviceSchema(), self.deviceId)

    @slot
    async def slotUpdateSchemaAttributes(self, updates):
        """Apply runtime attribute updates to the device

        This method will apply all attributes to the device. If a single
        attribute cannot be set, the success return boolean is set to False.
        However, even with a single failure, all other remaining attributes
        are still tried to be set.

        :param updates: List of Hashes with "path", "attribute" and "value"
                        as keys
        """
        success = self.applyRuntimeUpdates(updates)
        await self.publishInjectedParameters()

        ret = Hash()
        ret["success"] = success
        ret["instanceId"] = self.deviceId
        ret["updatedSchema"] = self.getDeviceSchema()
        ret["requestedUpdate"] = updates

        return ret


class DeviceClientBase(Device):
    """Keep track of other devices

    A :class:`~karabo.middlelayer.Device` which also inherits from this class
    keeps track of all the other devices in this Karabo installation. Without
    inheriting from this class, listing other devices is impossible."""
    abstract = True
    wait_topology = True

    def __init__(self, configuration):
        # "unknown" is default type for bare C++ SignalSlotable
        self.systemTopology = Hash("device", Hash(), "server", Hash(),
                                   "macro", Hash(), "client", Hash(),
                                   "unknown", Hash())
        self.loggerMap = Hash()
        super().__init__(configuration)

    async def _run(self, **kwargs):
        await super()._run(**kwargs)
        self._ss.emit("call", {"*": ["slotPing"]}, self.deviceId, 0, False)
        # We are collecting all the instanceInfo's and wait for their arrival
        # before the device comes online.
        # Some clients, such as ikarabo, don't want to wait this additional
        # time
        if self.wait_topology:
            await sleep(3)

    @slot
    async def slotInstanceNew(self, instanceId, info):
        self.removeServerChildren(instanceId, info)
        self.updateSystemTopology(instanceId, info, "instanceNew")
        await super().slotInstanceNew(instanceId, info)

    @slot
    def slotInstanceUpdated(self, instanceId, info):
        self.updateSystemTopology(instanceId, info, "instanceUpdated")
        super().slotInstanceUpdated(instanceId, info)

    @slot
    def slotInstanceGone(self, instanceId, info):
        self.removeServerChildren(instanceId, info)
        self.systemTopology[info["type"]].pop(instanceId, None)
        return super().slotInstanceGone(instanceId, info)

    @slot
    def slotPingAnswer(self, deviceId, info):
        self.updateSystemTopology(deviceId, info, None)

    def removeServerChildren(self, instanceId, info):
        """Cleanup the device children from the server
        """
        if info["type"] == "server":
            devices = [k for k, v, a in self.systemTopology["device"].iterall()
                       if a["serverId"] == instanceId]
            for deviceId in devices:
                self.systemTopology["device"].pop(deviceId, None)

    def updateSystemTopology(self, instanceId, info, task):
        type = info["type"]
        ret = Hash(type, Hash())
        ret[type][instanceId] = Hash()
        ret[type][instanceId, ...] = dict(info.items())
        self.systemTopology.merge(ret)
        return ret
