# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.

import copy
import os
import signal
import socket
import sys
import threading
import time

from karabind import (
    ALARM_ELEMENT, BOOL_ELEMENT, FLOAT_ELEMENT, INT32_ELEMENT, MICROSEC,
    NODE_ELEMENT, OBSERVER, OVERWRITE_ELEMENT, SLOT_ELEMENT, STATE_ELEMENT,
    STRING_ELEMENT, UINT32_ELEMENT, WRITE, AccessLevel, AccessType,
    AssemblyRules, Broker, ChannelMetaData, ConnectionStatus, Epochstamp,
    EventLoop, Hash, HashFilter, HashMergePolicy, Logger, MetricPrefix, Schema,
    SignalSlotable, Timestamp, Trainstamp, Unit, Validator,
    ValidatorValidationRules, VectorHash, loadFromFile)
from karabo import __version__ as karaboVersion
from karabo.common.api import (
    KARABO_CLASS_ID_ALARM, KARABO_CLASS_ID_STATE,
    KARABO_LOGGER_CONTENT_DEFAULT, AlarmCondition, Capabilities, Interfaces,
    State, karabo_deprecated)

from .configurator import Configurator
from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
# Use patched DeviceClient, not the one directly from bound_tool:
from .device_client import DeviceClient
from .no_fsm import NoFsm


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("PythonDevice", karaboVersion)
class PythonDevice(NoFsm):
    """The PythonDevice class is the basis for all karabo.bound devices

    Devices implemented in the karabo.bound API should derive from this
    class. It provides an interface to the distributed system and holds
    properties that represent the state of a device. Commands may be exposed
    through it using so-called slots.

    Devices can optionally either use a full finite state machine, deriving
    from the `FSM` class, or use a simplified approach (`NoFSM`), where state
    updates are explicitly called from the device logic. The latter is the
    default if a device class just inherits from this PythonDevice.
    Otherwise the device class has to also inherit from a concrete FSM
    implementation that inherits from BaseFsm.

    Devices run in a separate process, but internally use an event loop with
    multiple threads to serve requests from the distributed system.
    """

    instanceCountPerDeviceServer = dict()
    instanceCountLock = threading.Lock()
    connectionParams = Hash(Broker.brokerTypeFromEnv(), Hash())
    timeServerId = None
    _loggerCfg = None

    @staticmethod
    def expectedParameters(expected):
        (
            STRING_ELEMENT(expected).key("_deviceId_")
            .displayedName("_DeviceID_")
            .description("Do not set this property, it will be set by the"
                         " device-server")
            .expertAccess().assignmentInternal().noDefaultValue().init()
            .commit(),

            STRING_ELEMENT(expected).key("deviceId")
            .displayedName("DeviceID")
            .description("The device instance ID uniquely identifies a device"
                         " instance in the distributed system")
            .readOnly()
            .commit(),

            INT32_ELEMENT(expected).key("heartbeatInterval")
            .displayedName("Heartbeat interval")
            .description("The heartbeat interval")
            .assignmentOptional()
            .defaultValue(20)  # smaller than C++ device: own process!
            .minInc(10)  # avoid too much traffic
            .adminAccess()
            .commit(),

            STRING_ELEMENT(expected).key("_serverId_")
            .displayedName("_ServerID_")
            .description("Do not set this property, it will be set by the"
                         " device-server")
            .expertAccess().assignmentInternal().noDefaultValue().init()
            .commit(),

            INT32_ELEMENT(expected).key("visibility")
            .displayedName("Visibility")
            .description("Configures who is allowed to see this device at all")
            .assignmentOptional().defaultValue(AccessLevel(OBSERVER))
            .expertAccess()
            .init()
            .commit(),

            STRING_ELEMENT(expected).key("classId")
            .displayedName("ClassID")
            .description("The (factory)-name of the class of this device")
            .readOnly().initialValue(PythonDevice.__classid__)
            .commit(),

            STRING_ELEMENT(expected).key("classVersion")
            .displayedName("Class version")
            .description("The version of the class of this device defined in"
                         " KARABO_CLASSINFO, prepended by package name")
            .expertAccess()
            # No version dependent initial value: It would make the static
            # schema version dependent, i.e. introduce fake changes.
            .readOnly()
            .commit(),

            STRING_ELEMENT(expected).key("karaboVersion")
            .displayedName("Karabo version")
            .description("The version of the Karabo framework running this "
                         "device")
            .expertAccess()
            .readOnly()
            # No version dependent initial value, see above at "classVersion".
            .commit(),

            STRING_ELEMENT(expected).key("serverId")
            .displayedName("ServerID")
            .description("The device-server on which this device "
                         "is running on")
            .expertAccess()
            .readOnly()
            .commit(),

            STRING_ELEMENT(expected).key("hostName")
            .displayedName("Host")
            .description("Do not set this property, it will be set by the"
                         " device-server.")
            .expertAccess()
            .assignmentInternal().noDefaultValue()
            .init()
            .commit(),

            INT32_ELEMENT(expected).key("pid")
            .displayedName("Process ID")
            .description("The unix process ID of the device")
            .expertAccess()
            .readOnly().initialValue(0)
            .commit(),

            STATE_ELEMENT(expected).key("state")
            .displayedName("State")
            .description("The current state the device is in")
            .initialValue(State.UNKNOWN)
            .commit(),

            STRING_ELEMENT(expected).key("status")
            .displayedName("Status")
            .description("A more detailed status description")
            .readOnly().initialValue("")
            .commit(),

            ALARM_ELEMENT(expected).key("alarmCondition")
            .displayedName("Alarm condition")
            .description("The current alarm condition of the device. Evaluates"
                         " to the highest condition on any property if not set"
                         " manually.")
            .initialValue(AlarmCondition.NONE)
            .commit(),

            STRING_ELEMENT(expected).key("lockedBy")
            .displayedName("Locked by")
            .reconfigurable()
            .assignmentOptional().defaultValue("")
            .setSpecialDisplayType("lockedBy")
            .commit(),

            SLOT_ELEMENT(expected).key("slotClearLock")
            .displayedName("Clear Lock")
            .expertAccess()
            .commit(),

            STRING_ELEMENT(expected).key("lastCommand")
            .displayedName("Last command")
            .description("The last slot called.")
            .adminAccess()
            .readOnly().initialValue("")
            .commit(),

            NODE_ELEMENT(expected).key("performanceStatistics")
            .displayedName("Performance Statistics")
            .description("Accumulates some statistics")
            .expertAccess()
            .commit(),

            BOOL_ELEMENT(expected).key("performanceStatistics."
                                       "messagingProblems")
            .displayedName("Messaging problems")
            .description("If true, there is a problem consuming"
                         " broker messages")
            .expertAccess()
            .readOnly().initialValue(False)
            .commit(),

            BOOL_ELEMENT(expected).key("performanceStatistics.enable")
            .displayedName("Enable Performance Indicators")
            .description("Enables some statistics to follow the"
                         " performance of an individual device")
            .reconfigurable()
            .expertAccess()
            .assignmentOptional().defaultValue(False)
            .commit(),

            FLOAT_ELEMENT(expected).key("performanceStatistics."
                                        "processingLatency")
            .displayedName("Processing latency")
            .description("Average time interval between remote message"
                         " sending and processing it in this device.")
            .unit(Unit.SECOND).metricPrefix(MetricPrefix.MILLI)
            .expertAccess()
            .readOnly().initialValue(0.0)
            .warnHigh(3000.)  # 3 s
            .info("Long average time between message being sent and"
                  " start of its processing")
            .needsAcknowledging(False)
            .alarmHigh(10000.)  # 10 s
            .info("Very long average time between message being sent "
                  "and start of its processing")
            .needsAcknowledging(False)
            .commit(),

            UINT32_ELEMENT(expected).key("performanceStatistics"
                                         ".maxProcessingLatency")
            .displayedName("Maximum latency")
            .description("Maximum processing latency within averaging"
                         " interval.")
            .unit(Unit.SECOND).metricPrefix(MetricPrefix.MILLI)
            .expertAccess()
            .readOnly().initialValue(0)
            .commit(),

            UINT32_ELEMENT(expected).key("performanceStatistics.numMessages")
            .displayedName("Number of messages")
            .description("Number of messages received within"
                         " averaging interval.")
            .unit(Unit.COUNT)
            .expertAccess()
            .readOnly().initialValue(0)
            .commit(),

            UINT32_ELEMENT(expected).key("performanceStatistics"
                                         ".maxEventLoopLatency")
            .displayedName("Max. event loop latency")
            .description("Maximum time interval between posting a message on"
                         " the central event loop and processing it within"
                         " averaging interval.")
            .unit(Unit.SECOND).metricPrefix(MetricPrefix.MILLI)
            .expertAccess()
            .readOnly().initialValue(0)
            .commit(),

            # Logging config:
            # Expose only the non-appender specific part (only 'priority' now).
            # Would like to use NODE_ELEMENT(..)...appendParametersOf(Logger)
            # and then remove again "ostream", "file" and "cache" from
            # expected["Logger"], but Schema.getParameterHash() returns a copy.
            NODE_ELEMENT(expected).key("Logger")
            .description("Logging settings")
            .displayedName("Logger")
            .expertAccess()
            .commit(),

            # Keep in sync with 'priority' in C++ Logger::expectedParameters
            STRING_ELEMENT(expected).key("Logger.priority")
            .displayedName("Priority")
            .description("The default log priority")
            .options("DEBUG INFO WARN ERROR FATAL")
            .assignmentOptional().defaultValue("INFO")
            .commit(),
        )

    log = None  # make always available, at least as None

    def __init__(self, configuration: Hash | None):
        """The initialization method of a device

        Expects a configuration passed as a Karabo Hash.

        :param configuration: the configuration Hash. It may contain the
               following entries:

               - _serverId_: there id of the hosting server
               - _deviceId_: a usually autogenerated device id
        """
        if configuration is None:
            raise ValueError("Configuration must be Hash object, not None")
        super().__init__(configuration)

        self._parameters = configuration
        if "_serverId_" in self._parameters:
            self.serverid = self._parameters["_serverId_"]
        else:
            self.serverid = "__none__"

        if "_deviceId_" in self._parameters:
            self.deviceid = self._parameters["_deviceId_"]
        else:
            self.deviceid = "__none__"  # TODO: generate uuid

        # Initialize threading locks...
        self._stateChangeLock = threading.Lock()
        self._stateDependentSchema = {}
        self._injectedSchema = Schema()

        # Initialize _client to None (important!)
        self._client = None

        # Initialize hostName
        if "hostName" not in self._parameters:
            self._parameters["hostName"] = (
                socket.gethostname().partition('.')[0])
        self.hostname = self._parameters["hostName"]

        # timeserver related
        self._timeLock = threading.Lock()
        self._timeId = 0
        self._timeSec = 0
        self._timeFrac = 0
        self._timePeriod = 0

        # Setup the validation classes
        rules = ValidatorValidationRules()
        rules.allowAdditionalKeys = False
        rules.allowMissingKeys = True
        rules.allowUnrootedConfiguration = True
        rules.injectDefaults = False
        rules.injectTimestamps = True
        rules.forceInjectedTimestamp = False  # allows to specify case-by-case
        # Internal validator for set(..)
        self.validatorIntern = Validator(rules)
        rules.forceInjectedTimestamp = True
        # External validator for slotReconfigure(..)
        self.validatorExtern = Validator(rules)

        # For broker error handler
        self.lastBrokerErrorStamp = 0

        self.initClassId()
        self.initSchema()

        with self._stateChangeLock:
            self._parameters.set("classId", self.classid)
            # class version is the (base) module name plus __version__ where
            # the latter comes from KARABO_CLASSINFO decorator and should be
            # the repository version
            clsVers = f"{self.__module__.split('.', 1)[0]}-{self.__version__}"
            self._parameters.set("classVersion", clsVers)
            self._parameters.set("karaboVersion", karaboVersion)
            self._parameters.set("deviceId", self.deviceid)
            self._parameters.set("serverId", self.serverid)
            self._parameters.set("pid", os.getpid())

            # Validate first time to assign timestamps
            # Note that invalid property keys are already caught via
            # Configurator(PythonDevice).create in launchPythonDevice below.
            result, error, validated = self.validatorIntern.validate(
                self._fullSchema, self._parameters,
                self.getActualTimestamp())
            if not result:
                raise RuntimeError(error)
            self._parameters.merge(validated,
                                   HashMergePolicy.REPLACE_ATTRIBUTES)

        # Create 'info' hash
        info = Hash("type", "device")
        info["classId"] = self.classid
        info["serverId"] = self.serverid
        info["visibility"] = self["visibility"]
        info["host"] = self.hostname
        currentState = self["state"]
        if currentState is State.ERROR:
            status = "error"
        elif currentState is State.UNKNOWN:
            status = "unknown"
        else:
            status = "ok"
        info["status"] = status

        # device capabilities are encoded in a bit mask field
        capabilities = 0
        if configuration.has("availableScenes"):
            capabilities |= Capabilities.PROVIDES_SCENES
        if configuration.has("availableMacros"):
            capabilities |= Capabilities.PROVIDES_MACROS
        if configuration.has("interfaces"):
            capabilities |= Capabilities.PROVIDES_INTERFACES

        info["capabilities"] = capabilities

        interfaces = 0
        if configuration.has("interfaces"):
            for description in self.get("interfaces"):
                if description in Interfaces.__members__:
                    interfaces |= Interfaces[description]
                else:
                    raise NotImplementedError(
                        "Provided interface is not supported: {}".format(
                            description))

            info["interfaces"] = interfaces

        # Setup device logger (needs self._parameters) before SignalSlotable
        # to log e.g. broker setup (i.e. logging must not log to broker).
        self.loadLogger()
        self.log = Logger.getCategory(self.deviceid)

        # Instantiate SignalSlotable object
        self._ss = SignalSlotable(self.deviceid,
                                  PythonDevice.connectionParams,
                                  self._parameters["heartbeatInterval"], info)

        # Initialize FSM slots if defined
        if hasattr(self, 'initFsmSlots'):
            self.initFsmSlots(self._ss)

        # Initialize Device slots and instantiate all channels
        self._initDeviceSlots()
        self._inputChannelHandlers = {}  # for re-injected InputChannel

        self.initChannels()

        # Register guard for slot calls
        self._ss.registerSlotCallGuardHandler(self.slotCallGuard)

        # Register updateLatencies handler
        self._ss.registerPerformanceStatisticsHandler(self.updateLatencies)

        self._ss.registerBrokerErrorHandler(self.onBrokerError)

    def _finalizeInternalInitialization(self):
        # Start - after all settings/registrations done:
        # Communication (incl. system registration) starts and thus parallelism
        # This is done here and not yet in __init__ to be sure that inheriting
        # devices can register in their __init__ after super(..).__init__(..)
        self._ss.start()  # Can raise e.g. for invalid instanceId

        pid = self["pid"]
        self.log.INFO("'{0.classid}' with deviceId '{0.deviceid}' got started "
                      "on server '{0.serverid}', pid '{1}'.".format(self, pid))

        # Inform server that we are up - fire-and-forget is sufficient.
        self._ss.call(
            self.serverid, "slotDeviceUp", self.deviceid, True, "success")

        # Trigger connection of input channels
        self._ss.connectInputChannels()

        # A Failing FSM start (e.g. calling the registered initialisation
        # methods) will not stop the device, so we post it on the event
        # loop and take care of exceptions.
        # If an exception occurs, we will kill it
        def wrapStartFsm():
            try:
                self.startFsm()
            except Exception as e:
                msg = f"{repr(e)} in initialisation"
                self.log.ERROR(msg)
                self.set("status", msg)
                self._ss.call("", "slotKillDevice")

        # As long as the below connect(..) to the time server is not turned
        # into a non-blocking asyncConnect(..), we better add a thread here:
        # Otherwise, we can get into trouble if the init function (registered
        # by the device coder and called from self.startFsm() above) also does
        # some synchronous operation (e.g. another connect(..)):
        # Both operations block at the same time and we have only two threads
        # (main one and one from our SignalSlotable), so the blocking cannot be
        # resolved and thus both actions time out.
        EventLoop.addThread()
        EventLoop.post(wrapStartFsm)

        if self.timeServerId:
            self.log.DEBUG("Connecting to time server : \"{}\""
                           .format(self.timeServerId))
            # TODO 2: Better use asyncConnect!
            self._ss.connect(self.timeServerId, "signalTimeTick",
                             "", "slotTimeTick")

    @property
    def signalSlotable(self):
        """Get SignalSlotable object embedded in PythonDevice instance."""
        return self._ss

    def loadLogger(self):
        """Load the distributed logger

        Uses config in self._parameters["Logger"] and PythonDevice._loggerCfg
        """
        # Take cfg as passed from server and merge device specific settings
        if PythonDevice._loggerCfg is None:  # for now if started from MDL
            config = self._parameters["Logger"]
        else:
            config = copy.copy(PythonDevice._loggerCfg)
            config.merge(self._parameters["Logger"])

        # Cure the file name of file logger: own dir inside server's log dir:
        if 'file.filename' in config:
            serverLogDir = os.path.dirname(config['file.filename'])
            path = os.path.join(serverLogDir, self.deviceid)
        else:  # Again, for now if started from MDL
            path = os.path.join(os.environ["KARABO"], "var", "log",
                                self.serverid, self.deviceid)
        if not os.path.isdir(path):
            os.makedirs(path)
        path = os.path.join(path, 'device.log')
        config.set('file.filename', path)

        # finally configure the logger
        Logger.configure(config)
        Logger.useOstream()
        Logger.useFile()
        Logger.useCache()

    def remote(self):
        """Return a DeviceClient instance.

        The DeviceClient will use this device's SignalSlotable to interact with
        the distributed system
        :return:
        """
        if self._client is None:
            # SignalSlotable object for reuse
            self._client = DeviceClient(self._ss)
        return self._client

    def set(self, *args, **kwargs):
        """Updates device properties and notifies any observers.
        Note that an update of the device "state" property must be done using
        updateState(..).

        args: can be of length

            * one: expects a Hash, and uses current timestamp
            * two: expects a key, value pair and uses current timestamp or a
                   Hash, timestamp pair
            * three: expects key, value and timestamp

        kwargs: validate: specifies if validation of args should be performed
                before notification. Skipping validation should not be used
                with State or AlarmCondition.

        If a Hash is provided, its keys should be device properties and the
        values should have the proper types. A State or AlarmCondition inside
        a Hash should be given as a string.
        """
        with self._stateChangeLock:
            self._setNoStateLock(*args, **kwargs)

    def _setNoStateLock(self, *args, **kwargs):
        """"
        Internal helper like set, but requires 'with self._stateChangeLock:'
        """

        pars = tuple(args)
        validate = kwargs.get("validate", True)

        if len(pars) == 0 or len(pars) > 3:
            raise SyntaxError("Number of parameters is wrong: "
                              "from 1 to 3 arguments are allowed.")

        # key, value, timestamp args
        if len(pars) == 3:
            key, value, stamp = pars

            if not isinstance(stamp, Timestamp):
                raise TypeError("The 3rd argument should be Timestamp")

            h = Hash()
            # State and AlarmCondition are set as strings - the validator will
            # add necessary "indicateState"/"indicateAlarm" attributes.
            # (Will not work if validate=False!)
            if isinstance(value, State):
                h.set(key, value.name)
            elif isinstance(value, AlarmCondition):
                h.set(key, value.asString())
            else:
                h.set(key, value)
            pars = tuple([h, stamp])

        # hash args
        if len(pars) == 1:
            h = pars[0]
            if not isinstance(h, Hash):
                raise TypeError("The only argument should be a Hash")
            pars = tuple([h, self.getActualTimestamp()])  # add timestamp

        # key, value or hash, timestamp args
        if len(pars) == 2:
            if not isinstance(pars[0], Hash):
                key, value = pars

                h = Hash()
                # See comment above about State and AlarmCondition
                if isinstance(value, State):
                    h.set(key, value.name)
                elif isinstance(value, AlarmCondition):
                    h.set(key, value.asString())
                else:
                    h.set(key, value)
                pars = tuple([h, self.getActualTimestamp()])

            hash, stamp = pars

            validated = None

            if validate:
                result, error, validated = self.validatorIntern.validate(
                    self._fullSchema, hash, stamp)
                if not result:
                    raise RuntimeError("Bad parameter setting attempted, "
                                       "ignore keys {}. Validation "
                                       "reports: {}".format(hash.keys(),
                                                            error))
            else:
                validated = hash
                # Add timestamps
                for path in validated.getPaths():
                    node = validated.getNode(path)
                    attributes = node.getAttributes()
                    stamp.toHashAttributes(attributes)

            if not validated.empty():
                self._parameters.merge(
                    validated, HashMergePolicy.REPLACE_ATTRIBUTES)

                # Hash containing 'state' or at least one reconfigurable
                # key should be signalled by 'signalStateChanged'
                signal = "signalChanged"
                shrt = self.validatorIntern.hasReconfigurableParameter()
                if 'state' in validated or (validate and shrt):
                    signal = "signalStateChanged"

                self._ss.emit(signal, validated, self.deviceid)

    def setVectorUpdate(self, key, updates, updateType, timestamp=None):
        """Concurrency safe update of vector property (not for tables)

        :param key: key of the vector property to update
        :param updates: iterable of items to remove from property vector
                       (starting at the front) or to add (at the end)
        :param updateType: indicates update type, applied individually to all
                           items in 'updates',
                           one of "add", "addIfNotIn", "removeOne", "removeAll"
        :param timestamp: optional timestamp to assign to updated vector
                          property, defaults to self.getActualTimestamp()
        """
        if timestamp is None:
            timestamp = self.getActualTimestamp()

        with self._stateChangeLock:
            # vec is a copy, so we are safe if _setNoStateLock raises
            vec = self._parameters.get(key)
            if updateType == "add":
                vec.extend(updates)
            else:
                for update in updates:
                    if updateType == "addIfNotIn":
                        if update not in vec:
                            vec.append(update)
                    elif updateType == "removeOne":
                        if update in vec:
                            vec.remove(update)
                    elif updateType == "removeAll":
                        for _ in range(vec.count(update)):
                            vec.remove(update)
                    else:
                        raise ValueError(f"Unknown updateType '{updateType}'")
            # Finally update the property
            self._setNoStateLock(key, vec, timestamp)

    def slotLoggerContent(self, info):
        """Slot call to receive logger content from the print logger

        This slot is similar to `slotLoggerContent` for servers except that
        the `serverId` key is substituted with key `deviceId`.

        look in the device_server module for detailed informations
        """
        nMessages = info.get("logs", default=KARABO_LOGGER_CONTENT_DEFAULT)
        content = Logger.getCachedContent(nMessages)
        self._ss.reply(Hash("deviceId", self.deviceid, "content", content))

    def __setitem__(self, key, value):
        """Alternative to `self.set`: `self[key] = value`

        The timestamp is set to the current timestamp
        """
        self.set(key, value, self.getActualTimestamp())

    def writeChannel(self, channelName, data,
                     timestamp=None, safeNDArray=False):
        """Write data to an output channel.

        :param channelName: name given to an OUTPUT_CHANNEL in
                            expectedParameters
        :param data: a Hash with keys as described in the Schema of the
                     channel
        :param timestamp: optional timestamp; if none is given, the current
                          timestamp is used
        :param safeNDArray: Boolean that should be set to 'True' if 'data'
                            contains any 'NDArray' and their data is not
                            changed after this 'writeChannel'. Otherwise,
                            data will be copied if needed, i.e. when the output
                            channel has to queue or serves inner-process
                            receivers.

        Example for an output channel sending an image (key: "image") and
        a frame number (key: "frame"):

        imgArray = numpy.array(...)
        self.writeChannel("output", Hash("image", ImageData(imgArray),
                                         "frame", frameNumber))
        Note:
        The methods 'writeChannel(..)' and 'signalEndOfStream(..)'
        must not be called concurrently.
        """

        channel = self._ss.getOutputChannel(channelName)
        sourceName = f"{self.getInstanceId()}:{channelName}"
        if not timestamp:
            timestamp = self.getActualTimestamp()
        meta = ChannelMetaData(sourceName, timestamp)
        channel.write(data, meta)
        channel.update(safeNDArray=safeNDArray)

    def signalEndOfStream(self, channelName):
        """Signal an end-of-stream event

        The channel is identified by `channnelName`

        Note:
        The methods 'writeChannel(..)' and 'signalEndOfStream(..)'
        must not be called concurrently.
        """
        self._ss.getOutputChannel(channelName).signalEndOfStream()

    def get(self, key):
        """Return a property of this device

        :param key: as defined in the expected parameter section
        :return: the value of the property
        """
        with self._stateChangeLock:
            try:
                result = self._parameters[key]
                if not self._fullSchema.hasClassId(key):
                    classId = None
                else:
                    classId = self._fullSchema.getClassId(key)
                if classId == KARABO_CLASS_ID_STATE:
                    return State(result)
                elif classId == KARABO_CLASS_ID_ALARM:
                    return AlarmCondition(result)
                elif isinstance(result, (Hash, VectorHash)):
                    # For Hash and VectorHash, 'result' is a reference, so if
                    # it would be returned and the returned object would be
                    # changed, self._parameters would be changed as well, pro-
                    # viding a back door without using self._stateChangeLock!
                    return copy.copy(result)
                else:
                    # Note that vectors of numbers are copies
                    return result
            except RuntimeError as e:
                print(e)
                raise AttributeError(
                    f"Error while retrieving '{key}' from device")

    def __getitem__(self, key):
        """Alternative for `value = self.get(key)`: `value = self[key]`"""
        return self.get(key)

    def getFullSchema(self):
        """Return the full schema describing this device

        :return: a karabo Schema object
        """
        # Have to copy to protect using it while updating
        s = Schema()
        with self._stateChangeLock:
            s.copy(self._fullSchema)
        return s

    def updateSchema(self, schema):
        """Updates the existing device schema
        It merges the schema in argument to the static schema defined in
        expectedParameters, removing any previous schema injections.

        If a property is being reinjected, and of the same type, then it will
        keep its current value. If it does not fit within range, an error will
        be raised.
        Likewise, if the type changes, and the value cannot be cast, an error
        will be raised.

        Input and output channels will be created if injected and removed again
        in case updateSchema is called again without them.
        An output channel is also recreated if its schema changes.
        Note that for newly created input channels there are no data, input
        and end-of-stream handlers registered. This has to be done
        via the corresponding self.KARABO_ON_[DATA|INPUT|EOS] methods.
        If an InputChannel is re-injected, its handlers are kept.

        :param schema: to be merged with the static schema
        """
        rules = ValidatorValidationRules()
        rules.allowAdditionalKeys = True
        rules.allowMissingKeys = True
        rules.allowUnrootedConfiguration = True
        rules.injectDefaults = True
        rules.injectTimestamps = True
        validator = Validator()
        validator.setValidationRules(rules)
        _, _, validated = validator.validate(schema, Hash(),
                                             self.getActualTimestamp())

        with self._stateChangeLock:
            for path in self._injectedSchema.getPaths():
                if not (self._staticSchema.has(path) or schema.has(path)):
                    self._parameters.erasePath(path)
                    # Now we might have removed 'n.m.l.c' completely although
                    # 'n.m' is in static schema - restore (empty) node 'n.m':
                    pathSplit = path.split('.')
                    for i in range(1, len(pathSplit)):
                        p = ".".join(pathSplit[0:-i])  # 'n.m.l', 'n.m', 'n'
                        if (self._staticSchema.has(p)
                                and not self._parameters.has(p)):
                            self._parameters[p] = Hash()
                            # 'n.m' added added back (after 'n.m.l' failed)
                            break

            self._stateDependentSchema.clear()

            prevFullSchemaLeaves = [p for p in self._fullSchema.getPaths()
                                    if not self._fullSchema.isNode(p)]

            # Erase previously present injected InputChannels
            for inChannel in self._ss.getInputChannelNames():
                if self._staticSchema.has(inChannel):
                    # Do not touch static one
                    # (even if re-injected to change properties).
                    continue
                if self._injectedSchema.has(inChannel):
                    self.log.INFO("updateSchema: Remove input channel '"
                                  f"{inChannel}'")
                    self._ss.removeInputChannel(inChannel)
                    if not schema.has(inChannel):
                        # not re-injected - clear handler back-up
                        del self._inputChannelHandlers[inChannel]
            # Treat injected OutputChannels
            outChannelsToRecreate = set()
            for outChannel in self._ss.getOutputChannelNames():
                if self._injectedSchema.has(outChannel):
                    if self._staticSchema.has(outChannel):
                        # Channel changes its schema back to its default
                        outChannelsToRecreate.add(outChannel)
                    else:
                        # Previously injected channel has to be removed
                        self.log.INFO("updateSchema: Remove output channel '"
                                      f"{outChannel}'")
                        self._ss.removeOutputChannel(outChannel)
                if (self._staticSchema.has(outChannel)
                    and schema.has(outChannel)
                    and (not schema.hasClassId(outChannel)
                         or schema.getClassId(outChannel)
                         != "OutputChannel"
                         )):
                    outChannelsToRecreate.add(outChannel)

            self._injectedSchema.copy(schema)
            self._fullSchema.copy(self._staticSchema)
            self._fullSchema += self._injectedSchema

            # notify the distributed system...
            self._ss.emit("signalSchemaUpdated",
                          self._fullSchema, self.deviceid)

            # Keep new leaves only. This hash is then set, to avoid re-sending
            # updates with the same value.
            for path in prevFullSchemaLeaves:
                validated.erasePath(path)

            self._setNoStateLock(validated)

            # Init any freshly injected channels
            self._initChannels(topLevel="", schema=self._injectedSchema)
            # ... and those with potential Schema change
            for outToCreate in outChannelsToRecreate:
                self.log.INFO("updateSchema triggers creation of output "
                              f"channel '{outToCreate}'")
                self._prepareOutputChannel(outToCreate)

        self.log.INFO("Schema updated")

    def appendSchema(self, schema):
        """Append to the existing device schema

        If a property is being reinjected, and of the same type, then it will
        keep its current value. If it does not fit within range, an error will
        be raised.
        Likewise, if the type changes, and the value cannot be cast, an error
        will be raised.

        Input and output channels will be created if injected.
        An output channel is also recreated if its schema changes, to make the
        other end aware.
        Note that for newly created input channels there are no data, input
        and end-of-stream handlers registered. This has to be done
        via the corresponding self.KARABO_ON_[DATA|INPUT|EOS] methods.
        If an InputChannel is re-injected, its handlers are kept.

        :param schema: to append to current full schema
        """
        rules = ValidatorValidationRules()
        rules.allowAdditionalKeys = True
        rules.allowMissingKeys = True
        rules.allowUnrootedConfiguration = True
        rules.injectDefaults = True
        rules.injectTimestamps = True
        validator = Validator()
        validator.setValidationRules(rules)
        _, _, validated = validator.validate(schema, Hash(),
                                             self.getActualTimestamp())

        with self._stateChangeLock:
            # Take care of OutputChannels schema changes
            outChannelsToRecreate = set()
            for path in self._ss.getOutputChannelNames():
                if (self._fullSchema.has(path) and schema.has(path)
                    and (not schema.hasClassId(path) or
                         schema.getClassId(path) != "OutputChannel")):
                    # maybe output schema change without using OUTPUT_CHANNEL
                    outChannelsToRecreate.add(path)
                # elif schema.getClassId(path) == "OutputChannel":
                #      will be recreated by _initChannels(schema) below

            self._stateDependentSchema = {}
            self._injectedSchema += schema

            prevFullSchemaLeaves = [p for p in self._fullSchema.getPaths()
                                    if not self._fullSchema.isNode(p)]
            self._fullSchema.copy(self._staticSchema)
            self._fullSchema += self._injectedSchema

            # notify the distributed system...
            self._ss.emit("signalSchemaUpdated", self._fullSchema,
                          self.deviceid)

            # Keep new leaves only. This hash is then set, to avoid re-sending
            # updates with the same value.
            for path in prevFullSchemaLeaves:
                validated.erasePath(path)
            self._setNoStateLock(validated)

            # Init any freshly injected channels
            self._initChannels(topLevel="", schema=schema)
            # ... and those output channels with potential Schema change
            for outToCreate in outChannelsToRecreate:
                self.log.INFO("updateSchema triggers creation of output "
                              f"channel '{outToCreate}'")
                self._prepareOutputChannel(outToCreate)

        self.log.INFO("Schema appended")

    def appendSchemaMaxSize(self, path, value, emitFlag=True):
        """
        Append Schema to change/set maximum size information for path.
        If paths does not exist, raise KeyError.

        This is similar to the more general appendSchema, but dedicated to a
        common use case.

        :param path  indicates the parameter which should be a
                     Vector- or TableElement
        :param value is the new maximum size of the element
        :param emitFlag indicates if others should be informed about this
                        Schema update. If this method is called for a bunch of
                        paths, it is recommended to set this to True only for
                        the last call.
        """
        with self._stateChangeLock:
            if not self._fullSchema.has(path):
                raise KeyError("Path '{}' not found in the device schema."
                               .format(path))

            self._stateDependentSchema = {}
            # Do not touch static schema - that must be restorable via
            # updateSchema(Schema())
            # OVERWRITE_ELEMENT checks whether max size attribute makes sense
            # for path
            (OVERWRITE_ELEMENT(self._fullSchema).key(path)
             .setNewMaxSize(value).commit(),)
            if self._injectedSchema.has(path):
                (OVERWRITE_ELEMENT(self._injectedSchema).key(path)
                 .setNewMaxSize(value).commit(),)

            if emitFlag:
                self._ss.emit("signalSchemaUpdated",
                              self._fullSchema, self.deviceid)

    def getAliasFromKey(self, key, aliasReferenceType):
        """
        Return the alias of a key
        :param key: to return the alias from
        :param aliasReferenceType: type the alias is of
        :return: an object of aliasReferenceType
        """
        try:
            with self._stateChangeLock:
                return self._fullSchema.getAliasFromKey(key,
                                                        aliasReferenceType)
        except RuntimeError as e:
            raise AttributeError("Error while retrieving alias from parameter"
                                 " ({}): {}".format(key, e))

    def getKeyFromAlias(self, alias):
        """Return the key mapping to a given alias"""
        try:
            with self._stateChangeLock:
                return self._fullSchema.getKeyFromAlias(alias)
        except RuntimeError as e:
            raise AttributeError("Error while retrieving parameter from alias"
                                 " ({}): {}".format(alias, e))

    def aliasHasKey(self, alias):
        """Check if a key for a given alias exists"""
        with self._stateChangeLock:
            return self._fullSchema.aliasHasKey(alias)

    def keyHasAlias(self, key):
        """Check if a given key has an alias defined"""
        with self._stateChangeLock:
            return self._fullSchema.keyHasAlias(key)

    def getValueType(self, key):
        """Get the ValueType of a given key

        :returns: The type in terms of `karabo::util::ReferenceTypes`
        """
        with self._stateChangeLock:
            return self._fullSchema.getValueType(key)

    def getCurrentConfiguration(self, tags=""):
        """Return the current configuration, optionally filtered by tags

        :param tags: a string, with several entries separated by commas
                    spaces or semicolons. Set to an empty string if no
                    filtering is to be applied.
        :return: a configuration Hash
        """
        with self._stateChangeLock:
            if tags == "":
                # Outside the state change lock we need a copy:
                return copy.copy(self._parameters)
            else:
                return HashFilter.byTag(self._fullSchema, self._parameters,
                                        tags, " ,;")

    def getCurrentConfigurationSlice(self, paths):
        """Retrieves a slice of the current configuration.

        :param paths: of the configuration which should be returned
                      (as declared in expectedParameters,
                       method throws if a non-existing path is given)
        :return: Hash with the current values and attributes (e.g. timestamp)
                 of the selected configuration
        """
        result = Hash()
        with self._stateChangeLock:
            for p in paths:
                node = self._parameters.getNode(p)
                # with normal set, type deduction for empty vector may fail
                result.setAs(p, node.getValue(), node.getType())
                newNode = result.getNode(p)
                newNode.setAttributes(node.getAttributes())

        return result

    def filterByTags(self, configuration, tags):
        """Filter a given configuration Hash by tags

        :param configuration:
        :param tags: a string, with several entries separated by commas
                     spaces or semicolons
        :return: the filtered configuration Hash
        """
        with self._stateChangeLock:
            return HashFilter.byTag(self._fullSchema, configuration,
                                    tags, " ,;")

    def getServerId(self):
        """Return the id of the server hosting this devices"""

        return self.serverid

    def getAvailableInstances(self):
        """Return available instances in the distributed system"""

        return self._ss.getAvailableInstances()

    def preReconfigure(self, incomingReconfiguration):
        """
        Use this hook to alter a configuration Hash before it gets applied to
        the device and the distributed system is notified of the change.
        :param incomingReconfiguration:
        """

    def postReconfigure(self):
        """
        Use this hook to react on configuration changes after they have been
        validated and applied to the device, and have been notified to the
        distributed system.
        """

    def preDestruction(self):
        """
        Use this hook if you need to perform clean-up actions before a device
        gets destroyed.
        """

    def initClassId(self):
        self.classid = self.__class__.__classid__

    def initSchema(self):
        self._staticSchema = PythonDevice.getSchema(self.classid)
        self._fullSchema = Schema(self.classid)
        self._fullSchema.copy(self._staticSchema)

    def updateState(self, newState, propertyUpdates=None, timestamp=None):
        """Update the state property of the device to a new state.

        This should be used for NoFSM devices and should *not* be used if you
        have an underlying FSM.
        :param newState: the state to set the device to
        :propertyUpdates: a Hash with further properties to update (or None)
        :timestamp: timestamp to be assigned to the update,
                    if None, use self.getActualTimestamp()
        :return:
        """
        assert isinstance(newState, State)
        stateName = newState.name
        self.log.DEBUG(f"updateState: {stateName}")
        if propertyUpdates is None:
            propertyUpdates = Hash()
        if timestamp is None:
            timestamp = self.getActualTimestamp()

        newStatus = None
        with self._stateChangeLock:
            if self._parameters["state"] != stateName:
                propertyUpdates.set("state", stateName)
                # Validator adds "indicateState" attribute
                if newState is State.ERROR:
                    newStatus = "error"
                elif newState is State.UNKNOWN:
                    newStatus = "unknown"
                else:
                    statuses = ("error", "unknown")
                    if self._ss.getInstanceInfo()["status"] in statuses:
                        newStatus = "ok"

            if propertyUpdates:
                self._setNoStateLock(propertyUpdates, timestamp)

        # Send potential instanceInfo update without state change lock
        if newStatus:
            self._ss.updateInstanceInfo(Hash("status", newStatus))
        # place new state as default reply to interested event initiators
        self._ss.reply(stateName)

    def noStateTransition(self, currentState, currentEvent):
        """
        This function is called if a requested state transition is not allowed
        in the current context. Usually, this means you have an error in your
        state machine.
        """
        self.log.WARN("Device \"{}\" being in state '{}' does not allow the"
                      " transition for event '{}'."
                      .format(self.deviceid, currentState, currentEvent))

    def onTimeUpdate(self, id, sec, frac, period):
        """Called when an update from the time server is received

        :param id: train id
        :param sec: seconds
        :param frac: fractional seconds
        :param period:
        :return:
        """

    def KARABO_SLOT(self, slot, slotName=None, numArgs=None):
        """Register a slot in the distributed system.

        :param slot is the callable to register
        :param slotname is used to call the slot, 'None' means slot.__name__,
                        note that a '_' can also be called as '.' as needed
                        for nested slots in the Schema like 'node.slotInNode'
        :param numArgs number of arguments that the slot has,
                       'None' means to (try to) deduce from 'slot'

        Note that a slot is only connected with a SLOT_ELEMENT if the key of
        the SLOT_ELEMENT matches the slot name provided to this function.

            SLOT_ELEMENT(expected).key("slotDoSomething")

            ....

            self.KARABO_SLOT(slotDoSomething)

            .....

            def slotDoSomething(self):
                pass

        For slots under a node, the method name needs to replace '.' by '_'.

            SLOT_ELEMENT(expected).key("node.slotOther")

            ....

            self.KARABO_SLOT(node_slotOther)

            ....

            def node_slotOther(self):
                pass
        """

        if slotName is None:
            if numArgs is None:
                self._ss.registerSlot(slot)
            else:
                self._ss.registerSlot(slot, numArgs=numArgs)
        elif numArgs is None:
            self._ss.registerSlot(slot, slotName)
        else:
            self._ss.registerSlot(slot, slotName, numArgs)

    def _initDeviceSlots(self):
        # Register intrinsic signals
        # changeHash, instanceId
        self._ss.registerSignal("signalChanged", Hash, str)
        # changeHash, instanceId
        self._ss.registerSystemSignal("signalStateChanged", Hash, str)
        # schema, deviceid
        self._ss.registerSystemSignal("signalSchemaUpdated", Schema, str)

        # Register intrinsic slots
        self._ss.registerSlot(self.slotReconfigure)
        self._ss.registerSlot(self.slotGetConfiguration)
        self._ss.registerSlot(self.slotGetConfigurationSlice)
        self._ss.registerSlot(self.slotGetSchema)
        self._ss.registerSlot(self.slotKillDevice)
        self._ss.registerSlot(self.slotUpdateSchemaAttributes)
        # Timeserver related slots
        self._ss.registerSlot(self.slotTimeTick)
        self._ss.registerSlot(self.slotGetTime)

        self._ss.registerSlot(self.slotLoggerPriority)
        self._ss.registerSlot(self.slotLoggerContent)
        self._ss.registerSlot(self.slotClearLock)

    def initChannels(self, topLevel="", schema=None):
        """
        Initialise Input-/OutputChannels
        :param schema to recurse for channels - if None, use self._fullSchema
        :param topLevel is path in schema hierarchy where to start recursion
        """
        with self._stateChangeLock:
            if schema is None:
                schema = self._fullSchema
            self._initChannels(topLevel, schema)

    def _initChannels(self, topLevel, schema):
        """
        Helper for initChannels, requiring _stateChangeLock protection
        """
        # Keys under topLevel, without leading "topLevel.":
        subKeys = schema.getKeys(topLevel)
        # Now go recursively down the node:
        for subKey in subKeys:
            key = topLevel + '.' + subKey if topLevel else subKey
            if schema.hasClassId(key):
                classId = schema.getClassId(key)
                if classId == "OutputChannel":
                    self._prepareOutputChannel(key)
                elif classId == "InputChannel":
                    self._prepareInputChannel(key)
                else:
                    self.log.DEBUG("Not creating in-/output channel for '"
                                   + key + "' since it's a '"
                                   + classId + "'")
            elif schema.isNode(key):
                # Recursively go down the tree for channels within nodes
                self.log.DEBUG("Looking for input/output channels " +
                               "under node '" + key + "'")
                self._initChannels(key, schema)

    def _prepareOutputChannel(self, path):
        """
        Internal method to create an OutputChannel for given path.
        Needs _stateChangeLock protection
        """
        self.log.INFO(f"Creating output channel '{path}'")
        outputChannel = self._ss.createOutputChannel(
            path, self._parameters)
        if not outputChannel:
            self.log.ERROR(f"Failed to create output channel "
                           f"'{path}'")
        else:
            def connectionsHandler(table):
                with self._stateChangeLock:
                    if self._fullSchema.has(path):
                        self._setNoStateLock(path + ".connections", table)
                    # else might just be removed by self.updateSchema

            outputChannel.registerShowConnectionsHandler(connectionsHandler)

            def statsHandler(bytesRead, bytesWritten):
                with self._stateChangeLock:
                    if self._fullSchema.has(path):
                        h = Hash(path + ".bytesRead", bytesRead,
                                 path + ".bytesWritten", bytesWritten)
                        self._setNoStateLock(h)
                    # else might just be removed by self.updateSchema

            outputChannel.registerShowStatisticsHandler(statsHandler)
            # Publish the resolved address of the output channel
            update = Hash(path, outputChannel.getInitialConfiguration())
            # No lock since this method requires _stateChangeLock protection
            self._setNoStateLock(update, self.getActualTimestamp())

    def _prepareInputChannel(self, path):
        """
        Internal method to create an InputChannel for given path.
        Needs _stateChangeLock protection
        """
        self.log.INFO(f"Creating input channel '{path}'")
        handlers = self._inputChannelHandlers.get(path, [None] * 3)

        def tracker(name, status):
            if (status == ConnectionStatus.CONNECTING
                    or status == ConnectionStatus.DISCONNECTING):
                return  # ignore any intermediate connection status
            updateType = "addIfNotIn"
            if status == ConnectionStatus.CONNECTED:
                updateType = "removeOne"
            self.setVectorUpdate(path + ".missingConnections",
                                 [name], updateType)

        self._ss.createInputChannel(path, self._parameters, handlers[0],
                                    handlers[1], handlers[2], tracker)
        h = Hash(path + ".missingConnections",
                 self._parameters.get(path + ".connectedOutputChannels"))
        self._setNoStateLock(h)

    def KARABO_ON_DATA(self, channelName, handlerPerData):
        """Registers a data handler function

        This function will be called if data is received on an input channel
        identified by `channelName`. The handler function should have the
        signature:

            def onData(data, metaData):
                pass

        where `data` and `metaData` are both Hashes.
        """
        self._inputChannelHandlers.setdefault(channelName, [None] * 3)
        self._inputChannelHandlers[channelName][0] = handlerPerData
        self._ss.registerDataHandler(channelName, handlerPerData)

    def KARABO_ON_INPUT(self, channelName, handlerPerInput):
        """Registers an input handler function

        Registers a handler to be called if data is available on the input
        channel identified by `channelName`. It is up to the device developer
        to read data (in contrast to the `KARABO_ON_DATA` registration).

            def onInput(input):
                for i in range(input.size()):
                    data, metaData = input.read(i)

        Here `input` is a reference to the input channel.
        """
        self._inputChannelHandlers.setdefault(channelName, [None] * 3)
        self._inputChannelHandlers[channelName][1] = handlerPerInput
        self._ss.registerInputHandler(channelName, handlerPerInput)

    def KARABO_ON_EOS(self, channelName, handler):
        """Registers an end-of-stream handler

        Registers a handler to be called if input channel identified by
        `channelName` is signaled end-of-stream.

        The handler function should  have the signature

             def onEos(input):
                 pass

        where `input` is a reference to the input channel.
        """
        self._inputChannelHandlers.setdefault(channelName, [None] * 3)
        self._inputChannelHandlers[channelName][2] = handler
        self._ss.registerEndOfStreamHandler(channelName, handler)

    def execute(self, command, *args):
        if len(args) == 0:
            self._ss.call("", command)
        elif len(args) == 1:
            self._ss.call("", command, args[0])
        elif len(args) == 2:
            self._ss.call("", command, args[0], args[1])
        elif len(args) == 3:
            self._ss.call("", command, args[0], args[1], args[2])
        elif len(args) == 4:
            self._ss.call("", command, args[0], args[1], args[2], args[3])
        else:
            raise AttributeError(
                "Number of command parameters should not exceed 4")

    def slotCallGuard(self, slotName, callee):
        # Check whether the slot is mentioned in the expectedParameters
        # as the call guard only works on those and will ignore all others
        with self._stateChangeLock:
            isSchemaSlot = self._fullSchema.has(slotName)

        # Check whether the slot can be called given the current locking state
        lockableSlot = isSchemaSlot or slotName == "slotReconfigure"
        if self.allowLock() and lockableSlot and slotName != "slotClearLock":
            self._ensureSlotIsValidUnderCurrentLock(slotName, callee)

        if isSchemaSlot:
            with self._stateChangeLock:
                if self._fullSchema.hasAllowedStates(slotName):
                    allowedStates = self._fullSchema.getAllowedStates(slotName)
                    if allowedStates:
                        currentState = State(self._parameters["state"])
                        if currentState not in allowedStates:
                            msg = "Command \"{}\" is not allowed in current " \
                                  "state \"{}\" of device \"{}\""\
                                .format(slotName, currentState.name,
                                        self.deviceid)
                            raise RuntimeError(msg)

        if lockableSlot:
            # Log the call of this slot by setting a parameter of the device
            self.set("lastCommand", slotName + " <- " + callee)

    def allowLock(self):
        """
        Overwrite this function for service devices that cannot be locked
        :return:
        """
        return True

    def slotClearLock(self):
        """ Clear the lock on this device
        """
        self.set("lockedBy", "")

    def slotGetTime(self, info):
        """
        Return the actual time information of this device

        :param info: An empty place holder hash

        This slot returns a Hash with:

        - key ``time`` and the attributes provide an actual
        timestamp with train Id information
        - key ``timeServerId`` to show the configured time server
        - key ``reference`` and the attributes provide the latest
        received timestamp information from the timeserver
        """
        result = Hash()

        result.set("time", True)
        stamp = self.getActualTimestamp()
        stamp.toHashAttributes(result.getAttributes("time"))

        # Provide a nice output for the time server Id
        timeServer = 'None' if not self.timeServerId else self.timeServerId
        result.set("timeServerId", timeServer)

        # And the last reference stamp received
        result.set("reference", True)

        with self._timeLock:
            epoch = Epochstamp(self._timeSec, self._timeFrac)
            train = Trainstamp(self._timeId)
            stamp = Timestamp(epoch, train)

        attrs = result.getAttributes("reference")
        stamp.toHashAttributes(attrs)

        self.reply(result)

    def _ensureSlotIsValidUnderCurrentLock(self, slotName, callee):
        lockHolder = self["lockedBy"]
        if lockHolder:
            msg = "{} is locked by {} and called by {}"
            self.log.DEBUG(msg.format(self.deviceid, lockHolder, callee))
            if callee != "unknown" and callee != lockHolder:
                msg = "Command {} is not allowed as device is locked by {}"
                raise RuntimeError(msg.format(slotName, lockHolder))

    def slotGetConfiguration(self):
        with self._stateChangeLock:
            self._ss.reply(self._parameters, self.deviceid)

    def slotGetConfigurationSlice(self, info):
        paths = info.get("paths")
        cfgSlice = self.getCurrentConfigurationSlice(paths)
        self._ss.reply(cfgSlice)

    def slotReconfigure(self, newConfiguration):
        if newConfiguration.empty():
            return
        result, error, validated = self._validate(newConfiguration)
        if result:
            self.preReconfigure(validated)
            self._applyReconfiguration(validated)
            self.postReconfigure()
        else:
            raise ValueError(error)

    def _validate(self, unvalidated):
        currentState = self["state"]
        whiteList = self._getStateDependentSchema(currentState)
        flag, error, validated = self.validatorExtern.validate(
            whiteList, unvalidated, self.getActualTimestamp())
        return (flag, error, validated)

    def _applyReconfiguration(self, reconfiguration):

        with self._stateChangeLock:
            self._parameters += reconfiguration

        if self.validatorExtern.hasReconfigurableParameter():
            self._ss.emit("signalStateChanged", reconfiguration, self.deviceid)
        else:
            self._ss.emit("signalChanged", reconfiguration, self.deviceid)

    def slotGetSchema(self, onlyCurrentState):
        # state lock!
        if onlyCurrentState:
            currentState = self["state"]
            schema = self._getStateDependentSchema(currentState)
            self._ss.reply(schema, self.deviceid)
        else:
            with self._stateChangeLock:
                self._ss.reply(self._fullSchema, self.deviceid)

    def slotKillDevice(self):
        senderid = self._ss.getSenderInfo(
            "slotKillDevice").getInstanceIdOfSender()
        if senderid == self.serverid and self.serverid != "__none__":
            self.log.INFO("Device is going down as instructed by server")
        else:
            self.log.INFO("Device is going down as instructed by \"{}\""
                          .format(senderid))
            self._ss.call(self.serverid, "slotDeviceGone", self.deviceid)
        try:
            self.preDestruction()
            self.stopFsm()
        except Exception as e:
            # 'repr(e)' to get both, exception type and text
            self.log.WARN(f"Clean-up failed in slotKillDevice: {repr(e)}")
        finally:
            # TODO:
            # Remove this hack if known how to get rid of the object cleanly
            # (slotInstanceGone will be called in _ss destructor again...).
            self._ss.call("*", "slotInstanceGone", self.deviceid,
                          self._ss.getInstanceInfo())

            # This will trigger the central event-loop to finish
            os.kill(os.getpid(), signal.SIGTERM)

    def slotUpdateSchemaAttributes(self, updates):
        success = False

        with self._stateChangeLock:
            success = self._fullSchema.applyRuntimeUpdates(updates)
            # Whenever updating self._fullSchema, we have to clear the cache
            self._stateDependentSchema.clear()

            if success:
                # Once the attributes in the fullSchema have been successfully
                # updated, also perform any required attribute update on the
                # injectedSchema. The update of attributes for the injected
                # Schema is performed on a best effort basis - every path in
                # updates that is found in the injectedSchema will have its
                # attribute updated; those that are not found will be skipped.
                self._injectedSchema.applyRuntimeUpdates(updates)
                # Notify everyone
                self._ss.emit("signalSchemaUpdated", self._fullSchema,
                              self.deviceid)

            self._ss.reply(Hash("success", success,
                                "instanceId", self.deviceid,
                                "updatedSchema", self._fullSchema,
                                "requestedUpdate", updates))

    def slotTimeTick(self, id, sec, frac, period):
        epochNow = Epochstamp()
        with self._timeLock:
            self._timeId = id
            self._timeSec = sec
            self._timeFrac = frac
            # Fallback to the local timing ...
            if sec == 0:
                self._timeSec = epochNow.getSeconds()
                self._timeFrac = epochNow.getFractionalSeconds()
            self._timePeriod = period
        self.onTimeUpdate(id, sec, frac, period)

    def slotLoggerPriority(self, newprio):
        oldprio = Logger.getPriority()
        self.set("Logger.priority", newprio)
        Logger.setPriority(newprio)
        self.log.INFO(
            f"Logger Priority changed : {oldprio} ==> {newprio}")

    def getActualTimestamp(self):
        """Returns the actual timestamp.

        The Trainstamp part of Timestamp is extrapolated from the last values
        received via slotTimeTick (or zero if no time ticks received, i.e.
        timeServerId is empty). To receive time ticks, the server of the device
        has to be connected to a time server.

        :return: the actual timestamp
        """
        return self.getTimestamp(Epochstamp())  # i.e. for now

    def getTimestamp(self, epoch):
        """Returns the Timestamp for given Epochstamp.

        The Trainstamp part of Timestamp is extrapolated forward or backward
        from the last values received via slotTimeTick (or zero if no time
        ticks received yet). To receive time ticks, the server of the device
        has to be connected to a time server.

        :param epoch: Epochstamp for that the time stamp is searched for
        :return: the matching Timestamp, consisting of epoch and the
                 corresponding Trainstamp
        """
        resultId = 0
        with self._timeLock:
            if self._timePeriod > 0:
                epochLastReceived = Epochstamp(self._timeSec, self._timeFrac)
                # duration is always positive, irrespective whether epoch or
                # epochLastReceived is earlier
                duration = epoch.elapsed(epochLastReceived)
                nPeriods = (duration.getTotalSeconds() * 1000000
                            + duration.getFractions(MICROSEC)
                            ) // self._timePeriod
                if epochLastReceived <= epoch:
                    resultId = self._timeId + nPeriods
                elif self._timeId >= nPeriods + 1:  # sanity check
                    resultId = self._timeId - nPeriods - 1
                elif self.log:  # if 'log' is not yet initialised
                    self.log.WARN("Bad input: (train)Id zero since epoch = {};"
                                  " from time server: epoch = {}, id = {},"
                                  " period = {} mus"
                                  .format(epoch.toIso8601(),
                                          epochLastReceived.toIso8601(),
                                          self._timeId, self._timePeriod))
        return Timestamp(epoch, Trainstamp(resultId))

    def _getStateDependentSchema(self, state):
        with self._stateChangeLock:
            if state not in self._stateDependentSchema:
                rules = AssemblyRules(AccessType(WRITE), state.value)
                schemaForState = self._fullSchema.subSchemaByRules(rules)
                self._stateDependentSchema[state] = schemaForState
            return self._stateDependentSchema[state]

    def getInstanceId(self):
        return self._ss.getInstanceId()

    def registerSlot(self, slotFunc):
        self._ss.registerSlot(slotFunc)

    def updateLatencies(self, performanceMeasures):
        if self.get("performanceStatistics.enable"):
            # Keys and values of 'performanceMeasures' are defined in
            # SignalSlotable::updatePerformanceStatistics (C++)
            # and expectedParameters has to foresee this content under node
            # "performanceStatistics".
            self.set(Hash("performanceStatistics", performanceMeasures))

    def onBrokerError(self, message):
        self.log.ERROR(f"Broker consumption problem: {message}")
        # Trigger alarm, but not always a new one (system is busy anyway).
        # By setting messagingProblems up to every second, we can investigate
        # roughly the time of problems via the data logger.
        if (not self["performanceStatistics.messagingProblems"]
                or time.time() - self.lastBrokerErrorStamp >= 1.):
            self["performanceStatistics.messagingProblems"] = True
            self.lastBrokerErrorStamp = time.time()

    def setAlarmCondition(self, condition, **deprecated):
        """Set the global alarm condition

        :param condition: condition to set
        :return: None
        """
        if not isinstance(condition, AlarmCondition):
            raise TypeError("First argument must be 'AlarmCondition',"
                            " not '{}'".format(str(type(condition))))

        timestamp = self.getActualTimestamp()
        with self._stateChangeLock:
            self._setNoStateLock(
                "alarmCondition", condition.asString(),
                timestamp, validate=False)

    def getAlarmCondition(self, key=None, separator="."):
        if key is None:
            return AlarmCondition.fromString(self.get("alarmCondition"))
        else:
            with self._stateChangeLock:
                condition = self._parameters.getAttribute(
                    key, "alarmCondition", separator)
                return AlarmCondition.fromString(condition)

    def hasRollingStatistics(self, key):
        with self._stateChangeLock:
            return self._fullSchema.hasRollingStatistics(key)

    def getRollingStatistics(self, key):
        with self._stateChangeLock:
            # TODO
            # I fear we have to copy here, since 'getRollingStatistics is
            # defined as 'bp::return_internal_reference<>()' in PyUtilSChema.cc
            return self.validatorIntern.getRollingStatistics(key)

    @staticmethod
    def loadConfiguration(cfgFile):
        cfg = loadFromFile(cfgFile)
        os.remove(cfgFile)
        return cfg

    # the following functions expose parts of SignalSlotable to the public
    # device interface.

    def registerSignal(self, signalName, *args):
        """Register a signal to be handles in the remote system

        :param signalName:name of the signal to be registered
        :param args: signature of the signal, e.g. `str, Hash, str`
        """
        self._ss.registerSignal(signalName, *args)

    def connect(self, signalInstance, signalName, slotInstance, slotName):
        """Connect a signal with a slot

        :param signalInstance: instance the signal is on, use "" for local
        :param signalName: name of the signal to connect
        :param slotInstance: instance the slot is on, use "" for local
        :param slotName: name of the slot to be executed upon signal reception
        :return whether connection could be established
        """
        return self._ss.connect(signalInstance, signalName,
                                slotInstance, slotName)

    def reply(self, *args):
        """Place the reply of a slot being called

        Reply content will not be sent immediately, but when the slot call
        ends. If called more than once, the last call defines the slot reply.

        :param args: list of arguments to reply, maximum length is 4
        """
        self._ss.reply(*args)

    def emit(self, signalName, *args):
        """Emit a signal to the remote system

        :param signalName: name of the signal.
        :param args: list of arguments signal is emitted with. Maximum 4
        """
        self._ss.emit(signalName, *args)

    def call(self, instanceId, slotName, *args):
        """Call a remote slot with arguments

        :param instanceId: instance of the remote device to call slot on
        :param slotName: name of the slot to call on instanceId
        :param args: list of arguments to call slot with, maximum length is 4
        """
        self._ss.call(instanceId, slotName, *args)

    def request(self, instanceId, slotName, *args):
        """Request a reply from a remote slot

        :param instanceId: instance of the remote device to request from
        :param slotName: name of the slot to request from on instanceId
        :param args: list of arguments to call slot with, maximum length is 4
        :return: a `SignalSlotable.Requestor` object handling the reply
        """
        return self._ss.request(instanceId, slotName, *args)

    def requestNoWait(self, instanceId, slotName, replyInstance,
                      replySlotName, *args):
        """Request a reply from a remote slot

        :param instanceId: instance of the remote device to request from
        :param slotName: name of the slot to request from on instanceId
        :param replyInstance: instance on which to handle reply, use "" for
                              local device.
        :param replySlotName: slot to call with reply on replyInstance
        :param args: list of arguments to call slot with, maximum length is 4
        :return: a `SignalSlotable.Requestor` object handling the reply
        """
        return self._ss.requestNoWait(instanceId, slotName, replyInstance,
                                      replySlotName, *args)

    # Added for backward compatibility when fullSchema => _fullSchema
    @property
    @karabo_deprecated
    def fullSchema(self):
        """
        DEPRECATED - use getFullSchema()
        """
        return self.getFullSchema()

    # Added for backward compatibility when parameters => _parameters
    @property
    @karabo_deprecated
    def parameters(self):
        """
        DEPRECATED
        * for full config use getCurrentConfiguration()
        * maybe what you want is just some key, then use get(some_key)
        """
        return self.getCurrentConfiguration()


