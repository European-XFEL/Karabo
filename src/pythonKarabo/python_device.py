__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Jul 30, 2012 9:03:51 PM$"

import threading
import os
import time
import datetime
import sys
import socket
from abc import ABCMeta, abstractmethod
from libkarathon import *
from fsm import *
from karabo_decorators import *
from base_fsm import *


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
        
        e = VECTOR_STRING_ELEMENT(expected).key("visibility")
        e.displayedName("Visibility").description("Configures who is allowed to see this device at all")
        e.assignmentOptional().defaultValueFromString("")
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
        
    def __init__(self, configuration):
        if configuration is None:
            raise ValueError,"Configuration must be Hash object, not None"
        print "PythonDevice configuration\n", configuration
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
        
        # Setup the validation classes
        rules = ValidatorValidationRules()
        rules.allowAdditionalKeys = False
        rules.allowMissingKeys    = True
        rules.allowUnrootedConfiguration = True
        rules.injectDefaults = False
        
        self.validatorIntern   = Validator()
        rules.injectTimestamps = True
        self.validatorIntern.setValidationRules(rules)
        
        self.validatorExtern   = Validator()
        rules.injectTimestamps = False
        self.validatorExtern.setValidationRules(rules)
        
        # Setup device logger
        logging_config = Hash("categories[0]", Hash("Category.name", self.deviceid, "Category.priority", "DEBUG"),
                    "appenders[0].Ostream.layout", "Pattern")
        Logger.configure(logging_config)
        self.log = Logger.getLogger(self.deviceid)
        
        # Instantiate connection
        self._ss = SignalSlotable.create(self.deviceid)    #, "Jms", self.parameters["connection.Jms"])
        
        # Initialize FSM slots for user defined FSM (polymorphic call) 
        self.initFsmSlots(self._ss)
        
        # Initialize Device slots
        self._initDeviceSlots()
        
    def run(self):
        self.initClassId()
        self.initSchema()
        
        self._ss.registerSignal("signalNewDeviceInstanceAvailable", str, Hash)  # DeviceServerInstanceId, currentConfig
        self._ss.connect("", "signalNewDeviceInstanceAvailable", "*", "slotNewDeviceInstanceAvailable", ConnectionType.NO_TRACK, False)
        self._ss.emit("signalNewDeviceInstanceAvailable", self.serverid, Hash(self.classid, self.parameters))
        
        fullHostName = socket.gethostname()
        self.hostName, dotsep, domainName = fullHostName.partition('.')
        self.running = True
        self.startFsm()
        while self.running:
            time.sleep(1)
            
    def stopEventLoop(self):
        self.running = False
    
    def __del__(self):
        ''' PythonDevice destructor '''
        # Destroying device instance means finishing subprocess runnung a device instance.
        # Call exit for child processes (os._exit(...)) to shutdown properly a SignalSlotable object
        os._exit(0)   

    def remote(self):
        if self._client is None:
            self._client = DeviceClient()  # connectionType="Jms" config=Hash()
        return self._client
    
    def set(self, *args):
        """
        Updates the state of the device. This function automatically notifies any observers.
        This function supports 3 args: key, value, timestamp or 2 arg: hash, timestamp
        If 1 or more than 3 arguments, it does nothing
        """
        pars = tuple(args)
        with self._lock:
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
                #        self.log.WARN(warnings[key]["message"])
                #        #TODO trigger warnOrAlarm

                if not validated.empty():
                    self.parameters += validated
                    self._ss.emit("signalChanged", validated, self.getInstanceId(), self.classid)

    def __setitem__(self, key, value):
        self.set(key, value)
        
    def get(self,key):
        with self._lock:
            try:
                return self.parameters[key]
            except RuntimeError,e:
                raise AttributeError,"Error while retrieving '" + key + "' from device"
            
    def __getitem__(self, key):
        return self.get(key)
    
    def getFullSchema(self):
        return self.fullSchema
        
    def updateSchema(self, schema):
        print "Update Schema requested"
        self._injectSchema(schema)
        print "Injected..."
        xsd = self.__class__.convertToXsd(self.getFullSchema())
        print "Serialized..."
        self._ss.emit("signalSchemaUpdated", xsd, self.deviceid, self.classid)
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
        
    def key2alias(self, key, aliasReferenceType):
        return self.getAliasFromKey(key, aliasReferenceType)
    
    def getKeyFromAlias(self, alias):
        try:
            return self.fullSchema.getKeyFromAlias(alias)
        except RuntimeError,e:
            raise AttributeError,"Error while retrieving parameter from alias (" + str(alias) + "): " + str(e)
    
    def alias2key(self, alias):
        return self.getKeyFromAlias(alias)
    
    def aliasHasKey(self, alias):
        return self.fullSchema.aliasHasKey(key)
    
    def keyHasAlias(self, key):
        return self.fullSchema.keyHasAlias(key)
        
    def getValueType(self, key):
        return self.fullSchema.getValueType(key)
    
    def getCurrentConfiguration(self):
        return self.parameters
    
    def getServerId(self):
        return self.serverid

    # In C++: the following functions are protected
    
    def errorFoundAction(self, shortMessage, detailedMessage):
        print "Error Found Action: {} -- {}".format(shortMessage, detailedMessage)
    
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
        self.set("state", currentState)
        self._ss.reply(currentState)
    
    def noStateTransition(self):
        self._ss.emit("signalNoTransition", "No transition possible", self.deviceid)
        
    def _initDeviceSlots(self):
        #-------------------------------------------- register intrinsic signals
        self._ss.registerSignal("signalChanged", Hash, str, str)                # changeHash, instanceId, classId
        self._ss.connect("", "signalChanged", "*", "slotChanged", ConnectionType.NO_TRACK, False)
        
        self._ss.registerSignal("signalErrorFound", str, str, str, str)         # timeStamp, shortMsg, longMsg, instanceId
        self._ss.connect("", "signalErrorFound", "*", "slotErrorFound", ConnectionType.NO_TRACK, False)
        
        self._ss.registerSignal("signalBadReconfiguration", str, str)           # shortMsg, instanceId
        self._ss.connect("", "signalBadReconfiguration", "*", "slotBadReconfiguration", ConnectionType.NO_TRACK, False)        
        
        self._ss.registerSignal("signalNoTransition", str, str)                 # 
        self._ss.connect("", "signalNoTransition", "*", "slotNoTransition", ConnectionType.NO_TRACK, False)        
        
        self._ss.registerSignal("signalWarningOrAlarm", str, str, str, str)     # timeStamp, warnMsg, instanceId, priority
        self._ss.registerSignal("signalWarning", str, str, str, str)            # timeStamp, warnMsg, instanceId, priority
        self._ss.connect("", "signalWarning", "*", "slotWarning", ConnectionType.NO_TRACK, False)
        
        self._ss.registerSignal("signalAlarm", str, str, str, str)              # timeStamp, alarmMsg, instanceId, priority
        self._ss.connect("", "signalAlarm", "*", "slotAlarm", ConnectionType.NO_TRACK, False)
        
        self._ss.registerSignal("signalSchemaUpdated", str, str, str)           # schema, instanceId, classId
        self._ss.connect("", "signalSchemaUpdated", "*", "slotSchemaUpdated", ConnectionType.NO_TRACK, False)

        # TODO Deprecate!
        self._ss.registerSignal("signalDeviceInstanceGone", str, str)           # DeviceServerInstanceId, deviceInstanceId
        self._ss.connect("", "signalDeviceInstanceGone", "*", "slotDeviceInstanceGone", ConnectionType.NO_TRACK, False)
        
        self._ss.registerSignal("signalProgressUpdated", int, str, str)         # Progress value [0,100], label, deviceInstanceId
        self._ss.connect("", "signalProgressUpdated", "*", "slotProgressUpdated", ConnectionType.NO_TRACK, False)
        
        #---------------------------------------------- register intrinsic slots
        self._ss.registerSlot(self.slotReconfigure)
        self._ss.registerSlot(self.slotRefresh)
        self._ss.registerSlot(self.slotGetSchema)
        self._ss.registerSlot(self.slotKillDeviceInstance)
        
    def slotRefresh(self):
        self._ss.emit("signalChanged", self.parameters, self.deviceid, self.classid);
        self._ss.reply(self.parameters);
       
    def slotReconfigure(self, newConfiguration):
        if newConfiguration.empty():
            return
        validated = Hash()
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
        currentState = self.get("state")
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
        self._ss.emit("signalChanged", reconfiguration, self.deviceid, self.classid)
        self.postReconfigure()
    
    def slotGetSchema(self, onlyCurrentState):
        if onlyCurrentState:
            currentState = self.get("state")
            self._ss.reply(self._getStateDependentSchema(currentState))
        else:
            self._ss.reply(self.fullSchema)
   
    def slotKillDeviceInstance(self):
        self.log.INFO("Device is going down...")
        self.preDestruction()
        self._ss.emit("signalDeviceInstanceGone", self.serverid, self.deviceid)
        self.stopEventLoop()
        self.log.INFO("dead.")
   
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
        print "Device parseCommandLine hash\n",hash
        return hash
 
def launchPythonDevice():
    script, modname, classid, xmlfile = tuple(sys.argv)
    config = PythonDevice.loadConfiguration(xmlfile)
    if classid in config:
        configuration = config[classid]
    else:
        configuration = Hash()
    print "launchPythonDevice: configuration...\n", configuration
    try:
        module = __import__(modname)
        device = PythonDevice.create(classid, configuration)
        print "Device object created"
        device.run()
        print "device.run() returns..."
    except Exception,e:
        print "Exception caught: " + str(e)
    
