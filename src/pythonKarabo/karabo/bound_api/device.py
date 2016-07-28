__author__ = "Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ = "$Jul 30, 2012 9:03:51 PM$"

import threading
import os
import time
import datetime
import sys
import socket
import re

from PIL import Image
import numpy as np

from karathon import (
    CpuImageCHAR, CpuImageDOUBLE, CpuImageFLOAT, CpuImageINT16,
    CpuImageINT32, CpuImageUINT16, CpuImageUINT8,
    BOOL_ELEMENT, CHOICE_ELEMENT, FLOAT_ELEMENT, INT32_ELEMENT, INT64_ELEMENT,
    UINT32_ELEMENT, NODE_ELEMENT, STRING_ELEMENT,
    OBSERVER, READ, WRITE, INIT,
    AccessLevel, AccessType, AssemblyRules, BrokerConnection,
    Data, DeviceClient, Epochstamp, Hash, HashFilter, HashMergePolicy,
    ImageData, Logger, NDArray, Priority, RawImageData, Schema, SignalSlotable,
    Timestamp, Trainstamp, Unit, MetricPrefix, Validator, ValidatorValidationRules,
    loadFromFile
)

from karabo.common.states import State
from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from .configurator import Configurator
from .no_fsm import NoFsm
from .alarm_conditions import AlarmCondition

def isCpuImage(value):
    return isinstance(value, (CpuImageCHAR, CpuImageDOUBLE, CpuImageFLOAT,
                              CpuImageINT16, CpuImageINT32, CpuImageUINT16,
                              CpuImageUINT8))

