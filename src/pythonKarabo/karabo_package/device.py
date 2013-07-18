__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Jul 30, 2012 9:03:51 PM$"

import threading
import os
import time
import datetime
import sys
import socket
from abc import ABCMeta, abstractmethod
from karabo.karathon import *
from karabo.decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from karabo.base_fsm import BaseFsm


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("PythonDevice", "1.0")
class PythonDevice(BaseFsm):

    instanceCountPerDeviceServer = dict()
    instanceCountLock = threading.Lock()
    
    @staticmethod
    def expectedParameters(expected):

        e = STRING_ELEMENT(expected).key("version")
        e.displayedName("Version").description("The version of this device class")
        e.advanced().readOnly().initialValue(PythonDevice.__version__).commit()
        
        e = INT32_ELEMENT(expected).key("visibility")
        e.displayedName("Visibility").description("Configures who is allowed to see this device at all")
        e.assignmentOptional().defaultValue(AccessLevel(OBSERVER))
        e.advanced().reconfigurable().commit()
        
        e = STRING_ELEMENT(expected).key("classId")
        e.displayedName("ClassID").description("The (factory)-name of the class of this device")
        e.advanced().readOnly().initialValue(PythonDevice.__classid__).commit()
        
        e = STRING_ELEMENT(expected).key("serverId")
        e.displayedName("ServerID").description("The device-server on which this device is running on")
        e.advanced().assignmentInternal().noDefaultValue().init().commit()

        e = STRING_ELEMENT(expected).key("deviceId")
        e.displayedName("DeviceID").description("The device instance ID uniquely identifies a device instance in the distributed system")
        e.assignmentOptional().noDefaultValue().init().commit()
            
        e = STRING_ELEMENT(expected).key("state")
        e.displayedName("State").description("The current state the device is in")
        e.assignmentOptional().defaultValue("uninitialized").readOnly().commit()
        
        e = NODE_ELEMENT(expected).key("Logger").displayedName("Logger")
        e.description("Logging settings").advanced()
        e.appendParametersOfConfigurableClass(Logger, "Logger").commit()
    
        e = OVERWRITE_ELEMENT(expected).key("Logger.appenders")
        e.setNewDefaultValue("Ostream").commit()
        
        e = OVERWRITE_ELEMENT(expected).key("Logger.appenders.Ostream.layout")
        e.setNewDefaultValue("Pattern").commit()
        
        e = OVERWRITE_ELEMENT(expected).key("Logger.appenders.Ostream.layout.Pattern.format")
        e.setNewDefaultValue("%p  %c  : %m%n").commit()
        
    def __init__(self, configuration):
        if configuration is None:
            raise ValueError,"Configuration must be Hash object, not None"
        #print "PythonDevice constructor: Input configuration after being validated is ...\n", configuration
        super(PythonDevice, self).__init__(configuration)

        self.parameters = configuration
        if "serverId" in self.parameters:
            self.serverid = self.parameters["serverId"]
        else:
            self.serverid = "__none__"    
        
        if "deviceId" in self.parameters:
            self.deviceid = self.parameters["deviceId"]
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
        self._ss = SignalSlotable.create(self.deviceid)    #, "Jms", self.parameters["connection.Jms"], autostart = False
        
        # Setup device logger
        #logcfg = Hash("categories[0]", Hash("Category.name", self.deviceid, "Category.priority", "DEBUG"),
        #            "appenders[0].Ostream.layout", "Pattern")
        logcfg = configuration["Logger"]
        logcfg["categories[0].Category.name"] = self.deviceid
        logcfg["categories[0].Category.appenders[0].Ostream.layout.Pattern.format"] = "%p  %c  : %m%n"
        logcfg["categories[0].Category.additivity"] = False
        logcfg["categories[0].Category.appenders[1].Network.layout.Pattern.format"] = "%d{%F %H:%M:%S} | %p | %c | %m"
        if "connection" in configuration:
            logcfg["categories[0].Category.appenders[1].Network.connection"] = configuration["connection"]
        #logcfg["priority"] = "DEBUG"
        Logger.configure(logcfg)
        self.log = Logger.getLogger(self.deviceid)
        
        # Initialize FSM slots for user defined FSM (polymorphic call) 
        self.initFsmSlots(self._ss)
        
        # Initialize Device slots
        self._initDeviceSlots()
        
    def run(self):
        self.initClassId()
        self.initSchema()
        self.startFsm()
        with self._stateChangeLock:
            validated = self.validatorIntern.validate(self.fullSchema, self.parameters)
            self.parameters.merge(validated, HashMergePolicy.REPLACE_ATTRIBUTES)
            
        # Create 'info' hash
        info = Hash("type", "device")
        info["classId"] = self.classid
        info["serverId"] = self.serverid
        info["visibility"] = self["visibility"]
        info["version"] = self.__class__.__version__
        info["host"] = self.hostname
        #... add here more info entries if needed
        
        # Run event loop ( in a thread ) with given info
        # TODO Make configurable
        self._ss.runEventLoop(10, info) # block while SignalSlotable event loop running 
            
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
    
    def set(self, *args):
        """
        Updates the state of the device. This function automatically notifies any observers.
        This function supports 3 args: key, value, timestamp or 2 arg: hash, timestamp
        If 1 or more than 3 arguments, it does nothing
        """
        pars = tuple(args)
        with self._stateChangeLock:
            # key, value args
            if len(pars) == 2:
                key, value = pars
                pars = (Hash(key, value),)
            
            if len(pars) == 1:
                # hash arg
                hash = pars[0]
                try:
                    validated = self.validatorIntern.validate(self.fullSchema, hash)
                except RuntimeError,e:
                    print "Validation Exception (Intern): " + str(e)
                    raise RuntimeError,"Validation Exception: " + str(e)

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
        self.set(key, value)
        
    def get(self,key):
        with self._stateChangeLock:
            try:
                return self.parameters[key]
            except RuntimeError,e:
                raise AttributeError,"Error while retrieving '" + key + "' from device"
            
    def __getitem__(self, key):
        return self.get(key)
    
    def getFullSchema(self):
        return self.fullSchema
        
    def updateSchema(self, schema):
        self.log.DEBUG("Update Schema requested")
        self._injectSchema(schema)
        self.log.DEBUG("Injected...")
        # notify the distributed system...
        self._ss.emit("signalSchemaUpdated", self.fullSchema, self.deviceid)
        self.log.INFO("Schema updated")
    
    def setProgress(self, value, associatedText = ""):
        v = self.progressMin + value / (self.progressMax - self.progressMin)
        self._ss.emit("signalProgressUpdated", v, associatedText, self.deviceid)
    
    def resetProgress(self):
        self._ss.emit("signalProgressUpdated", self.progressMin, "")
    
    def setProgressRange(self, minimum, maximum):
        self.progressMin, self.progressMax = minimum, maximum
    
    def getAliasFromKey(self, key, aliasReferenceType):
        try:
            return self.fullSchema.getAliasFromKey(key, aliasReferenceType)
        except RuntimeError,e:
            raise AttributeError,"Error while retrieving alias from parameter (" + key + "): " + str(e)
        
    def getKeyFromAlias(self, alias):
        try:
            return self.fullSchema.getKeyFromAlias(alias)
        except RuntimeError,e:
            raise AttributeError,"Error while retrieving parameter from alias (" + str(alias) + "): " + str(e)
    
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
            return HashFilter.byTag(self.fullSchema, self.parameters, tags)
    
    def filterByTags(self, configuration, tags):
        return HashFilter.byTag(self.fullSchema, configuration, tags)
    
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
        self.fullSchema = self.staticSchema
        
    def onStateUpdate(self, currentState):
        self.log.DEBUG("onStateUpdate: {}".format(currentState))
        if self["state"] != currentState:
            self["state"] = currentState
        self._ss.reply(currentState)  # reply new state to interested event initiators
    
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
        
        self._ss.registerSignal("signalProgressUpdated", int, str, str)         # Progress value [0,100], label, deviceid
        self._ss.connect("", "signalProgressUpdated", "*", "slotProgressUpdated", ConnectionType.NO_TRACK, False)
        
        #---------------------------------------------- register intrinsic slots
        self._ss.registerSlot(self.slotReconfigure)
        self._ss.registerSlot(self.slotRefresh)
        self._ss.registerSlot(self.slotGetConfiguration)
        self._ss.registerSlot(self.slotGetSchema)
        self._ss.registerSlot(self.slotKillDevice)
        self._ss.registerSlot(self.errorFound)

    def triggerError(self, s, d):
        self._ss.call("", "errorFound", s, d)
        
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
            raise AttributeError,"Number of command parameters should not exceed 4"
        
    def slotRefresh(self):
        self._ss.emit("signalChanged", self.parameters, self.deviceid);
        self._ss.reply(self.parameters);
    
    def slotGetConfiguration(self):
        self._ss.reply(self.parameters)
        
    def slotReconfigure(self, newConfiguration):
        if newConfiguration.empty():
            return
        result, error, validated = self._validate(newConfiguration)
        if result:
            try:
                self.preReconfigure(validated)
            except Exception,e:
                print "PythonDevice.slotReconfigure Exception:", str(e)
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
        except RuntimeError,e:
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
        if onlyCurrentState:
            currentState = self["state"]
            self._ss.reply(self._getStateDependentSchema(currentState))
        else:
            self._ss.reply(self.fullSchema)
   
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
    
    def _injectSchema(self, schema):
        with self._stateChangeLock:
            self._stateDependentSchema = {}
            self._injectedSchema = schema
            self.fullSchema += self._injectedSchema
            
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
        input = InputHash.create("TextFile", Hash("filename", xmlfile))
        hash = Hash()
        input.read(hash)
        return hash
 
def launchPythonDevice():
    script, modname, classid, xmlfile = tuple(sys.argv)
    config = PythonDevice.loadConfiguration(xmlfile)
    if classid in config:
        configuration = config[classid]
    else:
        configuration = Hash()
    #print "launchPythonDevice: input configuration before being validated is ...\n", configuration
    try:
        module = __import__(modname)
        device = PythonDevice.create(classid, configuration)
        device.run()
    except Exception,e:
        print "Exception caught: " + str(e)
    
