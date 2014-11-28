__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Jul 30, 2012 9:03:51 PM$"

import threading
import os
import time
import datetime
import sys
import socket
import re
from abc import ABCMeta, abstractmethod
from karabo.karathon import *
from karabo.decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from karabo.configurator import Configurator
from karabo.no_fsm import NoFsm

def isCpuImage(value):
    return (type(value) is CpuImageCHAR or type(value) is CpuImageDOUBLE
            or type(value) is CpuImageFLOAT or type(value) is CpuImageINT16
            or type(value) is CpuImageINT32 or type(value) is CpuImageUINT16
            or type(value) is CpuImageUINT8)

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
                    .expertAccess().readOnly().initialValue(PythonDevice.__version__).commit()
                    ,
            STRING_ELEMENT(expected).key("_serverId_")
                    .displayedName("_ServerID_").description("Do not set this property, it will be set by the device-server")
                    .expertAccess().assignmentInternal().noDefaultValue().init().commit()
                    ,
            STRING_ELEMENT(expected).key("_deviceId_")
                    .displayedName("_DeviceID_").description("Do not set this property, it will be set by the device-server")
                    .expertAccess().assignmentInternal().noDefaultValue().init().commit()
                    ,
            INT32_ELEMENT(expected).key("visibility")
                    .displayedName("Visibility").description("Configures who is allowed to see this device at all")
                    .assignmentOptional().defaultValue(AccessLevel(OBSERVER))
                    .expertAccess().reconfigurable().commit()
                    ,
            STRING_ELEMENT(expected).key("classId")
                    .displayedName("ClassID").description("The (factory)-name of the class of this device")
                    .expertAccess().readOnly().initialValue(PythonDevice.__classid__).commit()
                    ,
            STRING_ELEMENT(expected).key("serverId")
                    .displayedName("ServerID").description("The device-server on which this device is running on")
                    .expertAccess().readOnly().commit()
                    ,
            STRING_ELEMENT(expected).key("deviceId")
                    .displayedName("DeviceID").description("The device instance ID uniquely identifies a device instance in the distributed system")
                    .readOnly().commit()
                    ,
            BOOL_ELEMENT(expected).key("archive")
                        .displayedName("Archive")
                        .description("Decides whether the properties of this device will be logged or not")
                        .reconfigurable()
                        .assignmentOptional().defaultValue(True)
                        .commit()
                        ,
            INT32_ELEMENT(expected).key("progress")
                    .displayedName("Progress").description("The progress of the current action")
                    .readOnly().initialValue(0).commit()
                    ,
            STRING_ELEMENT(expected).key("state")
                    .displayedName("State").description("The current state the device is in")
                    .assignmentOptional().defaultValue("uninitialized").readOnly().commit()
                    ,
            NODE_ELEMENT(expected).key("Logger")
                    .description("Logging settings")
                    .displayedName("Logger")
                    .appendParametersOfConfigurableClass(Logger,"Logger")
                    .expertAccess()
                    .commit()
                    ,
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
        
        # Setup the validation classes
        self.validatorIntern   = Validator()
        self.validatorExtern   = Validator()
        rules = ValidatorValidationRules()
        rules.allowAdditionalKeys = False
        rules.allowMissingKeys    = True
        rules.allowUnrootedConfiguration = True
        rules.injectDefaults = False
        rules.injectTimestamps = True
        self.validatorIntern.setValidationRules(rules)
        self.validatorExtern.setValidationRules(rules)

        # Instantiate SignalSlotable object without starting event loop
        try:
            self._ss = SignalSlotable.create(self.deviceid)    #, "Jms", self.parameters["connection.Jms"], autostart = False
        except RuntimeError as e:
            raise RuntimeError(
                "PythonDevice.__init__: SignalSlotable.create Exception -- " +
                str(e))
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
    
    @property
    def signalSlotable(self):
        '''Get SignalSlotable object embeded in PythonDevice instance.'''
        return self._ss
    
    def loadLogger(self,input):
        config = input["Logger"]

        # make a copy of additional appenders defined by user
        appenders = config["appenders"]
        config["appenders[2].Network.layout"] = Hash()
        config["appenders[2].Network.layout.Pattern.format"] = "%d{%F %H:%M:%S} | %p | %c | %m"
        if "connection" in input:
            config["appenders[2].Network.connection"] = input["connection"]
