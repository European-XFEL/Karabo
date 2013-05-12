import threading
import os
import time
import datetime

# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="esenov"
__date__ ="$Jul 30, 2012 9:03:51 PM$"

import sys
import socket
from abc import ABCMeta, abstractmethod
from libkarathon import *
from fsm import *
from karabo_decorators import *
from core_exceptions import ParameterException

@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("PythonBaseDevice", "1.0")
class PythonBaseDevice(object):
    __metaclass__ = ABCMeta
    
    def __init__(self):
        super(PythonBaseDevice, self).__init__()
        
    @abstractmethod
    def run(self):
        pass
    
    @abstractmethod
    def getCurrentConfiguration(self):
        return Hash()
    

@KARABO_CLASSINFO("PythonDevice", "1.0")
class PythonDevice(PythonBaseDevice, BaseFsm):

    instanceCountPerDeviceServer = dict()
    instanceCountLock = threading.Lock()
    
    @staticmethod
    def expectedParameters(expected):

        e = STRING_ELEMENT(expected).key("version")
        e.displayedName("Version").description("The version of this device class")
        e.advanced().readOnly().initialValue(PythonDevice.__version__).commit()
        
        e = CHOICE_ELEMENT(expected).key("connection")
        e.displayedName("Connection").description("The connection to the communication layer of the distributed system")
        e.appendNodesOfConfigurationBase(BrokerConnection)
        e.assignmentOptional().defaultValue("Jms").init().advanced().commit()
        
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
        
    def __init__(self, configuration, *args, **kwargs):
        super(PythonDevice, self).__init__(*args, **kwargs)

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
        logging_config = Hash("categories[0]", Hash("Category.name", self._deviceId, "Category.priority", "DEBUG"),
                    "appenders[0].Ostream.layout", "Pattern")
        Logger.configure(logging_config)
        self.log = Logger.getLogger(self._deviceId)
        
        # Instantiate connection
        self._ss = SignalSlotable.create(self._deviceId, "Jms", self.parameters["connection.Jms"])
        
        # Initialize FSM
        self.initFsmSlots(self._ss)
        
        # Initialize Device slots
        self.initDeviceSlots()
        
    def initDeviceSlots(self):
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
        self._ss.registerSlot(self.slotReconfigure, Hash)
        self._ss.registerSlot(self.slotRefresh)
        self._ss.registerSlot(self.slotGetSchema, bool)
        self._ss.registerSlot(self.slotKillDeviceInstance)
        
    def errorFound(self, shortMessage, detailedMessage):
        pass
    
    def errorFoundAction(self, shortMessage, detailedMessage):
        self.triggerErrorFound(shortMessage, detailedMessage)
    
    def noStateTransition(self):
        self._ss.emit("signalNoTransition", "No transition possible", self._deviceId)
        
    def onStateUpdate(self, currentState):
        self.set("state", currentState)
        self._ss.reply(currentState)
    
    def run(self):
        self.classid = self.__class__.__classid__
        self.staticSchema = getSchema(self.classid)
        self.fullSchema = self.staticSchema
        
        fullHostName = socket.gethostname()
        self.hostName, dotsep, domainName = fullHostName.partition('.')
        self.running = True
        self.startFsm()
        while self.running:
            time.sleep(3)
            
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
            if len(pars) == 3:
                key, value, timestamp = pars
                pars = (Hash(key, value), timestamp,)
            
            if len(pars) == 2:
                # hash arg
                hash, timestamp = pars
                validated = Hash()
                try:
                    validated = self.validatorIntern.validate(self.fullSchema, hash)
                except RuntimeError,e:
                    raise RuntimeError,"Validation Exception: " + str(e)
                if self.validatorIntern.hasParametersInWarnOrAlarm():
                    warnings = self.validatorIntern.getParametersInWarnOrAlarm()
                    for key in warnings:
                        self.log.WARN(warnings[key]["message"])
                        #TODO trigger warnOrAlarm
                        
                if not validated.empty():
                    self.parameters += validated
                    self._ss.emit("signalChanged", validated, self.getInstanceId(), self.classid)

    def get(self,key):
        with self._lock:
            try:
                return self.parameters[key]
            except RuntimeError,e:
                raise AttributeError,"Error while retrieving '" + key + "' from device"
      
    def getFullSchema(self):
        return self.fullSchema
        
    def _injectSchema(self, schema):
        with self._stateChangeLock:
            self._stateDependentSchema = dict()
            self._injectedExpectedParameters = schema
   
    def updateSchema(self, schema):
        print "Update Schema requested"
        self._injectSchema(schema)
        print "Injected..."
        xsd = self.__class__.convertToXsd(self.getFullSchema())
        print "Serialized..."
        self._ss.emit("signalSchemaUpdated", xsd, self._devInstId, self._classId)
        self.log.INFO("Schema updated")
    
    def getCurrentConfiguration(self):
        return self.parameters
    
    def getServerId(self):
        return self.serverid
        
    def reconfigure(self, instanceId, configuration):
        self._ss.call(instanceId, "slotReconfigure", configuration)
    
    def slotKillDeviceInstance(self):
        self.log.INFO("Device is going down...")
        self.onKill()
        self._ss.emit("signalDeviceInstanceGone", self._devSrvInstId, self._devInstId)
        self.stopEventLoop()
        self.log.INFO("dead.")
   
    def slotRefresh(self):
        all = Hash(self._initialParameters)
        all.update(self._reconfigurableParameters)
        all.update(self._monitoredParameters)
        self._ss.emit("signalChanged", all, self._devInstId, self._classId);
        self._ss.reply(all);
       
    def slotReconfigure(self, newConfiguration):
        if newConfiguration.empty():
            return
        result, error = self._validate(newConfiguration)
        if result:
            try:
                self.onReconfigure(self._incomingValidatedReconfiguration)
            except Exception,e:
                self._ss.reply(False, str(e))
                self.triggerErrorFound("Python Exception happened", str(e))
                return
            self._applyReconfiguration(self._incomingValidatedReconfiguration)
        self._ss.reply(result, error)
    
    def _validate(self, newConfiguration):
        currentState = self.get("state")
        whiteList = self._getStateDependentSchema(currentState)
        config = Hash(self._classId, newConfiguration)
        try:
            config = whiteList.validate(config, False, False, False, True)
        except RuntimeError, e:
            errorText = e + " in state: \"" + currentState + "\""
            return (False, errorText,)
        self._incomingValidatedReconfiguration = config.get(self._classId)
        return (True, "",)
    
    def _getStateDependentSchema(self, state):
        with self._stateDependentSchemaLock:
            if state in self._stateDependentSchema:
                return self._stateDependentSchema[state]
            self._stateDependentSchema[state] = self.__class__.getExpectedParameters(AccessType(WRITE), state)
            if not self._injectedExpectedParameters.empty():
                self._stateDependentSchema[state].addExternalSchema(self._injectedExpectedParameters)
            return self._stateDependentSchema[state]
    
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
            self._ss.reply(self._allExpectedParameters)
   
    def onReconfigure(self, inputConfig): pass

    def onKill(self): pass
    
    def _generateDefaultDeviceInstanceId(self):
        cls = self.__class__
        with cls.instanceCountLock:
            if self._devSrvInstId not in cls.instanceCountPerDeviceServer:
                cls.instanceCountPerDeviceServer[self._devSrvInstId] = 0
            cls.instanceCountPerDeviceServer[self._devSrvInstId] += 1
            _index = cls.instanceCountPerDeviceServer[self._devSrvInstId]
            if self._devSrvInstId == "":
                #myHostName, someList, myHostAddrList = socket.gethostbyaddr(socket.gethostname())
                myFullHostName = socket.gethostname()
                myHostName, dotsep, domainName = myFullHostName.partition('.')
                return myHostName + "/" + self._classId + "/" + str(_index)
            tokens = self._devSrvInstId.split("/")
            _domain = tokens.pop(0) + "-" + tokens.pop()
            _id = _domain + "/" + self._classId + "/" + str(_index)
            return _id
            
    def checkWarningsAndAlarms(self, key, value): pass
    
    def _increaseInstanceCount(self):
        cls = self.__class__
        with cls.instanceCountLock:
            cls.instanceCountPerDeviceServer[self._devSrvInstId] += 1
    
    def _decreaseInstanceCount(self):
        cls = self.__class__
        with cls.instanceCountLock:
            cls.instanceCountPerDeviceServer[self._devSrvInstId] -= 1
    
    def getCurrentDateTime(self):
        dt = datetime.datetime(1,1,1).today()
        return dt.isoformat(' ')
    
    def triggerErrorFound(self, shortMessage, detailedMessage):
        self._ss.emit(
            "signalErrorFound", self.getCurrentDateTime(), shortMessage,
            detailedMessage, self._devInstId)

    def triggerWarning(self, warningMessage, priority):
        self._ss.emit(
            "signalWarning", self.getCurrentDateTime(), warningMessage,
            self._devInstId, priority)
    
    def triggerAlarm(self, alarmMessage, priority):
        self._ss.emit(
            "signalAlarm", self.getCurrentDateTime(), alarmMessage,
            self._devInstId, priority)

    @staticmethod
    def parseCommandLine(args):
        script, xmlfile = tuple(args)
        input = InputHash.create("TextFile", Hash("filename", xmlfile))
        hash = Hash()
        input.read(hash)
        return (hash["module"], hash["classId"], hash["configuration"],)
 
def launchPythonDevice():
    modname, classid, configuration = PythonDevice.parseCommandLine(sys.argv)
    module = __import__(modname)
    device = PythonDevice.create(classid, configuration)
    device.run()
    