@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("PythonDevice", "1.0")
class PythonDevice(NoFsm):

    instanceCountPerDeviceServer = dict()
    instanceCountLock = threading.Lock()
    
    @staticmethod
    def expectedParameters(expected):
        (
            STRING_ELEMENT(expected).key("compatibility")
                    .displayedName("Compatibility").description("The compatibility of this device to the Karabo framework")
                    .expertAccess().readOnly().initialValue(PythonDevice.__version__)
                    .commit(),

            STRING_ELEMENT(expected).key("_serverId_")
                    .displayedName("_ServerID_").description("Do not set this property, it will be set by the device-server")
                    .expertAccess().assignmentInternal().noDefaultValue().init()
                    .commit(),

            STRING_ELEMENT(expected).key("_deviceId_")
                    .displayedName("_DeviceID_").description("Do not set this property, it will be set by the device-server")
                    .expertAccess().assignmentInternal().noDefaultValue().init()
                    .commit(),

            INT32_ELEMENT(expected).key("visibility")
                    .displayedName("Visibility").description("Configures who is allowed to see this device at all")
                    .assignmentOptional().defaultValue(AccessLevel(OBSERVER))
                    .expertAccess().reconfigurable()
                    .commit(),

            CHOICE_ELEMENT(expected).key("_connection_")
                    .displayedName("Connection")
                    .description("The connection to the communication layer of the distributed system")
                    .appendNodesOfConfigurationBase(BrokerConnection)
                    .assignmentOptional().defaultValue("Jms")
                    .adminAccess()
                    .init()
                    .commit(),

            STRING_ELEMENT(expected).key("classId")
                    .displayedName("ClassID").description("The (factory)-name of the class of this device")
                    .expertAccess().readOnly().initialValue(PythonDevice.__classid__)
                    .commit(),

            STRING_ELEMENT(expected).key("serverId")
                    .displayedName("ServerID").description("The device-server on which this device is running on")
                    .expertAccess().readOnly()
                    .commit(),

            STRING_ELEMENT(expected).key("deviceId")
                    .displayedName("DeviceID").description("The device instance ID uniquely identifies a device instance in the distributed system")
                    .readOnly()
                    .commit(),

            BOOL_ELEMENT(expected).key("archive")
                        .displayedName("Archive")
                        .description("Decides whether the properties of this device will be logged or not")
                        .reconfigurable()
                        .expertAccess()
                        .assignmentOptional().defaultValue(True)
                        .commit(),

            BOOL_ELEMENT(expected).key("useTimeserver")
                        .displayedName("Use Timeserver")
                        .description("Decides whether to use time and train ID from TimeServer device")
                        .init()
                        .expertAccess()
                        .assignmentOptional().defaultValue(False)
                        .commit(),
                        
            INT32_ELEMENT(expected).key("progress")
                    .displayedName("Progress").description("The progress of the current action")
                    .readOnly().initialValue(0).commit(),
                    
            STRING_ELEMENT(expected).key("state")
                    .displayedName("State").description("The current state the device is in")
                    .readOnly().initialValue(State.UNKNOWN.name)
                    .commit(),

            STRING_ELEMENT(expected).key("alarmCondition")
                        .displayedName("Alarm condition")
                        .description("The current alarm condition of the device. "
                                     "Evaluates to the highest condition on any"
                                     " property if not set manually.")
                        .readOnly().initialValue(AlarmCondition.NONE.asString())
                        .commit(),

            NODE_ELEMENT(expected).key("performanceStatistics")
                    .displayedName("Performance Statistics")
                    .description("Accumulates some statistics")
                    .expertAccess()
                    .commit(),

            BOOL_ELEMENT(expected).key("performanceStatistics.enable")
                    .displayedName("Enable Performance Indicators")
                    .description("Enables some statistics to follow the performance of an individual device")
                    .reconfigurable()
                    .expertAccess()
                    .assignmentOptional().defaultValue(False)
                    .commit(),

            FLOAT_ELEMENT(expected).key("performanceStatistics.brokerLatency")
                    .displayedName("Broker latency (ms)")
                    .description("Average time interval between remote message sending and receiving it on this device before queuing.")
                    .unit(Unit.SECOND).metricPrefix(MetricPrefix.MILLI)
                    .expertAccess()
                    .readOnly().initialValue(0.0)
                    .commit(),

            FLOAT_ELEMENT(expected).key("performanceStatistics.processingLatency")
                    .displayedName("Processing latency (ms)")
                    .description("Average time interval between remote message sending and reading it from the queue on this device.")
                    .unit(Unit.SECOND).metricPrefix(MetricPrefix.MILLI)
                    .expertAccess()
                    .readOnly().initialValue(0.0)
                    .warnHigh(3000.)  # 3 s
                    .info("Long average time between message being sent and start of its processing")
                    .needsAcknowledging(False)
                    .alarmHigh(10000.)  # 10 s
                    .info("Very long average time between message being sent and start of its processing")
                    .needsAcknowledging(False)
                    .commit(),

            UINT32_ELEMENT(expected).key("performanceStatistics.maxProcessingLatency")
                     .displayedName("Maximum proc. latency")
                     .description("Maximum processing latency within averaging interval.")
                     .unit(Unit.SECOND).metricPrefix(MetricPrefix.MILLI)
                     .expertAccess()
                     .readOnly().initialValue(0)
                     .commit(),

            UINT32_ELEMENT(expected).key("performanceStatistics.messageQueueSize")
                    .displayedName("Local message queue size")
                    .description("Current size of the local message queue.")
                    .expertAccess()
                    .readOnly().initialValue(0)
                    #.warnHigh(100)
                    .commit(),

            NODE_ELEMENT(expected).key("Logger")
                    .description("Logging settings")
                    .displayedName("Logger")
                    .appendParametersOfConfigurableClass(Logger,"Logger")
                    .expertAccess()
                    .commit(),

        )
        
    def __init__(self, configuration):
        if configuration is None:
            raise ValueError("Configuration must be Hash object, not None")
        #print "PythonDevice constructor: Input configuration after being validated is ...\n", configuration
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
        self.hostname, dotsep, self.domainname = socket.gethostname().partition('.')
        
        # timeserver related
        self._timeLock = threading.Lock()
        self._timeId = 0
        self._timeSec = 0
        self._timeFrac = 0
        self._timePeriod = 0
        
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

        # Instantiate SignalSlotable object without starting event loop
        try:
            self._ss = SignalSlotable.create(self.deviceid, "Jms", self.parameters["_connection_.Jms"], autostart = False)
        except RuntimeError as e:
            raise RuntimeError("PythonDevice.__init__: SignalSlotable.create Exception -- {0}".format(str(e)))
        # Setup device logger
        self.loadLogger(configuration)
        self.log = Logger.getLogger(self.deviceid)

        # Initialize FSM slots if defined
        if hasattr(self, 'initFsmSlots'):
            self.initFsmSlots(self._ss)
        
        # Initialize Device slots
        self._initDeviceSlots()
        
        # Initialize regular expression object
        self.errorRegex = re.compile(".*error.*", re.IGNORECASE)
        
        # Register guard for slot calls
        self._ss.registerSlotCallGuardHandler(self.slotCallGuard)
        
        # Register exception handler
        self._ss.registerExceptionHandler(self.exceptionFound)

        # Register updateLatencies handler
        self._ss.registerPerformanceStatisticsHandler(self.updateLatencies)
        
    
    @property
    def signalSlotable(self):
        '''Get SignalSlotable object embedded in PythonDevice instance.'''
        return self._ss
    
    def setNumberOfThreads(self, nThreads):
        self._ss.setNumberOfThreads(nThreads)
    
    def loadLogger(self,input):
        config = input["Logger"]

        # make a copy of additional appenders defined by user
        appenders = config["appenders"]
        config["appenders[2].Network.layout"] = Hash()
        config["appenders[2].Network.layout.Pattern.format"] = "%d{%F %H:%M:%S} | %p | %c | %m"
        if "connection" in input:
            config["appenders[2].Network.connection"] = input["connection"]