#        print "loadLogger final:\n", config
        Logger.configure(config)
        
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
        self.startFsm()
        with self._stateChangeLock:
            validated = self.validatorIntern.validate(self.fullSchema, self.parameters)
            self.parameters.merge(validated, HashMergePolicy.REPLACE_ATTRIBUTES)
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
        
    def set(self, *args):
        """
        Updates the state of the device. This function automatically notifies any observers.
        This function supports 3 args: key, value, timestamp or 2 arg: hash, timestamp
        If 1 or more than 3 arguments, it does nothing
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
                if isCpuImage(value):
                    self._setImage(key, value)
                    return;
                elif type(value) is RawImageData:
                    self._setRawImageData(key, value)
                    return
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
                    if isCpuImage(value):
                        self._setImage(key, value)
                        return
                    elif type(value) is RawImageData:
                        self._setRawImageData(key, value)
                        return
                    pars = tuple([Hash(key,value), Timestamp()])
                hash, stamp = pars
                # Check that hash is image's free
                paths = hash.getPaths()
                for key in paths:
                    value = hash[key]
                    if isCpuImage(value):
                        self._setImage(key, value)    # process images individually
                        hash.erasePath(key)      # clear hash from images 
                    elif type(value) is RawImageData:
                        self._setRawImageData(key, value)
                        hash.erasePath(key)
        
                try:
                    validated = self.validatorIntern.validate(self.fullSchema, hash, stamp)
                except RuntimeError as e:
                    print("Validation Exception (Intern): " + str(e))
                    raise RuntimeError("Validation Exception: " + str(e))

                #if self.validatorIntern.hasParametersInWarnOrAlarm():
                #    warnings = self.validatorIntern.getParametersInWarnOrAlarm()
                #    for key in warnings:
                #        desc = warnings[key]
                #        self.log.WARN(desc["message"])
                #        self._ss.emit("signalNotification", desc["type"], desc["message"], "", self.deviceid)

                if not validated.empty():
                    self.parameters.merge(validated, HashMergePolicy.REPLACE_ATTRIBUTES)
                    self._ss.emit("signalChanged", validated, self.deviceid)
       
    def __setitem__(self, key, value):
        self.set(key, value, Timestamp())
        
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
        validated = validator.validate(schema, Hash())
        with self._stateChangeLock:
            for path in self._injectedSchema.getPaths():
                if self.parameters.has(path) and not self.staticSchema.has(path): 
                    self.parameters.erase(path)
            self._stateDependentSchema = {}
            self._injectedSchema.copy(schema)
            self.fullSchema.copy(self.staticSchema)
            self.fullSchema += self._injectedSchema
            #self.parameters.merge(validated, HashMergePolicy.REPLACE_ATTRIBUTES)
            #validated = self.validatorIntern.validate(self.fullSchema, self.parameters)
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
        validated = validator.validate(schema, self.parameters)
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
        return self.fullSchema.aliasHasKey(key)
    
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
        self.log.ERROR("Error Found Action: {} -- {}".format(shortMessage, detailedMessage))
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
        self.log.DEBUG("updateState: {}".format(currentState))
        if self["state"] != currentState:
            self["state"] = currentState
            if self.errorRegex.match(currentState) is not None:
                self._ss.updateInstanceInfo(Hash("status", "error"))
            else:
                if self._ss.getInstanceInfo()["status"] == "error":
                    self._ss.updateInstanceInfo(Hash("status", "ok"))
        self._ss.reply(currentState)  # reply new state to interested event initiators

    def onStateUpdate(self, currentState):
        print("onStateUpdate() is deprecated, use updateState() instead")
        self.updateState(currentState)

    def exceptionFound(self, shortMessage, detailedMessage):
        self.log.ERROR(shortMessage)
        self._ss.emit("signalNotification", "EXCEPTION", shortMessage,
                      detailedMessage, self.deviceid)

    def noStateTransition(self):
        self._ss.emit("signalNoTransition", "No state transition possible", self.deviceid)
        
    def _initDeviceSlots(self):
        #-------------------------------------------- register intrinsic signals
        self._ss.registerSignal("signalChanged", Hash, str)                # changeHash, instanceId
        self._ss.connect("", "signalChanged", "*", "slotChanged", ConnectionType.NO_TRACK, False)
        
        self._ss.registerSignal("signalNoTransition", str, str)                 # 
        self._ss.connect("", "signalNoTransition", "*", "slotNoTransition", ConnectionType.NO_TRACK, False)        
        
        self._ss.registerSignal("signalNotification", str, str, str, str)     # type, shortMessage, detailedMessage, deviceId
        self._ss.connect("", "signalNotification", "*", "slotNotification", ConnectionType.NO_TRACK, False)
        
        self._ss.registerSignal("signalSchemaUpdated", Schema, str)           # schema, deviceid
        self._ss.connect("", "signalSchemaUpdated", "*", "slotSchemaUpdated", ConnectionType.NO_TRACK, False)                
        
        #---------------------------------------------- register intrinsic slots
        self._ss.registerSlot(self.slotReconfigure)        
        self._ss.registerSlot(self.slotGetConfiguration)
        self._ss.registerSlot(self.slotGetSchema)
        self._ss.registerSlot(self.slotKillDevice)        

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
            except Exception as e:
                print("PythonDevice.slotReconfigure Exception:", str(e))
                self.errorFound("Python Exception happened", str(e))
                self._ss.reply(False, str(e))
                return
            self._applyReconfiguration(validated)
        self._ss.reply(result, error)
    
    def _validate(self, unvalidated):
        currentState = self["state"]
        whiteList = self._getStateDependentSchema(currentState)
        self.log.DEBUG("Incoming (un-validated) reconfiguration:\n{}".format(unvalidated))
        try:
            validated = self.validatorExtern.validate(whiteList, unvalidated)
        except RuntimeError as e:
            errorText = str(e) + " in state: \"" + currentState + "\""
            return (False, errorText, unvalidated)
        self.log.DEBUG("Validated reconfiguration:\n{}".format(validated))
        return (True,"",validated)
    
    def _applyReconfiguration(self, reconfiguration):
        with self._stateChangeLock:
            self.parameters += reconfiguration
        self.log.DEBUG("After user interaction:\n{}".format(reconfiguration))
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
        senderid = self.serverid # getSenderInfo("slotKillDevice").getInstanceIdOfSender()
        if senderid == self.serverid: 
            self.log.INFO("Device is going down as instructed by server")
            self.preDestruction()
            self.stopEventLoop()
        else:
            self.log.INFO("Device is going down as instructed by \"{}\"".format(senderid))
            self._ss.call(self.serverid, "slotDeviceGone", self.deviceid)
            self.stopEventLoop()
   
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
    script, modname, classid, xmlfile = tuple(sys.argv)
    config = PythonDevice.loadConfiguration(xmlfile)
   
    try:
        device = Configurator(PythonDevice).create(classid, config)
        device.run()
        device.__del__()
    except Exception as e:
        print("Exception caught: " + str(e))
    os._exit(77)
    
