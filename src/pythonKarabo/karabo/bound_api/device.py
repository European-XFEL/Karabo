__author__ = "Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ = "$Jul 30, 2012 9:03:51 PM$"

import threading
import os
import time
import sys
import socket
import re
import signal
import traceback

from karathon import (
    ALARM_ELEMENT, BOOL_ELEMENT, CHOICE_ELEMENT, FLOAT_ELEMENT, INT32_ELEMENT,
    UINT32_ELEMENT, MICROSEC, NODE_ELEMENT, OVERWRITE_ELEMENT, SLOT_ELEMENT,
    STATE_ELEMENT, STRING_ELEMENT,
    OBSERVER, READ, WRITE, INIT,
    AccessLevel, AccessType, AssemblyRules, ChannelMetaData, JmsConnection,
    EventLoop, Epochstamp, Hash, HashFilter, HashMergePolicy,
    LeafType, loadFromFile, Logger, MetricPrefix, Priority,
    Schema, SignalSlotable, Timestamp, Trainstamp, Unit, Validator,
    ValidatorValidationRules
)
from karabo.common.api import (AlarmCondition, Capabilities, State,
                               karabo_deprecated)
# Use patched DeviceClient, not the one directly from karathon:
from .device_client import DeviceClient
from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from .configurator import Configurator
from .no_fsm import NoFsm


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("PythonDevice", "1.0")
class PythonDevice(NoFsm):
    """
    The PythonDevice class is the basis for all karabo.bound devices

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

    @staticmethod
    def expectedParameters(expected):
        (
            STRING_ELEMENT(expected).key("_serverId_")
                    .displayedName("_ServerID_")
                    .description("Do not set this property, it will be"
                                 " set by the device-server")
                    .expertAccess().assignmentInternal().noDefaultValue().init()
                    .commit(),

            STRING_ELEMENT(expected).key("_deviceId_")
                    .displayedName("_DeviceID_")
                    .description("Do not set this property, it will be set"
                                 " by the device-server")
                    .expertAccess().assignmentInternal().noDefaultValue().init()
                    .commit(),

            NODE_ELEMENT(expected).key("_connection_")
                    .displayedName("Connection")
                    .description("The connection to the communication layer"
                                 " of the distributed system")
                    .appendParametersOf(JmsConnection)
                    .adminAccess()
                    .commit(),

            INT32_ELEMENT(expected).key("visibility")
                    .displayedName("Visibility")
                    .description("Configures who is allowed to see this "
                                 "device at all")
                    .assignmentOptional().defaultValue(AccessLevel(OBSERVER))
                    .expertAccess().reconfigurable()
                    .commit(),

            STRING_ELEMENT(expected).key("deviceId")
                    .displayedName("DeviceID")
                    .description("The device instance ID uniquely identifies"
                                 " a device instance in the distributed system")
                    .readOnly()
                    .commit(),

            STRING_ELEMENT(expected).key("classId")
                    .displayedName("ClassID")
                    .description("The (factory)-name of the class of"
                                 " this device")
                    .expertAccess()
                    .readOnly().initialValue(PythonDevice.__classid__)
                    .commit(),

            STRING_ELEMENT(expected).key("classVersion")
                    .displayedName("Class version")
                    .description("The version of the class of this device defined in KARABO_CLASSINFO")
                    .expertAccess()
                    .readOnly().initialValue(PythonDevice.__version__)
                    .commit(),

            STRING_ELEMENT(expected).key("serverId")
                    .displayedName("ServerID")
                    .description("The device-server on which this device"
                                 " is running on")
                    .expertAccess().readOnly()
                    .commit(),

            STRING_ELEMENT(expected).key("hostName")
                    .displayedName("Host")
                    .description("The name of the host where this device runs")
                    .expertAccess()
                    .readOnly()
                    .commit(),

            INT32_ELEMENT(expected).key("pid")
                    .displayedName("Process ID")
                    .description("The unix process ID of the device")
                    .expertAccess().readOnly().initialValue(0)
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
                        .description("The current alarm condition of the "
                                     "device. Evaluates to the highest"
                                     " condition on any property if not set"
                                     " manually.")
                        .initialValue(AlarmCondition.NONE)
                        .commit(),

            STRING_ELEMENT(expected).key("lockedBy")
                        .displayedName("Locked by")
                        .reconfigurable()
                        .assignmentOptional().defaultValue("")
                        .expertAccess()
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

            BOOL_ELEMENT(expected).key("archive")
                        .displayedName("Archive")
                        .description("Decides whether the properties of this"
                                     " device will be logged or not")
                        .reconfigurable()
                        .expertAccess()
                        .assignmentOptional().defaultValue(True)
                        .commit(),

            # This parameter is obsolete and should be removed.  Replaced by "timeServerId"
            BOOL_ELEMENT(expected).key("useTimeserver")
                        .displayedName("Use Timeserver")
                        .description("Decides whether to use time and train ID"
                                     " from TimeServer device")
                        .init()
                        .expertAccess()
                        .assignmentOptional().defaultValue(False)
                        .commit(),

            STRING_ELEMENT(expected).key("timeServerId")
                    .displayedName("TimeServer ID")
                    .description("The instance id uniquely identifies a TimeServer instance in the distributed system")
                    .assignmentOptional().defaultValue("")
                    .commit(),

            INT32_ELEMENT(expected).key("progress")
                    .displayedName("Progress")
                    .description("The progress of the current action")
                    .readOnly().initialValue(0).commit(),

            NODE_ELEMENT(expected).key("performanceStatistics")
                    .displayedName("Performance Statistics")
                    .description("Accumulates some statistics")
                    .expertAccess()
                    .commit(),

            BOOL_ELEMENT(expected).key("performanceStatistics."
                                       "messagingProblems")
                    .displayedName("Messaging problems")
                    .description("If true, there is a problem consuming "
                                 "broker messages")
                    .expertAccess()
                    .readOnly().initialValue(False)
                    # Alarm commented as long as it dominates alarm handling:
                    ## Threshold is exclusive: value True fulfils "> False"
                    ## and triggers alarm whereas False does not:
                    #.alarmHigh(False)
                    #.info("Unreliable broker message consumption - "
                    #      "consider restarting device!")
                    #.needsAcknowledging(True)
                    .commit(),

            BOOL_ELEMENT(expected).key("performanceStatistics.enable")
                    .displayedName("Enable Performance Indicators")
                    .description("Enables some statistics to follow the"
                                 " performance of an individual device")
                    .reconfigurable()
                    .expertAccess()
                    .assignmentOptional().defaultValue(False)
                    .commit(),

            FLOAT_ELEMENT(expected).key("performanceStatistics.processingLatency")
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
                     .description("Number of messages received within "
                                  "averaging interval.")
                     .unit(Unit.COUNT)
                     .expertAccess()
                     .readOnly().initialValue(0)
                     .commit(),

            UINT32_ELEMENT(expected).key("performanceStatistics"
                                         ".maxEventLoopLatency")
                     .displayedName("Max. event loop latency")
                     .description("Maximum time interval between posting a "
                                  "message on the central event loop and "
                                  "processing it within averaging interval.")
                     .unit(Unit.SECOND).metricPrefix(MetricPrefix.MILLI)
                     .expertAccess()
                     .readOnly().initialValue(0)
                     .commit(),

            NODE_ELEMENT(expected).key("Logger")
                    .description("Logging settings")
                    .displayedName("Logger")
                    .appendParametersOf(Logger)
                    .expertAccess()
                    .commit(),

        )

    log = None  # make always available, at least as None

    def __init__(self, configuration):
        """
        The initialization method of a device expects a configuration passed
        as a Karabo Hash.

        :param configuration: the configuration Hash. It may contain the
               following entries:

               - _serverId_: there id of the hosting server
               - _deviceId_: a usually autogenerated device id
        """
        if configuration is None:
            raise ValueError("Configuration must be Hash object, not None")

        configuration.set("hostName", socket.gethostname().partition('.')[0])
        super(PythonDevice, self).__init__(configuration)

        self.parameters = configuration
        if "_serverId_" in self.parameters:
            self.serverid = self.parameters["_serverId_"]
        else:
            self.serverid = "__none__"

        if "_deviceId_" in self.parameters:
            self.deviceid = self.parameters["_deviceId_"]
        else:
            self.deviceid = "__none__"    #TODO: generate uuid

        # Initialize threading locks...
        self._lock = threading.Lock()
        self._stateChangeLock = threading.Lock()
        self._stateDependentSchemaLock = threading.Lock()
        self._stateDependentSchema = {}
        self._injectedSchema = Schema()

        # Initialize _client to None (important!)
        self._client = None

        # host & domain names
        self.hostname, dotsep, self.domainname = socket.gethostname()\
                                                       .partition('.')

        # timeserver related
        self._timeLock = threading.Lock()
        self._timeId = 0
        self._timeSec = 0
        self._timeFrac = 0
        self._timePeriod = 0
        self.timeServerId = self.parameters["timeServerId"]

        # Setup the validation classes
        self.validatorIntern   = Validator()
        self.validatorExtern   = Validator()
        rules = ValidatorValidationRules()
        rules.allowAdditionalKeys = True
        rules.allowMissingKeys    = True
        rules.allowUnrootedConfiguration = True
        rules.injectDefaults = False
        rules.injectTimestamps = True
        self.validatorIntern.setValidationRules(rules)
        self.validatorExtern.setValidationRules(rules)
        self.globalAlarmCondition = AlarmCondition.NONE
        self.accumulatedGlobalAlarms = set()

        # For broker error handler
        self.lastBrokerErrorStamp = 0

        # Initialize regular expression object
        self.errorRegex = re.compile(".*error.*", re.IGNORECASE)

        self.initClassId()
        self.initSchema()

        with self._stateChangeLock:
            self.parameters.set("classId", self.classid)
            self.parameters.set("deviceId", self.deviceid)
            self.parameters.set("serverId", self.serverid)

            # Validate first time to assign timestamps
            validated = self.validatorIntern.validate(self.fullSchema,
                                                      self.parameters,
                                                      self.getActualTimestamp())
            self.parameters.merge(validated, HashMergePolicy.REPLACE_ATTRIBUTES)

        # Create 'info' hash
        info = Hash("type", "device")
        info["classId"] = self.classid
        info["serverId"] = self.serverid
        info["visibility"] = self["visibility"]
        info["compatibility"] = self.__class__.__version__
        info["host"] = self.hostname
        info["status"] = "ok"
        info["archive"] = self.get("archive")

        # device capabilities are encoded in a bit mask field
        capabilities = 0
        if configuration.has("availableScenes"):
            capabilities |= Capabilities.PROVIDES_SCENES
        info["capabilities"] = capabilities

        # Instantiate SignalSlotable object
        self._ss = SignalSlotable(self.deviceid, "JmsConnection",
                                  self.parameters["_connection_"], 20, info)

        # Setup device logger (needs self._ss and self.parameters)
        self.loadLogger()
        self.log = Logger.getCategory(self.deviceid)

        # Initialize FSM slots if defined
        if hasattr(self, 'initFsmSlots'):
            self.initFsmSlots(self._ss)

        # Initialize Device slots and instantiate all channels
        self._initDeviceSlots()
        self.initChannels()

        # Register guard for slot calls
        self._ss.registerSlotCallGuardHandler(self.slotCallGuard)

        # Register updateLatencies handler
        self._ss.registerPerformanceStatisticsHandler(self.updateLatencies)

        self._ss.registerBrokerErrorHandler(self.onBrokerError)


    def _finalizeInternalInitialization(self):
        # Start - after all settings/registrations done:
        # Communication (incl. system registration) starts and thus parallelism!
        # This is done here and not yet in __init__ to be sure that inheriting
        # devices can register in their __init__ after super(..).__init__(..)
        try:
            self._ss.start()
        except RuntimeError as e:
            raise RuntimeError("PythonDevice.__init__: "
                               "SignalSlotable Exception -- {0}".format(str(e)))

        pid = os.getpid()
        self.log.INFO("'{0.classid}' with deviceId '{0.deviceid}' got started "
                      "on server '{0.serverid}', pid '{1}'.".format(self, pid))

        # Connect input channels
        self._ss.connectInputChannels()

        self.startFsm()

        self.set("pid", pid)

        if self.timeServerId:
            self.log.DEBUG("Connecting to time server : \"{}\"".format(self.timeServerId))
            self._ss.connect(self.timeServerId, "signalTimeTick",
                             "", "slotTimeTick")


    @property
    def signalSlotable(self):
        """Get SignalSlotable object embedded in PythonDevice instance."""
        return self._ss

    def loadLogger(self):
        """
        Load the distributed logger using config in self.parameters["Logger"]
        """
        config = self.parameters["Logger"]
        stamp = self.getActualTimestamp()

        # cure the network part of the logger config
        topicPath = "network.topic"
        if config.get(topicPath, default="") == "":
            # If not specified or empty, use the local topic for log messages
            config.set(topicPath, self._ss.getTopic())
            # Since manipulating self.parameters, add timestamp:
            topicAttrs = config.getNode(topicPath).getAttributes()
            stamp.toHashAttributes(topicAttrs)

        # cure the file part of the logger config
        path = os.path.join(os.environ['KARABO'], "var", "log", self.serverid,
                            self.deviceid)
        if not os.path.isdir(path):
            os.makedirs(path)
        path = os.path.join(path, 'device.log')
        config.set('file.filename', path)
        # Since manipulating self.parameters, add timestamp:
        pathAttrs = config.getNode('file.filename').getAttributes()
        stamp.toHashAttributes(pathAttrs)

        # finally configure the logger
        Logger.configure(config)
        Logger.useOstream()
        Logger.useFile()
        Logger.useNetwork()
        Logger.useOstream("karabo", False)
        Logger.useFile("karabo", False)


    def __del__(self):
        """ PythonDevice destructor """
        os._exit(0)

    def remote(self):
        """
        Return a DeviceClient instance. The DeviceClient will use this device's
        SignalSlotable to interact with the distributed system
        :return:
        """
        if self._client is None:
            # SignalSlotable object for reuse
            self._client = DeviceClient(self._ss)
        return self._client

    def set(self, *args, **kwargs):
        """
        Updates the state of the device. This function automatically
        notifies any observers.

        args: can be of length

            * one: expects a hash, and uses current timestamp
            * two: expects a key, value pair and uses current timestamp or a
                   hash, timestamp pair
            * three: expects key, value and timestamp

        kwargs: validate: specifies if validation of args should be performed
                before notification.


        """
        pars = tuple(args)
        validate = kwargs.get("validate", True)

        with self._stateChangeLock:
            if len(pars) == 0 or len(pars) > 3:
                raise SyntaxError("Number of parameters is wrong: "
                                  "from 1 to 3 arguments are allowed.")

            # key, value, timestamp args
            if len(pars) == 3:
                key, value, stamp = pars

                if not isinstance(stamp, Timestamp):
                    raise TypeError("The 3rd argument should be Timestamp")

                h = Hash()
                # assure we are allowed to set states and alarms to
                # appropriate elements
                if isinstance(value, State):
                    h.set(key, value.name)
                    h.setAttribute(key, "indicateState", True)
                elif isinstance(value, AlarmCondition):
                    h.set(key, value.asString())
                    h.setAttribute(key, "indicateAlarm", True)
                else:
                    h.set(key, value)
                pars = tuple([h, stamp])

            
            # hash args
            if len(pars) == 1:
                h = pars[0]
                if not isinstance(h, Hash):
                    raise TypeError("The only argument should be a Hash")
                pars = tuple([h, self.getActualTimestamp()])   # add timestamp
            
            # key, value or hash, timestamp args
            if len(pars) == 2:
                if not isinstance(pars[0], Hash):
                    key, value = pars

                    h = Hash()
                    if isinstance(value, State):
                        h.set(key, value.name)
                        h.setAttribute(key, "indicateState", True)
                    elif isinstance(value, AlarmCondition):
                        h.set(key, value.asString())
                        h.setAttribute(key, "indicateAlarm", True)
                    else:
                        h.set(key, value)
                    pars = tuple([h, self.getActualTimestamp()])

                hash, stamp = pars

                validated = None

                prevAlarmParams = self.validatorIntern \
                    .getParametersInWarnOrAlarm()

                if validate:
                    validated = self.validatorIntern.validate(self.fullSchema,
                                                              hash,
                                                              stamp)
                    resultingCondition = self._evaluateAndUpdateAlarmCondition(
                        forceUpdate=not prevAlarmParams.empty(),
                        prevParamsInAlarm=prevAlarmParams, silent=False)
                    # set the overal alarm condition if needed
                    if (resultingCondition is not None
                        and resultingCondition.asString()
                        != self.parameters.get("alarmCondition")):
                            validated.set("alarmCondition",
                                          resultingCondition.asString())
                            node = validated.getNode("alarmCondition")
                            attributes = node.getAttributes()
                            stamp.toHashAttributes(attributes)
                    changedAlarms = self._evaluateAlarmUpdates(prevAlarmParams)

                    if not changedAlarms.get("toClear").empty() or not \
                            changedAlarms.get("toAdd").empty():
                        self._ss.emit("signalAlarmUpdate", self.getInstanceId(),
                                      changedAlarms)
                    
                else:
                    validated = hash
                    # Add timestamps
                    for path in validated.getPaths():
                        node = validated.getNode(path)
                        attributes = node.getAttributes()
                        stamp.toHashAttributes(attributes)

                if not validated.empty():
                    self.parameters.merge(validated, HashMergePolicy.REPLACE_ATTRIBUTES)

                    # Hash containing 'state' should be signalled by 'signalStateChanged'
                    if 'state' in validated:
                        self._ss.emit("signalStateChanged", validated, self.deviceid)
                        return;

                    # if at least one key is reconfigurable -> signalStateChanged, also validation was performed
                    if validate and self.validatorIntern.hasReconfigurableParameter():
                        self._ss.emit("signalStateChanged", validated, self.deviceid)
                        return

                    self._ss.emit("signalChanged", validated, self.deviceid)
                    

    def _evaluateAndUpdateAlarmCondition(self, forceUpdate, prevParamsInAlarm,
                                         silent):
        """
        Evaluate the device's alarm condition as the most severe alarm condition
        present on its properties or explicitly set via `setAlarmCondition`.
        :param forceUpdate: if set to true  the global alarm condition will
                            always be returned, even if no parameter based
                            alarm conditions are present.
        :param prevParamsInAlarm: parameters in alarm before current update,
                                  needed only if silent=False
        :silent: If True, suppress any log messages about changed alarms
        :return:
        """
        if self.validatorIntern.hasParametersInWarnOrAlarm():
            warnings = self.validatorIntern.getParametersInWarnOrAlarm()
            conditions = [self.globalAlarmCondition]

            for node in warnings:
                desc = warnings[node]
                key = node.getKey()
                alarmType = desc["type"]
                if (not silent and
                    (key not in prevParamsInAlarm
                     or prevParamsInAlarm.get(key + ".type") != alarmType)):
                    # A new alarm - so it is worth to notify the sysyem:
                    self.log.WARN("{}: {}".format(alarmType, desc["message"]))
                conditions.append(AlarmCondition.fromString(alarmType))

            mostSignificantCondition = AlarmCondition\
                                        .returnMostSignificant(conditions)
            return mostSignificantCondition
        elif forceUpdate:
            return self.globalAlarmCondition
        else:
            return None

    def _evaluateAlarmUpdates(self, previous, forceUpdate=False):
        """
        Evaluates difference between previous and current parameter in alarm
        conditions and emits a signal with the update
        :param previous: alarm conditions previously present on the device.
        :param forceUpdate: force updating alarms even if no change occurred
               on validator side.
        :return: a Hash containing the alarm changes

        Calling this method must be protected by a state change lock!
        """
        toClear = Hash()
        toAdd = Hash()
        knownAlarms = set()

        current = self.validatorIntern.getParametersInWarnOrAlarm()
        # Check if we need to clear/clean alarms
        for p in previous:
            pKey = p.getKey()
            currentEntry = current.find(pKey)
            desc = p.getValue()
            exType = desc.get("type")
            if (currentEntry is not None and
                    exType == currentEntry.getValue().get("type")):
                timeStampPrevious = Timestamp.fromHashAttributes(
                    p.getAttributes())
                timeStampCurrent = Timestamp.fromHashAttributes(
                    currentEntry.getAttributes())
                if not forceUpdate and timeStampPrevious == timeStampCurrent:
                    knownAlarms.add(pKey)

                # alarmCondition still exists nothing to clean
                continue

            # alarm is gone: we should clean
            existingEntries = []
            existingEntyNode = toClear.find(pKey)
            if existingEntyNode is not None:
                existingEntries = existingEntyNode.getValue()

            existingEntries.append(exType)
            toClear.set(pKey, existingEntries)

        # Add new alarms
        for c in current:
            cKey = c.getKey()
            desc = c.getValue()
            conditionString = desc.get("type")
            # avoid unnecessary chatter of already sent messages
            if not forceUpdate and cKey in knownAlarms:
                continue

            condition = AlarmCondition(conditionString)
            pSep = cKey.replace(Validator.kAlarmParamPathSeparator, ".")

            alarmDesc = self.getFullSchema().getInfoForAlarm(pSep, condition)
            needAck = self.getFullSchema().doesAlarmNeedAcknowledging(
                pSep, condition)

            entry = Hash("type", conditionString,
                         "description", alarmDesc,
                         "needsAcknowledging", needAck)

            prop = Hash(conditionString, entry)
            entryNode = prop.getNode(conditionString)
            occuredAt = Timestamp.fromHashAttributes(c.getAttributes())
            occuredAt.toHashAttributes(entryNode.getAttributes())
            toAdd.set(cKey, prop)

        return Hash("toClear", toClear, "toAdd", toAdd)

    def slotReSubmitAlarms(self, existingAlarms):
        """
        This slot is called by the alarm service when it gets  (re-)
        instantiated. The alarm service will pass any for this instances that
        it recovered from its persisted data. These should be checked
        against whether they are still valid and if new ones appeared
        :param existingAlarms: A hash containing existing alarms pertinent to
               this device. May be empty
        """
        with self._stateChangeLock:
            alarmsToUpdate = self._evaluateAlarmUpdates(existingAlarms,
                                                        forceUpdate=True)

        self._ss.reply(self.getInstanceId(), alarmsToUpdate)


    
    def __setitem__(self, key, value):
        """
        Alternative to `self.set`: `self[key] = value`
        The timestamp is set to the current timestamp
        """

        self.set(key, value, self.getActualTimestamp())
        
    def writeChannel(self, channelName, data, timestamp=None):
        """
        Write data to an output channel.
        :param channelName: name given to an OUTPUT_CHANNEL in
         expectedParameters
        :param data: a Hash with keys as described in the Schema of the
         channel
        :param timestamp: optional timestamp; if none is given, the current
         timestamp is used

        Example for an output channel sending an image (key: "image") and
        a frame number (key: "frame"):

        imgArray = numpy.array(...)
        self.writeChannel("output", Hash("image", ImageData(imgArray),
                                         "frame", frameNumber))
        """

        channel = self._ss.getOutputChannel(channelName)
        sourceName = "{}:{}".format(self.getInstanceId(), channelName)
        if not timestamp:
            timestamp = self.getActualTimestamp()
        meta = ChannelMetaData(sourceName, timestamp)
        channel.write(data, meta)
        channel.update()

    def signalEndOfStream(self, channelName):
        """
        Signal an end-of-stream event on the channel identified by
        `channnelName`
        """

        self._ss.getOutputChannel(channelName).signalEndOfStream()
        
    def get(self, key):
        """
        Return a property of this device
        :param key: as defined in the expected parameter section
        :return: the value of the property
        """
        with self._stateChangeLock:
            try:
                leafType = None if not self.fullSchema.getParameterHash().hasAttribute(key, "leafType") \
                    else self.fullSchema.getParameterHash().getAttribute(key, "leafType")

                if leafType == LeafType.STATE:
                    return State(self.parameters[key])
                elif leafType == LeafType.ALARM_CONDITION:
                    return AlarmCondition(self.parameters[key])
                else:
                    return self.parameters[key]
            except RuntimeError as e:
                print(e)
                raise AttributeError(
                    "Error while retrieving '{}' from device".format(key))

    def __getitem__(self, key):
        """
        Alternative for `value = self.get(key)`: `value = self[key]`
        """

        return self.get(key)
    
    def getFullSchema(self):
        """
        Return the full schema describing this device
        :return: a karabo Schema object
        """

        return self.fullSchema
        
    def updateSchema(self, schema):
        """
        Update the existing device schema by merging the argument to the static
        schema defined in expectedParameters.

        :param schema: to be merged with the static schema
        """

        rules = ValidatorValidationRules()
        rules.allowAdditionalKeys        = True
        rules.allowMissingKeys           = True
        rules.allowUnrootedConfiguration = True
        rules.injectDefaults             = True
        rules.injectTimestamps           = True
        validator = Validator()
        validator.setValidationRules(rules)
        validated = validator.validate(schema, Hash(), self.getActualTimestamp())
        with self._stateChangeLock:
            for path in self._injectedSchema.getPaths():
                if self.parameters.has(path) and not self.staticSchema.has(path): 
                    self.parameters.erase(path)
            self._stateDependentSchema = {}
            self._injectedSchema.copy(schema)
            self.fullSchema.copy(self.staticSchema)
            self.fullSchema += self._injectedSchema
            self.fullSchema.updateAliasMap()
        # notify the distributed system...
        self._ss.emit("signalSchemaUpdated", self.fullSchema, self.deviceid)
        self.set(validated)
        self.log.INFO("Schema updated")
    
    def appendSchema(self, schema):
        """
        Append to the existing device schema

        :param schema: to append
        """

        rules = ValidatorValidationRules()
        rules.allowAdditionalKeys        = True
        rules.allowMissingKeys           = True
        rules.allowUnrootedConfiguration = True
        rules.injectDefaults             = True
        rules.injectTimestamps           = True
        validator = Validator()
        validator.setValidationRules(rules)
        validated = validator.validate(schema, self.parameters, self.getActualTimestamp())
        with self._stateChangeLock:
            for key in self._injectedSchema.getKeys():
                self.parameters.erase(key)
            self._stateDependentSchema = {}
            self._injectedSchema += schema
            self.fullSchema.copy(self.staticSchema)
            self.fullSchema += self._injectedSchema
            self.parameters.merge(validated, HashMergePolicy.REPLACE_ATTRIBUTES)
            self.fullSchema.updateAliasMap()
        # notify the distributed system...
        self._ss.emit("signalSchemaUpdated", self.fullSchema, self.deviceid)
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
            if not self.fullSchema.has(path):
                raise KeyError("Path '{}' not found in the device schema."
                               .format(path))

            self._stateDependentSchema = {}
            # Do not touch static schema - that must be restorable via
            # updateSchema(Schema())
            # OVERWRITE_ELEMENT checks whether max size attribute makes sense
            # for path
            (OVERWRITE_ELEMENT(self.fullSchema).key(path)
             .setNewMaxSize(value).commit(),)
            if self._injectedSchema.has(path):
                (OVERWRITE_ELEMENT(self._injectedSchema).key(path)
                 .setNewMaxSize(value).commit(),)

            if emitFlag:
                self._ss.emit("signalSchemaUpdated",
                              self.fullSchema, self.deviceid)

    def setProgress(self, value, associatedText = ""):
        """
        Set progress indicator on this device, use this for processing on
        devices, or a slow hardware related process.
        :param value:
        :param associatedText: optionally set a text describing the progress
        :return:
        """

        v = self.progressMin + value / (self.progressMax - self.progressMin)
        self.set("progress", v)

    def resetProgress(self):
        """
        Reset progress to the minimum progress range.
        :return:
        """

        set("progress", self.progressMin)

    def setProgressRange(self, minimum, maximum):
        """
        Set the range used for progress indication.
        :param minimum: lower bound of value used in `setProgress`
        :param maximum: upper bound of value used in `setProgress`
        :return:
        """

        self.progressMin, self.progressMax = minimum, maximum

    def getAliasFromKey(self, key, aliasReferenceType):
        """
        Return the alias of a key
        :param key: to return the alias from
        :param aliasReferenceType: type the alias is of
        :return: an object of aliasReferenceType
        """

        try:
            return self.fullSchema.getAliasFromKey(key, aliasReferenceType)
        except RuntimeError as e:
            raise AttributeError(
                "Error while retrieving alias from parameter ({}): {}".
                format(key, e))

    def getKeyFromAlias(self, alias):
        """
        Return the key mapping to a given alias
        """

        try:
            return self.fullSchema.getKeyFromAlias(alias)
        except RuntimeError as e:
            raise AttributeError(
                "Error while retrieving parameter from alias ({}): {}".
                format(alias, e))

    def aliasHasKey(self, alias):
        """
        Check if a key for a given alias exists
        """

        return self.fullSchema.aliasHasKey(alias)

    def keyHasAlias(self, key):
        """
        Check if a given key has an alias defined
        """

        return self.fullSchema.keyHasAlias(key)

    def getValueType(self, key):
        """
        Get the ValueType in terms of `karabo::util::ReferenceTypes` for a
        given key
        """

        return self.fullSchema.getValueType(key)

    def getCurrentConfiguration(self, tags = ""):
        """
        Return the current configuration, optionally filtered by tags

        :param tags: a string, with several entries separated by commas
                    spaces or semicolons. Set to an empty string if no
                    filtering is to be applied.
        :return: a configuration Hash
        """

        if tags == "":
            return self.parameters
        with self._stateChangeLock:
            return HashFilter.byTag(self.fullSchema, self.parameters, tags, " ,;")

    def filterByTags(self, configuration, tags):
        """
        Filter a given configuration Hash by tags

        :param configuration:
        :param tags: a string, with several entries separated by commas
                     spaces or semicolons
        :return: the filtered configuration Hash
        """

        return HashFilter.byTag(self.fullSchema, configuration, tags, " ,;")

    def getServerId(self):
        """
        Return the id of the server hosting this devices
        """

        return self.serverid

    def getAvailableInstances(self):
        """
        Return available instances in the distributed system
        """

        return self._ss.getAvailableInstances()

    @karabo_deprecated
    def errorFoundAction(self, shortMessage, detailedMessage):
        """
        This method is DEPRECATED!
        A function to overwrite for error handling.
        :param shortMessage:
        :param detailedMessage:
        :return:
        """
        self.log.ERROR("{} -- {}".format(shortMessage, detailedMessage))

    def preReconfigure(self, incomingReconfiguration):
        """
        Use this hook to alter a configuration Hash before it gets applied to
        the device and the distributed system is notified of the change.
        :param incomingReconfiguration:
        """

        pass

    def postReconfigure(self):
        """
        Use this hook to react on configuration changes after they have been
        validated and applied to the device, and have been notified to the
        distributed system.
        """

        pass

    def preDestruction(self):
        """
        Use this hook if you need to perform clean-up actions before a device
        gets destroyed.
        """

        pass


    def initClassId(self):
        self.classid = self.__class__.__classid__

    def initSchema(self):
        self.staticSchema = PythonDevice.getSchema(self.classid)
        self.fullSchema = Schema(self.classid)
        self.fullSchema.copy(self.staticSchema)

    def updateState(self, currentState):
        """
        Update the state of the device to a new state. This should be used
        for NoFSM devices and should *not* be used if you have an underlying
        FSM.
        :param currentState: the state to set the device to
        :return:
        """
        assert isinstance(currentState, State)
        stateName = currentState.name
        self.log.DEBUG("updateState: {}".format(stateName))
        if self["state"] != currentState:
            self.set("state", currentState)
            if currentState is State.ERROR:
                self._ss.updateInstanceInfo(Hash("status", "error"))
            else:
                if self._ss.getInstanceInfo()["status"] == "error":
                    self._ss.updateInstanceInfo(Hash("status", "ok"))
        self._ss.reply(stateName)  # reply new state to interested event initiators

    @karabo_deprecated
    def onStateUpdate(self, currentState):
        """This method is DEPRECATED, use updateState() instead"""
        assert isinstance(currentState, State)
        self.updateState(currentState)

    @karabo_deprecated
    def exceptionFound(self, shortMessage, detailedMessage):
        """
        This method is DEPRECATED and will be removed.
        Hook for when an exception is encountered.
        Catch exceptions where they can occur instead, e.g.
        when calling a slot or requesting a value!

        :param shortMessage: exception message
        :param detailedMessage: detailed exception message
        """

        self.log.ERROR(shortMessage + " -- " + detailedMessage)

    def noStateTransition(self, currentState, currentEvent):
        """
        This function is called if a requested state transition is not allowed
        in the current context. Usually, this means you have an error in your
        state machine.
        """

        self.log.WARN("Device \"{}\" being in state '{}' does not allow the transition for event '{}'."
                      .format(self.deviceid, currentState, currentEvent))

    def onTimeUpdate(self, id, sec, frac, period):
        """
        Called when an update from the time server is received
        :param id: train id
        :param sec: seconds
        :param frac: fractional seconds
        :param period:
        :return:
        """

        pass

    def KARABO_SLOT(self, slot):
        """
        Register a slot in the distributed system. Note that a slot is only
        connected with a SLOT_ELEMENT if the key of the SLOT_ELEMENT matches
        the slot name provided to this function.

            SLOT_ELEMENT(expected).key("slotDoSomething")

            ....

            self.KARABO_SLOT(slotDoSomething)

            .....

            def slotDoSomethin(self):
                pass

        """

        self._ss.registerSlot(slot)

    def _initDeviceSlots(self):
        #-------------------------------------------- register intrinsic signals
        self._ss.registerSignal("signalChanged", Hash, str)                # changeHash, instanceId
        self._ss.registerSystemSignal("signalStateChanged", Hash, str)                # changeHash, instanceId

        self._ss.registerSystemSignal("signalSchemaUpdated", Schema, str)           # schema, deviceid

        self._ss.registerSystemSignal("signalAlarmUpdate", str, Hash)

        #---------------------------------------------- register intrinsic slots
        self._ss.registerSlot(self.slotReconfigure)
        self._ss.registerSlot(self.slotGetConfiguration)
        self._ss.registerSlot(self.slotGetSchema)
        self._ss.registerSlot(self.slotKillDevice)
        self._ss.registerSlot(self.slotUpdateSchemaAttributes)
        self._ss.registerSlot(self.slotReSubmitAlarms)
        # timeserver related slots
        self._ss.registerSlot(self.slotTimeTick)
        self._ss.registerSlot(self.slotLoggerPriority)
        self._ss.registerSlot(self.slotClearLock)

    def initChannels(self, topLevel=""):
        # Keys under topLevel, without leading "topLevel.":
        subKeys = self.fullSchema.getKeys(topLevel)
        # Now go recursively down the node:
        for subKey in subKeys:
            key = topLevel + '.' + subKey if topLevel else subKey
            if self.fullSchema.hasDisplayType(key):
                displayType = self.fullSchema.getDisplayType(key)
                if displayType == "OutputChannel":
                    # Would best be INFO level, but without broadcasting:
                    self.log.DEBUG("Creating output channel \"{}\"".format(key))
                    self._ss.createOutputChannel(key, self.parameters)
                elif displayType == "InputChannel":
                    # Would best be INFO level, but without broadcasting:
                    self.log.DEBUG("Creating input channel \"{}\"".format(key))
                    self._ss.createInputChannel(key, self.parameters)
                else:
                    self.log.DEBUG("Not creating in-/output channel for '" +
                                   key + "' since it's a '" + displayType +"'")
            elif self.fullSchema.isNode(key):
                # Recursive call going down the tree for channels within nodes
                self.log.DEBUG("Looking for input/output channels " +
                               "under node '" + key + "'")
                self.initChannels(key)


    def KARABO_ON_DATA(self, channelName, handlerPerData):
        """
        Registers a handler function to be called if data is received on an
        input channel identified by `channelName`. The handler function should
        have the signature

            def onData(data, metaData):
                pass

        where `data` and `metaData` are both Hashes.

         Note that for each channelName one can only use one of
        `KARABO_ON_DATA` and`KARABO_ON_INPUT`.

        """

        self._ss.registerDataHandler(channelName, handlerPerData)

    def KARABO_ON_INPUT(self, channelName, handlerPerInput):
        """
        Registers a handler to be called if data is available on the input
        channel identified by `channelName`. It is up to the device developer
        to read data (in contrast to the `KARABO_ON_DATA` registration).

            def onInput(input):
                for i in range(input.size()):
                    data, metaData = input.read(i)

        Here `input` is a reference to the input channel.

        Note that for each channelName one can only use one of
        `KARABO_ON_DATA` and`KARABO_ON_INPUT`.

        """

        self._ss.registerInputHandler(channelName, handlerPerInput)

    def KARABO_ON_EOS(self, channelName, handler):
        """
        Registers a handler to be called if input channel identified by `channelName`
        is signaled end-of-stream.

        The handler function should  have the signature

             def onEos(input):
             pass

        where `input` is a reference to the input channel.
        
        """
        self._ss.registerEndOfStreamHandler(channelName, handler)

    @karabo_deprecated
    def triggerError(self, s, d):
        """This method is deprecated, use execute() instead"""
        self.exceptionFound(s, d)

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
        isSchemaSlot = self.fullSchema.has(slotName)

        # Check whether the slot can be called given the current locking state
        lockableSlot = isSchemaSlot or slotName == "slotReconfigure"
        if self.allowLock() and lockableSlot and slotName != "slotClearLock":
            self._ensureSlotIsValidUnderCurrentLock(slotName, callee)

        if isSchemaSlot and self.fullSchema.hasAllowedStates(slotName):
            allowedStates = self.fullSchema.getAllowedStates(slotName)
            if allowedStates:
                currentState = self["state"]
                if currentState not in allowedStates:
                    msg = "Command \"{}\" is not allowed in current state \"{}\" " \
                          "of device \"{}\"".format(slotName, currentState, self.deviceid)
                    raise RuntimeError(msg)

        # Log the call of this slot by setting a parameter of the device
        if isSchemaSlot:
            self.set("lastCommand", slotName)

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

    def _ensureSlotIsValidUnderCurrentLock(self, slotName, callee):
        lockHolder = self["lockedBy"]
        if lockHolder:
            msg = "{} is locked by {} and called by {}"
            self.log.DEBUG(msg.format(self.deviceid, lockHolder, callee))
            if callee != "unknown" and callee != lockHolder:
                msg = "Command {} is not allowed as device is locked by {}"
                raise RuntimeError(msg.format(slotName, lockHolder))

    def slotGetConfiguration(self):
        self._ss.reply(self.parameters, self.deviceid)

    def slotReconfigure(self, newConfiguration):
        if newConfiguration.empty():
            return
        result, error, validated = self._validate(newConfiguration)
        if result:
            self.preReconfigure(validated)
            self._applyReconfiguration(validated)
        else:
            raise ValueError(error)

    def _validate(self, unvalidated):
        currentState = self["state"]
        whiteList = self._getStateDependentSchema(currentState)

        try:
            validated = self.validatorExtern.validate(whiteList, unvalidated, self.getActualTimestamp())
        except RuntimeError as e:
            errorText = str(e) + " in '" + str(currentState) + "'"
            return (False, errorText, unvalidated)
        self.log.DEBUG("Validated reconfiguration:\n{}".format(validated))
        return (True,"",validated)

    def _applyReconfiguration(self, reconfiguration):

        instanceInfoUpdate = Hash()
        with self._stateChangeLock:
            for prop in ["archive", "visibility"]:
                node = reconfiguration.find(prop)
                if (node and node.getValue() != self.parameters.get(prop)):
                    instanceInfoUpdate.set(prop, node.getValue())
            self.parameters += reconfiguration

        if not instanceInfoUpdate.empty():
            self.updateInstanceInfo(instanceInfoUpdate)

        if self.validatorExtern.hasReconfigurableParameter():
            self._ss.emit("signalStateChanged", reconfiguration, self.deviceid)
        else:
            self._ss.emit("signalChanged", reconfiguration, self.deviceid)
        self.postReconfigure()

    def slotGetSchema(self, onlyCurrentState):

        #senderId = self._ss.getSenderInfo("slotGetSchema").getInstanceIdOfSender()

        if onlyCurrentState:
            currentState = self["state"]
            schema = self._getStateDependentSchema(currentState)
            #self._ss.call(senderId, "slotSchemaUpdated", schema, self.deviceid)
            self._ss.reply(schema, self.deviceid)
        else:
            #self._ss.call(senderId, "slotSchemaUpdated", self.fullSchema, self.deviceid)
            self._ss.reply(self.fullSchema, self.deviceid)

    def slotKillDevice(self):        
        senderid = self._ss.getSenderInfo("slotKillDevice").getInstanceIdOfSender()
        if senderid == self.serverid and self.serverid != "__none__":
            self.log.INFO("Device is going down as instructed by server")
        else:
            self.log.INFO("Device is going down as instructed by \"{}\"".format(senderid))
            self._ss.call(self.serverid, "slotDeviceGone", self.deviceid)
        self.preDestruction()
        self.stopFsm()
        # TODO:
        # Remove this hack once we know how to get rid of the object cleanly
        # (slotInstanceGone will be called in _ss destructor again...).
        self._ss.call("*", "slotInstanceGone", self.deviceid,
                      self._ss.getInstanceInfo())

        # This will trigger the central event-loop to finish
        os.kill(os.getpid(), signal.SIGTERM)

    def slotUpdateSchemaAttributes(self, updates):
        success = self.fullSchema.applyRuntimeUpdates(updates)
        if success:
            # Notify everyone
            self._ss.emit("signalSchemaUpdated", self.fullSchema, self.deviceid)

        self._ss.reply(Hash("success", success,
                            "instanceId", self.deviceid,
                            "updatedSchema", self.fullSchema,
                            "requestedUpdate", updates))

    def slotTimeTick(self, id, sec, frac, period):
        epochNow = Epochstamp()
        with self._timeLock:
            self._timeId = id
            self._timeSec = sec
            self._timeFrac = frac
            # Fallback to the local timing ...
            if sec == 0:
                self._timeSec  = epochNow.getSeconds()
                self._timeFrac = epochNow.getFractionalSeconds()
            self._timePeriod = period
        self.onTimeUpdate(id, sec, frac, period)

    def slotLoggerPriority(self, newprio):
        oldprio = Logger.getPriority()
        Logger.setPriority(newprio)
        self.log.INFO("Logger Priority changed : {} ==> {}".format(oldprio, newprio))

    def getActualTimestamp(self):
        """
        Returns the actual timestamp. The Trainstamp part of Timestamp is
        extrapolated from the last values received via slotTimeTick (or zero
        if no time ticks received, i.e. timeServerId is empty).

        :return: the actual timestamp
        """
        return self.getTimestamp(Epochstamp())  # i.e. for now

    def getTimestamp(self, epoch):
        """
        Returns the Timestamp for given Epochstamp. The Trainstamp part of
        Timestamp is extrapolated forward or backward from the last values
        received via slotTimeTick (or zero if no time ticks received yet,
        e.g. if useTimeserver is false).

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
                    self.log.WARN("Bad input: (train)Id zero since epoch = {}; "
                                  "from time server: epoch = {}, id = {}, "
                                  "period = {} mus"
                                  .format(epoch.toIso8601(),
                                          epochLastReceived.toIso8601(),
                                          self._timeId, self._timePeriod))
        return Timestamp(epoch, Trainstamp(resultId))

    @karabo_deprecated
    def _getActualTimestamp(self):
        '''This method is DEPRECATED, use getActualTimestamp() instead'''
        return self.getActualTimestamp()

    def _getStateDependentSchema(self, state):
        with self._stateDependentSchemaLock:
            if state in self._stateDependentSchema:
                return self._stateDependentSchema[state]
            self._stateDependentSchema[state] = self.__class__.getSchema(
                self.classid,
                AssemblyRules(AccessType(WRITE), state.value))
            if not self._injectedSchema.empty():
                self._stateDependentSchema[state] += self._injectedSchema
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
        self.log.ERROR("Broker consumption problem: {}".format(message))
        # Trigger alarm, but not always a new one (system is busy anyway).
        # By setting messagingProblems up to every second, we can investigate
        # roughly the time of problems via the data logger.
        if (not self["performanceStatistics.messagingProblems"]
                or time.time() - self.lastBrokerErrorStamp >= 1.):
            self["performanceStatistics.messagingProblems"] = True
            self.lastBrokerErrorStamp = time.time()

    def setAlarmCondition(self, condition, needsAcknowledging = True,
                          description = ""):
        """
        Set the global alarm condition
        :param condition: condition to set
        :param needsAcknowledging: if this condition will require
               acknowledgment on the alarm service
        :param description: an optional description of the condition.
                Consider including remarks on how to resolve
        :return:
        """
        if not isinstance(condition, AlarmCondition):
            raise TypeError("First argument must be 'AlarmCondition',"
                            " not '{}'".format(str(type(condition))))
        resultingCondition = None
        currentCondition = None
        previousGlobal = self.globalAlarmCondition
        self.accumulatedGlobalAlarms.add(previousGlobal.asString())

        with self._stateChangeLock:
            self.globalAlarmCondition = condition
            resultingCondition = \
                self._evaluateAndUpdateAlarmCondition(forceUpdate=True,
                                                      prevParamsInAlarm=Hash(),
                                                      silent=True)
            currentCondition = self.parameters.get("alarmCondition")

        if (resultingCondition is not None
                and resultingCondition.asString() != currentCondition):
            self.set("alarmCondition", resultingCondition,
                     validate=False)

        emitHash = Hash("toClear", Hash(), "toAdd", Hash())
        conditionString = condition.asString()

        if (condition == AlarmCondition.NONE
                and previousGlobal != AlarmCondition.NONE):
            alarmsToClear = list(self.accumulatedGlobalAlarms)
            self.accumulatedGlobalAlarms.clear()
            emitHash.set("toClear.global", alarmsToClear)
        else:
            entry = Hash("type", condition.asString(),
                         "description", description,
                         "needsAcknowledging", needsAcknowledging)

            prop = Hash(conditionString, entry)
            entryNode = prop.getNode(conditionString)
            # attach current timestamp
            Timestamp().toHashAttributes(entryNode.getAttributes())
            emitHash.set("toAdd.global", prop)

        if (not emitHash.get("toClear").empty() or not
            emitHash.get("toAdd").empty()):

            self._ss.emit("signalAlarmUpdate", self.getInstanceId(), emitHash)


    def getAlarmCondition(self, key=None, separator="."):
        if key is None:
            return AlarmCondition.fromString(self.get("alarmCondition"))
        else:
            condition = self.parameters.getAttribute(
                key, "alarmCondition", separator)
            return AlarmCondition.fromString(condition)

    def hasRollingStatistics(self, key):
        return self.getFullSchema().hasRollingStatistics(key)

    def getRollingStatistics(self, key):
        return self.validatorIntern.getRollingStatistics(key)

    def getAlarmInfo(self):
        """
        Output information on current alarms on this device
        :return: a Hash containing the property as key and as string for
         the alarm information as value.
        """
        info = Hash()
        with self._stateChangeLock:
            warnings = self.validatorIntern.getParametersInWarnOrAlarm()
            for key in warnings.getKeys():
                desc = warnings.get(key)
                condition = AlarmCondition.fromString(desc.get("type"))
                thisinfo = self.fullSchema.getInfoForAlarm(key,condition)
                info.set(key, thisinfo)
        return info

    @staticmethod
    def loadConfiguration(xmlfile):
        hash = Hash()
        loadFromFile(hash, xmlfile)
        os.remove(xmlfile)
        return hash

    # the following functions expose parts of SignalSlotable to the public
    # device interface.

    def registerSignal(self, signalName, *args):
        """
        Register a signal to be handles in the remote system
        :param signalName:name of the signal to be registered
        :param args: signature of the signal, e.g. `str, Hash, str`
        """
        self._ss.registerSignal(signalName, *args)

    def connect(self, signalInstance, signalName, slotInstance, slotName):
        """
        Connect a signal with a slot
        :param signalInstance: instance the signal is on, use "" for local
        :param signalName: name of the signal to connect
        :param slotInstance: instance the slot is on, use "" for local
        :param slotName: name of the slot to be executed upon signal reception
        """
        self._ss.connect(signalInstance, signalName, slotInstance, slotName)

    def reply(self, *args):
        """
        Send a reply upon a slot being called
        :param args: list of arguments to reply, maximum length is 4
        """
        self._ss.reply(*args)

    def emit(self, signalName, *args):
        """
        Emit a signal to the remote system
        :param signalName: name of the signal.
        :param args: list of arguments signal is emitted with. Maximum 4
        """
        self._ss.emit(signalName, *args)

    def call(self, instanceId, slotName, *args):
        """
        Call a remote slot with arguments
        :param instanceId: instance of the remote device to call slot on
        :param slotName: name of the slot to call on instanceId
        :param args: list of arguments to call slot with, maximum length is 4
        """
        self._ss.call(instanceId, slotName, *args)

    def request(self, instanceId, slotName, *args):
        """
        Request a reply from a remote slot
        :param instanceId: instance of the remote device to request from
        :param slotName: name of the slot to request from on instanceId
        :param args: list of arguments to call slot with, maximum length is 4
        :return: a `SignalSlotable.Requestor` object handling the reply
        """
        return self._ss.request(instanceId, slotName, *args)

    def requestNoWait(self, instanceId, slotName, replyInstance,
                      replySlotName, *args):
        """
        Request a reply from a remote slot
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



def launchPythonDevice():
    from .plugin_loader import PluginLoader

    # NOTE: The first argument is '-c'
    _, modname, classid, xmlfile = tuple(sys.argv)
    config = PythonDevice.loadConfiguration(xmlfile)
    # If log filename not specified, make use of device name to avoid
    # that different processes write to the same file.
    # TODO:
    # "karabo.log" is default from RollingFileAppender::expectedParameters.
    # Unfortunately, GUI sends full config, including defaults.
    if (not "Logger.file.filename" in config
        or config["Logger.file.filename"] == "karabo.log"):
        deviceId = config["_deviceId_"]
        defaultLog = "device-" + deviceId.replace(os.path.sep, "_") + ".log"
        config["Logger.file.filename"] = defaultLog
    loader = PluginLoader.create(
        "PythonPluginLoader", Hash("pluginNamespace", "karabo.bound_device")
    )
    loader.update()

    try:
        # Load the module containing classid so that it gets registered.
        entrypoint = loader.getPlugin(modname)
        deviceClass = entrypoint.load()
        assert deviceClass.__classid__ == classid
        t = threading.Thread(target=EventLoop.work)
        t.start()

        device = Configurator(PythonDevice).create(classid, config)
        device._finalizeInternalInitialization()

        t.join()
        device.__del__()
    except Exception:
        print("Exception caught when trying to run a '{}':".format(classid))
        traceback.print_exc()
    os._exit(77)