#        print "loadLogger final:\n", config
        self.logger=Logger.configure(config)
        
    def run(self):
        self.initClassId()
        self.initSchema()
            
        # Create 'info' hash
        info = Hash("type", "device")
        info["classId"] = self.classid
        info["serverId"] = self.serverid
        info["visibility"] = self["visibility"]
        info["compatibility"] = self.__class__.__version__
        info["host"] = self.hostname
        info["status"] = "ok"
        info["archive"] = self.get("archive")
        #... add here more info entries if needed

        self.parameters.set("classId", self.classid)
        self.parameters.set("deviceId", self.deviceid)
        self.parameters.set("serverId", self.serverid)
        
        # Run event loop ( in a thread ) with given info
        # TODO Make configurable
        t = threading.Thread(target = self._ss.runEventLoop, args = (20, info))
        t.start()
        time.sleep(0.01) # for rescheduling, some garantie that runEventLoop will start before FSM
        
        # if our own instanceId is used on topic -- exit
        ok = self._ss.ensureOwnInstanceIdUnique()
        if not ok:
            self.log.ERROR("Device of class '{0.classid}' could not start on "
                           "server '{0.serverid}' since id '{0.deviceid}' "
                           "already exists.".format(self))
            t.join()
            return

        self.log.INFO("'{0.classid}' with deviceId '{0.deviceid}' got started "
                      "on server '{0.serverid}'.".format(self))

        with self._stateChangeLock:
            validated = self.validatorIntern.validate(self.fullSchema, self.parameters, self._getActualTimestamp())
            self.parameters.merge(validated, HashMergePolicy.REPLACE_ATTRIBUTES)

        # Instantiate all channels
        self.initChannels()
        self._ss.connectInputChannels()
        
        self.startFsm()
        
        if self.parameters.get("useTimeserver"):
            self.log.DEBUG("Connecting to time server")
            self._ss.connect("Karabo_TimeServer", "signalTimeTick", "", "slotTimeTick")

        t.join()
            
    def stopEventLoop(self):
        self._ss.stopEventLoop()
    
    def __del__(self):
        ''' PythonDevice destructor '''
        # Destroying device instance means finishing subprocess runnung a device instancPythonConveyore.
        # Call exit for child processes (os._exit(...)) to shutdown properly a SignalSlotable object
        os._exit(0)   

    def remote(self):
        if self._client is None:
            self._client = DeviceClient(self._ss)  # SignalSlotable object for reuse
        return self._client
    
    def _setImage(self, key, image):
        hash = Hash(key, Hash())
        image.copyTo(hash[key])
        hash.setAttribute(key, "image", 1)
        self.parameters.merge(hash, HashMergePolicy.REPLACE_ATTRIBUTES)
        self._ss.emit("signalChanged", hash, self.deviceid)
    
    def _setRawImageData(self, key, image):
        hash = Hash(key, image.hash())
        hash.setAttribute(key, "image", 1)
        self.parameters.merge(hash, HashMergePolicy.REPLACE_ATTRIBUTES)
        self._ss.emit("signalChanged", hash, self.deviceid)


    def set(self, *args, **kwargs):
        """
        Updates the state of the device. This function automatically notifies any observers.

        args: can be of length

            * one: expects a hash, and uses current timestamp
            * two: expects a key, value pair and uses current timestamp or a hash, timestamp pair
            * three: expects key, value and timestamp

        kwargs: validate: specifies if validation of args should be performed before notification.


        """
        pars = tuple(args)
        hadPreviousParameterAlarm = self.validatorIntern.hasParametersInWarnOrAlarm()
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
                if isCpuImage(value):
                    self._setImage(key, value)
                    return
                elif isinstance(value, RawImageData):
                    self._setRawImageData(key, value)
                    return
                pars = tuple([Hash(key, value), stamp])
            
            # hash args
            if len(pars) == 1:
                h = pars[0]
                if not isinstance(h, Hash):
                    raise TypeError("The only argument should be a Hash")
                pars = tuple([h, self._getActualTimestamp()])   # add timestamp
            
            # key, value or hash, timestamp args
            if len(pars) == 2:
                if not isinstance(pars[0], Hash):
                    key, value = pars
                    if isCpuImage(value):
                        self._setImage(key, value)
                        return
                    elif isinstance(value, RawImageData):
                        self._setRawImageData(key, value)
                        return
                    pars = tuple([Hash(key,value), self._getActualTimestamp()])
                hash, stamp = pars
                # Check that hash is image's free
                paths = hash.getPaths()
                for key in paths:
                    value = hash[key]
                    if isCpuImage(value):
                        self._setImage(key, value)    # process images individually
                        hash.erasePath(key)      # clear hash from images 
                    elif isinstance(value, RawImageData):
                        self._setRawImageData(key, value)
                        hash.erasePath(key)

                validated = None
                if validate:
                    validated = self.validatorIntern.validate(self.fullSchema, hash, stamp)
                    resultingCondition = self._evaluateAndUpdateAlarmCondition(forceUpdate=hadPreviousParameterAlarm)
                    if resultingCondition is not None and resultingCondition.asString() != self.parameters.get("alarmCondition"):
                        validated.set("alarmCondition", resultingCondition.asString())
                        node = validated.getNode("alarmCondition")
                        attributes = node.getAttributes()
                        stamp.toHashAttributes(attributes)
                    
                else:
                    validated = hash

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
                    

    def _evaluateAndUpdateAlarmCondition(self, forceUpdate):
        if self.validatorIntern.hasParametersInWarnOrAlarm():
            warnings = self.validatorIntern.getParametersInWarnOrAlarm()
            conditions = [self.globalAlarmCondition]

            for key in warnings:
                desc = warnings[key]
                self.log.WARN("{}: {}".format(desc["type"],desc["message"]))
                self._ss.emit("signalNotification", desc["type"], desc["message"], "", self.deviceid)
                conditions.append(AlarmCondition.fromString(desc["type"]))

            mostSignificantCondition = AlarmCondition.returnMostSignificant(conditions)
            return mostSignificantCondition
        elif forceUpdate:
            return self.globalAlarmCondition
        else:
            return None
    
    def __setitem__(self, key, value):
        self.set(key, value, self._getActualTimestamp())
        
    def writeChannel(self, *args):
        pars = tuple(args)
        if len(pars) < 2 or len(pars) > 3:
            raise SyntaxError("Number of parameters is wrong: only 2 to 3 arguments are allowed.")
        if len(pars) == 3:
            channelName, key, value = pars
            if isCpuImage(value):
                dataval = ImageData(value)
            elif isinstance(value, (Data, ImageData, NDArray)):
                dataval = value
            elif isinstance(value, Image.Image):
                dataval = ImageData(np.array(value))
            else:
                raise ValueError('The type of value is neither a "CpuImage" nor a "Data"')
            data = Data(key, dataval)
        elif len(pars) == 2:
            channelName, data = pars
            if isinstance(data, (ImageData, NDArray)):
                data = Data(data.hash())
            elif isinstance(data, Image.Image):
                img = data
                imgdata = ImageData(np.array(img))
                data = Data(imgdata.hash())
            elif isinstance(data, Data):
                pass
            else:
                raise ValueError('Unsupported type of value: {}'.format(type(data)))
        data.attachTimestamp(self._getActualTimestamp())
        channel = self._ss.getOutputChannel(channelName)
        channel.write(data)
        channel.update()
        
    def signalEndOfStream(self, channelName):
        self._ss.getOutputChannel(channelName).signalEndOfStream()
        
    def get(self,key):
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
        rules = ValidatorValidationRules()
        rules.allowAdditionalKeys        = True
        rules.allowMissingKeys           = True
        rules.allowUnrootedConfiguration = True
        rules.injectDefaults             = True
        rules.injectTimestamps           = True
        validator = Validator()
        validator.setValidationRules(rules)
        validated = validator.validate(schema, Hash(), self._getActualTimestamp())
        with self._stateChangeLock:
            for path in self._injectedSchema.getPaths():
                if self.parameters.has(path) and not self.staticSchema.has(path): 
                    self.parameters.erase(path)
            self._stateDependentSchema = {}
            self._injectedSchema.copy(schema)
            self.fullSchema.copy(self.staticSchema)
            self.fullSchema += self._injectedSchema
            #self.parameters.merge(validated, HashMergePolicy.REPLACE_ATTRIBUTES)
            #validated = self.validatorIntern.validate(self.fullSchema, self.parameters, self._getActualTimestamp())
            #self.parameters.merge(validated, HashMergePolicy.REPLACE_ATTRIBUTES)
            self.fullSchema.updateAliasMap()
        # notify the distributed system...
        self._ss.emit("signalSchemaUpdated", self.fullSchema, self.deviceid)
        self.set(validated)
        self.log.INFO("Schema updated")
    
    def appendSchema(self, schema):
        rules = ValidatorValidationRules()
        rules.allowAdditionalKeys        = True
        rules.allowMissingKeys           = True
        rules.allowUnrootedConfiguration = True
        rules.injectDefaults             = True
        rules.injectTimestamps           = True
        validator = Validator()
        validator.setValidationRules(rules)
        validated = validator.validate(schema, self.parameters, self._getActualTimestamp())
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
    
    def setProgress(self, value, associatedText = ""):
        v = self.progressMin + value / (self.progressMax - self.progressMin)
        self.set("progress", v)
            
    def resetProgress(self):
        set("progress", self.progressMin)
    
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
        return self.fullSchema.aliasHasKey(alias)
    
    def keyHasAlias(self, key):
        return self.fullSchema.keyHasAlias(key)
        
    def getValueType(self, key):
        return self.fullSchema.getValueType(key)
    
    def getCurrentConfiguration(self, tags = ""):
        if tags == "":
            return self.parameters
        with self._stateChangeLock:
            return HashFilter.byTag(self.fullSchema, self.parameters, tags, " ,;")
    
    def filterByTags(self, configuration, tags):
        return HashFilter.byTag(self.fullSchema, configuration, tags, " ,;")
    
    def getServerId(self):
        return self.serverid
    
    def getAvailableInstances(self):
        return self._ss.getAvailableInstances()
    
    # In C++: the following functions are protected
    
    def errorFoundAction(self, shortMessage, detailedMessage):
        self.log.ERROR("{} -- {}".format(shortMessage, detailedMessage))
        self._ss.emit("signalNotification", "ERROR", shortMessage, detailedMessage, self.deviceid)
    
    def preReconfigure(self, incomingReconfiguration):
        pass
    
    def postReconfigure(self):
        pass
    
    def preDestruction(self):
        pass
    
    # In C++: the following functions are private...
    
    def initClassId(self):
        self.classid = self.__class__.__classid__
    
    def initSchema(self):
        self.staticSchema = PythonDevice.getSchema(self.classid)
        self.fullSchema = Schema(self.classid)
        self.fullSchema.copy(self.staticSchema)
        
    def updateState(self, currentState):
        assert isinstance(currentState, State)
        stateName = currentState.name
        self.log.DEBUG("updateState: {}".format(stateName))
        if self["state"] != stateName:
            self["state"] = stateName
            if currentState is State.ERROR:
                self._ss.updateInstanceInfo(Hash("status", "error"))
            else:
                if self._ss.getInstanceInfo()["status"] == "error":
                    self._ss.updateInstanceInfo(Hash("status", "ok"))
        self._ss.reply(stateName)  # reply new state to interested event initiators

    def onStateUpdate(self, currentState):
        assert isinstance(currentState, State)
        print("onStateUpdate() is deprecated, use updateState() instead")
        self.updateState(currentState)

    def exceptionFound(self, shortMessage, detailedMessage):
        self.log.ERROR(shortMessage + " -- " + detailedMessage)
        self._ss.emit("signalNotification", "EXCEPTION", shortMessage,
                      detailedMessage, self.deviceid)

    def noStateTransition(self):
        self.log.WARN("Device \"{}\" does not allow the transition for this event.".format(self.deviceid))
        
    def onTimeUpdate(self, id, sec, frac, period):
        pass
    
    def KARABO_SLOT(self, slot):
        self._ss.registerSlot(slot)
        
    def _initDeviceSlots(self):
        #-------------------------------------------- register intrinsic signals
        self._ss.registerSignal("signalChanged", Hash, str)                # changeHash, instanceId        
        self._ss.registerSystemSignal("signalStateChanged", Hash, str)                # changeHash, instanceId        

        self._ss.registerSystemSignal("signalNotification", str, str, str, str)     # type, shortMessage, detailedMessage, deviceId

        self._ss.registerSystemSignal("signalSchemaUpdated", Schema, str)           # schema, deviceid
                
        #---------------------------------------------- register intrinsic slots
        self._ss.registerSlot(self.slotReconfigure)        
        self._ss.registerSlot(self.slotGetConfiguration)
        self._ss.registerSlot(self.slotGetSchema)
        self._ss.registerSlot(self.slotKillDevice)        
        # timeserver related slots
        self._ss.registerSlot(self.slotTimeTick)
        self._ss.registerSlot(self.slotLoggerPriority)

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
        self._ss.registerDataHandler(channelName, handlerPerData)

    def KARABO_ON_INPUT(self, channelName, handlerPerInput):
        self._ss.registerInputHandler(channelName, handlerPerInput)

    def KARABO_ON_EOS(self, channelName, handler):
        self._ss.registerEndOfStreamHandler(channelName, handler)
    
    def triggerError(self, s, d):
        print("The triggerError() function is deprecated, use execute() instead")
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
    
    def slotCallGuard(self, slotName):
        if slotName in self.fullSchema and self.fullSchema.hasAllowedStates(slotName):
            allowedStates = self.fullSchema.getAllowedStates(slotName)
            if allowedStates:
                # print("Validating slot")
                currentState = self["state"]
                if currentState not in allowedStates:
                    msg = "Command \"{}\" is not allowed in current state \"{}\" of device \"{}\"".format(slotName, currentState, self.deviceid)
                    self._ss.reply(msg)
                    return False
        return True
    
    def slotGetConfiguration(self):
        #senderId = self._ss.getSenderInfo("slotGetConfiguration").getInstanceIdOfSender()
        #self._ss.call(senderId, "slotChanged", self.parameters, self.deviceid)
        self._ss.reply(self.parameters, self.deviceid)
        
    def slotReconfigure(self, newConfiguration):
        if newConfiguration.empty():
            return
        result, error, validated = self._validate(newConfiguration)
        if result:
            try:
                self.preReconfigure(validated)
                self._applyReconfiguration(validated)
            except Exception as e:
                print("PythonDevice.slotReconfigure Exception:", str(e))
                self.exceptionFound("Python Exception happened", str(e))
                self._ss.reply(False, str(e))
                return
        self._ss.reply(result, error)
    
    def _validate(self, unvalidated):
        currentState = self["state"]
        whiteList = self._getStateDependentSchema(currentState)
        self.log.DEBUG("Incoming (un-validated) reconfiguration:\n{}".format(unvalidated))
        try:
            validated = self.validatorExtern.validate(whiteList, unvalidated, self._getActualTimestamp())
        except RuntimeError as e:
            errorText = str(e) + " in state: \"" + currentState + "\""
            return (False, errorText, unvalidated)
        self.log.DEBUG("Validated reconfiguration:\n{}".format(validated))
        return (True,"",validated)
    
    def _applyReconfiguration(self, reconfiguration):
        with self._stateChangeLock:
            self.parameters += reconfiguration
        self.log.DEBUG("After user interaction:\n{}".format(reconfiguration))
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
        self.stopEventLoop()
   
    def slotTimeTick(self, id, sec, frac, period):
        with self._timeLock:
            self._timeId = id
            self._timeSec = sec
            self._timeFrac = frac
            self._timePeriod = period
        self.onTimeUpdate(id, sec, frac, period)
        
    def slotLoggerPriority(self, newprio):
        oldprio = Priority.getPriorityName(self.logger.getLogger("some_deviceId").getRootPriority())
        self.logger.getLogger("some_deviceId").setRootPriority(Priority.getPriorityValue(newprio))
        self.log.INFO("Logger Priority changed : {} ==> {}".format(oldprio, newprio))
        
    def _getActualTimestamp(self):
        epochNow = Epochstamp()
        id = 0
        with self._timeLock:
            if self._timePeriod > 0:
                epochLastReceived = Epochstamp(self._timeSec, self._timeFrac)
                duration = epochNow.elapsed(epochLastReceived)
                nPeriods = int((duration.getTotalSeconds() * 1000000 + duration.getFractions() / 1000) / self._timePeriod)
                id = self._timeId + nPeriods
        return Timestamp(epochNow, Trainstamp(int(id)))
        
    def _getStateDependentSchema(self, state):
        with self._stateDependentSchemaLock:
            if state in self._stateDependentSchema:
                return self._stateDependentSchema[state]
            self._stateDependentSchema[state] = self.__class__.getSchema(self.classid, AssemblyRules(AccessType(READ | WRITE | INIT), state))
            if not self._injectedSchema.empty():
                self._stateDependentSchema[state] += self._injectedSchema
            return self._stateDependentSchema[state]
    
    def getInstanceId(self):
        return self._ss.getInstanceId()
   
    def registerSlot(self, slotFunc):
        self._ss.registerSlot(slotFunc)
        
    def updateLatencies(self, avgBrokerLatency, maxBrokerLatency,
                        avgProcessingLatency, maxProcessingLatency, messageQueueSize):
        if self.get("performanceStatistics.enable"):
            # ignore maxBrokerLatency
            stats = Hash("brokerLatency", avgBrokerLatency,
                         "processingLatency", avgProcessingLatency,
                         "maxProcessingLatency", maxProcessingLatency,
                         "messageQueueSize", messageQueueSize)
            self.set(Hash("performanceStatistics", stats))


    def setAlarmCondition(self, condition):
        if isinstance(condition, str):
            raise TypeError("Stringified alarmconditions are note allowed!")
        with self._stateChangeLock:
            self.globalAlarmCondition = condition
            resultingCondition = \
                self._evaluateAndUpdateAlarmCondition(forceUpdate=True)
            if resultingCondition is not None and resultingCondition.asString()\
                    != self.parameters.get("alarmCondition"):
                self.set("alarmCondition", resultingCondition.asString(),
                         validate=False)

    def getAlarmCondition(self, key = None, seperator = "."):
        if key is None:
            return AlarmCondition.fromString(self.get("alarmCondition"))
        else:
            condition = self.parameters.getAttribute(key,"alarmCondition", sep)
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
            for key in warnings:
                desc = warnings.get(key)
                condition = AlarmCondition.fromString(desc.get("type"))
                thisinfo = self.fullSchema.getInfoForAlarm(str(key),condition)
                info.set(str(key), thisinfo)
        return info


        
    '''
    def getCurrentDateTime(self):
        return datetime.datetime(1,1,1).today().isoformat(' ')
    
    def triggerErrorFound(self, shortMessage, detailedMessage):
        self._ss.emit(
            "signalErrorFound", self.getCurrentDateTime(), shortMessage,
            detailedMessage, self.deviceid)

    def triggerWarning(self, warningMessage, priority):
        self._ss.emit(
            "signalWarning", self.getCurrentDateTime(), warningMessage,
            self.deviceid, priority)
    
    def triggerAlarm(self, alarmMessage, priority):
        self._ss.emit(
            "signalAlarm", self.getCurrentDateTime(), alarmMessage,
            self.deviceid, priority)
    '''
    
    @staticmethod
    def loadConfiguration(xmlfile):
        hash = Hash()
        loadFromFile(hash, xmlfile)
        os.remove(xmlfile)
        return hash


def launchPythonDevice():
    from .plugin_loader import PluginLoader

    # NOTE: The first argument is '-c'
    _, plugindir, modname, classid, xmlfile = tuple(sys.argv)
    config = PythonDevice.loadConfiguration(xmlfile)
    loader = PluginLoader.create(
        "PythonPluginLoader",
        Hash("pluginNamespace", "karabo.bound_device",
             "pluginDirectory", plugindir)
    )
    loader.update()

    try:
        # Load the module containing classid so that it gets registered.
        entrypoint = loader.getPlugin(modname)
        deviceClass = entrypoint.load()
        assert deviceClass.__classid__ == classid

        device = Configurator(PythonDevice).create(classid, config)
        device.run()
        device.__del__()
    except Exception as e:
        print("Exception caught: " + str(e))
    os._exit(77)