def launchPythonDevice():
    from .plugin_loader import DEFAULT_NAMESPACE, PluginLoader

    # NOTE: The first argument is '-c'
    _, modname, classid, cfgFile = tuple(sys.argv)

    config = PythonDevice.loadConfiguration(cfgFile)
    if '_connection_' in config:
        # Inject broker connection parameters into PythonDevice class, so
        # all possible instances share the same broker configuration
        PythonDevice.connectionParams = copy.copy(config['_connection_'])
        # Clean _connection_ to validate input configuration
        config.erase('_connection_')
    if 'timeServerId' in config:
        # Inject timeServerId from the server
        PythonDevice.timeServerId = copy.copy(config['timeServerId'])
        config.erase('timeServerId')
    if '_logger_' in config:
        # Also most logger settings are taken from server and not configurable:
        PythonDevice._loggerCfg = copy.copy(config['_logger_'])
        config.erase('_logger_')
    namespace = DEFAULT_NAMESPACE
    if '_pluginNamespace_' in config:
        # get the namespace from the server if present
        namespace = str(config['_pluginNamespace_'])
        config.erase('_pluginNamespace_')

    loader = PluginLoader.create(
        "PythonPluginLoader", Hash("pluginNamespace", namespace))
    loader.update()

    # Load the module containing classid so that it gets registered.
    entrypoint = loader.getPlugin(modname)
    deviceClass = entrypoint.load()
    assert deviceClass.__classid__ == classid

    device = None
    exception = None
    serverId = config["_serverId_"]
    deviceId = config["_deviceId_"]

    def initialize():
        nonlocal device, exception
        try:
            # Create device and prepare its initialisation
            try:
                device = Configurator(PythonDevice).create(classid, config)
            except Exception as e:
                # create a signal slotable to act as a flight data recorder
                # to inform the server that an exception occurred
                # during __init__
                fdr = SignalSlotable(deviceId)
                fdr.start()
                fdr.call(
                    serverId, "slotDeviceUp", deviceId, False, str(e))
                raise e
            device._finalizeInternalInitialization()
        except Exception as e:
            EventLoop.stop()
            exception = e

    EventLoop.post(initialize)

    # Start the event loop that will first call initialize()
    EventLoop.work()  # Blocks until loop is stopped

    # Finish off - either exception from initialize or clean shutdown
    if exception:
        # from None: no "During handling of the above exception, another..."
        raise exception from None
    else:
        print(f"'{device.getInstanceId()}' has cleanly exited!\n")
