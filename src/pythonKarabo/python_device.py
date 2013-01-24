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
from libkarabo import *
from fsm import *
from karabo_decorators import schemamethod
from core_exceptions import ParameterException

def inheritanceChain(cls):
    """
    Class method allowing to generate a chain of class inheritance for given class
    till PythonDevice.  The class should be a subclass of PythonDevice
    """
    def classInChain(cls):
        while cls.__name__!='PythonDevice':
            yield cls
            (cls,) = cls.__bases__
        else:
            yield cls

    cList = list()
    if issubclass(cls,(PythonDevice,)):
        for c in classInChain(cls):
            cList.append(c)
        cList.reverse()
    return cList


inheritanceChain = classmethod(inheritanceChain)


class PythonDevice(object):

    __metaclass__ = ABCMeta
    instanceCountPerDeviceServer = dict()
    instanceCountLock = threading.Lock()
    
    def __init__(self, input):
        """
        The contructor covers functionality that in C++ provided by
        C++ contructor + configure method.
        Initialization of three containers of parameters (description + actual):
        initial, monitored and reconfigurable. 
        """
        cls = self.__class__
        self.running = True
        
        self._lock = threading.Lock()
        self._stateChangeLock = threading.Lock()
        self._stateDependentSchemaLock = threading.Lock()

        self._stateDependentSchema = dict()
        
        self.fsm = None
        self._client = None
        
        self._expectedInitialParameters        = cls.getExpectedInitialParameters()
        self._expectedReconfigurableParameters = cls.getExpectedReconfigurableParameters()
        self._expectedMonitoredParameters      = cls.getExpectedMonitorableParameters()
        self._allExpectedParameters            = cls.getFullExpectedParameters()
        
        self._injectedExpectedParameters = Schema()
        self._incomingValidatedReconfiguration = Hash()

        # Define an event that has to be used on exception
        KARABO_FSM_ON_EXCEPTION(self, 'onException')
                
        try:
            # classId
            self._classId = cls.__name__
            # devSrvInstId
            if input.has("devSrvInstId"):
                self._devSrvInstId = input.get("devSrvInstId")
            else:
                self._devSrvInstId = ""
            
            with cls.instanceCountLock:
                if self._devSrvInstId not in cls.instanceCountPerDeviceServer:
                    cls.instanceCountPerDeviceServer[self._devSrvInstId] = 0
            
            # Increase instance count
            self._increaseInstanceCount();

            # Construct needed for splitting the parameters (validate function needs this)
            tmp = Hash(self._classId, input)
            # devInstId
            if input.has("devInstId"):
                self._devInstId = input.get("devInstId")
            else:
                self._devInstId = self._generateDefaultDeviceInstanceId()
                tmp.setFromPath(self._classId + ".devInstId", self._devInstId)
            
            # Split the configuration parameters into three pots
            self._initialParameters = self._expectedInitialParameters.validate(tmp, True, False, True).get(self._classId)
            self._reconfigurableParameters = self._expectedReconfigurableParameters.validate(tmp, True, False, True).get(self._classId)
            self._monitoredParameters = self._expectedMonitoredParameters.validate(tmp, True, False, True).get(self._classId)
            print "\nInitialize Device SignalSlotable object...\n"
            self._ss = SignalSlotable(self._devInstId)
            print "\nInitialize Device Logging...\n"
            currentConfig = self.getCurrentConfiguration()
            connectionConfig = currentConfig.getFromPath(self._classId + ".connection")

            logConfig = Hash()
            logConfig.setFromPath("Logger.categories[0].Category.name", self._devInstId)
            logConfig.setFromPath("Logger.categories[0].Category.priority", "DEBUG")
            logConfig.setFromPath("Logger.categories[0].Category.appenders[0].Network.layout.Pattern.pattern", "%d{%F %H:%M:%S} | %p | %c | %m")
            logConfig.setFromPath("Logger.categories[0].Category.appenders[0].Network.connection", connectionConfig)
            Logger.create(logConfig).initialize()
            self.log = Logger.logger(self._devInstId)
            
            #-------------------------------------------------- register signals
            self._ss.registerSignal("signalErrorFound", str, str, str, str)         # timeStamp, shortMsg, longMsg, instanceId
            self._ss.registerSignal("signalBadReconfiguration", str, str)           # shortMsg, instanceId
            self._ss.registerSignal("signalNoTransition", str, str)                 # 
            self._ss.registerSignal("signalChanged", Hash, str, str)                # changeHash, instanceId, classId
            self._ss.registerSignal("signalWarning", str, str, str, str)            # timeStamp, warnMsg, instanceId, priority
            self._ss.registerSignal("signalAlarm", str, str, str, str)              # timeStamp, alarmMsg, instanceId, priority
            self._ss.registerSignal("signalSchemaUpdated", str, str, str)           # schema, instanceId, classId
            self._ss.registerSignal("signalDeviceInstanceGone", str, str)           # DeviceServerInstanceId, deviceInstanceId
            self._ss.registerSignal("signalNewDeviceInstanceAvailable", str, Hash)  # DeviceServerInstanceId, currentConfig
            #---------------------------------------------------  register slots
            self._ss.registerSlot(self.slotReconfigure, self)
            self._ss.registerSlot(self.slotRefresh, self)
            self._ss.registerSlot(self.slotGetSchema, self)
            self._ss.registerSlot(self.slotKillDeviceInstance, self)
            #---------------------------- define hard-coded (global) connections
            self._ss.connect("", "signalChanged", "*", "slotChanged", ConnectionType.NO_TRACK, False)
            self._ss.connect("", "signalBadReconfiguration", "*", "slotBadReconfiguration", ConnectionType.NO_TRACK, False)
            self._ss.connect("", "signalNoTransition", "*", "slotNoTransition", ConnectionType.NO_TRACK, False)
            self._ss.connect("", "signalErrorFound", "*", "slotErrorFound", ConnectionType.NO_TRACK, False)
            self._ss.connect("", "signalWarning", "*", "slotWarning", ConnectionType.NO_TRACK, False)
            self._ss.connect("", "signalAlarm", "*", "slotAlarm", ConnectionType.NO_TRACK, False)
            self._ss.connect("", "signalSchemaUpdated", "*", "slotSchemaUpdated", ConnectionType.NO_TRACK, False)
            self._ss.connect("", "signalDeviceInstanceGone", "*", "slotDeviceInstanceGone", ConnectionType.NO_TRACK, False)
            self._ss.connect("", "signalNewDeviceInstanceAvailable", "*", "slotNewDeviceInstanceAvailable", ConnectionType.NO_TRACK, False)
            
            self.log.INFO("Starting up {} on networkId {}".format(self._classId, self._devInstId))
            
            #myHostName, someList, myHostAddrList = socket.gethostbyaddr(socket.gethostname())
            myFullHostName = socket.gethostname()
            myHostName, dotsep, domainName = myFullHostName.partition('.')
            if self._devSrvInstId == myHostName:
                xsd = self.__class__.convertToXsd(self._allExpectedParameters)
                self._ss.call("*", "slotNewStandaloneDeviceInstanceAvailable", myHostName, tmp, self._ss.getInstanceId(), xsd)

            self.set("devClaId", self._classId)
        except:
            self.log.ERROR("Unexpected error: {}".format(sys.exc_info()[0]))
            raise
            
    def __del__(self):
        ''' PythonDevice destructor '''
        self._decreaseInstanceCount()
        # Destroying device instance means finishing subprocess runnung a device instance.
        # Call exit for child processes (os._exit(...)) to shutdown properly a SignalSlotable object
        os._exit(0)   

    @schemamethod
    def expectedParameters(expected):
        """
        This is a method for defining parameters descriptions. The implementation
        should be provided by user. This function has to be used via decorator like 
        schema = cls.expectedParameters() 
        """
        
        e = CHOICE_ELEMENT_BROKERCONNECTION(expected)
        e.key("connection").displayedName("Connection")
        e.description("The connection to the communication layer of the distributed system")
        e.assignmentOptional().defaultValue("Jms").init().advanced().commit()
        
        e = STRING_ELEMENT(expected)
        e.key("devSrvInstId").displayedName("Device-Server Instance Id")
        e.description("The device-server instance id, on which this device-instance is running on")
        e.assignmentInternal().defaultValue("").commit()

        e = STRING_ELEMENT(expected)
        e.key("devInstId").displayedName("Device Instance Id")
        e.description("Device Instance Id uniquely identifies a device instance in the distributed system")
        e.assignmentOptional().noDefaultValue().init().advanced().commit()
            
        e = STRING_ELEMENT(expected)
        e.key("devClaId").displayedName("Device Class Id")
        e.description("The (factory)-name of the class of this device").readOnly().commit()
            
        e = STRING_ELEMENT(expected)
        e.key("state").displayedName("State").description("The current state the device is in")
        e.assignmentOptional().defaultValue("uninitialized").readOnly().commit()
        
    def remote(self):
        if self._client is None:
            self._client = DeviceClient()  # connectionType="Jms" config=Hash()
        return self._client
    
    def postprocessing(self):
        ''' Finalizing the building of a device instance '''
        self._ss.emit("signalNewDeviceInstanceAvailable", self._devSrvInstId, self.getCurrentConfiguration())
        
    def set(self, *args):
        """
        Updates the state of the device. This function automatically notifies any observers.
        This function supports 2 args: key, value or 1 arg: hash
        If 0 or more than 2 arguments, it does nothing
        """
        pars = tuple(args)
        with self._lock:
            # key, value args
            if len(pars) == 2:
                key, value = pars
                try:
                    if self._expectedMonitoredParameters.hasKey(key):
                        self.checkWarningsAndAlarms(key,value)
                        if type(value) is bool:
                            self._monitoredParameters.setFromPathAsBool(key,value)
                        else:
                            self._monitoredParameters.setFromPath(key,value)
                    elif self._expectedReconfigurableParameters.hasKey(key):
                        if type(value) is bool:
                            print 'PythonDevice.set as bool the key=', key, ' and value=', value
                            self._reconfigurableParameters.setFromPathAsBool(key, value)
                        else:
                            self._reconfigurableParameters.setFromPath(key, value)
                    elif self._expectedInitialParameters.hasKey(key):
                        if type(value) is bool:
                            self._initialParameters.setFromPathAsBool(key, value)
                        else:
                            self._initialParameters.setFromPath(key, value)
                    else:
                        self.log.WARN("Illegal trial to set parameter ({})"
                            " which was not described in the expectedParameters section".format(key))
                    config = Hash()
                    if type(value) is bool:
                        config.setAsBool(key,value)
                    else:
                        config.set(key,value)
                    self._ss.emit("signalChanged", config, self._devInstId, self._classId)
                except RuntimeError,e:
                    self.log.ERROR("{}".format(e))
                return
            
            if len(pars) == 1:
                # hash arg
                hash = args[0]
                flat = hash.flatten()
                for key in flat.keys():
                    value = flat.getFromPath(key)
                    if self._expectedMonitoredParameters.hasKey(key):
                        self.checkWarningsAndAlarms(key, value)
                        if type(value) == bool:
                            self._monitoredParameters.setFromPathAsBool(key,value)
                        else:                        
                            self._monitoredParameters.setFromPath(key, value)
                    elif self._expectedReconfigurableParameters.hasKey(key):
                        if type(value) == bool:
                            self._reconfigurableParameters.setFromPathAsBool(key, value)
                        else:
                            self._reconfigurableParameters.setFromPath(key, value)
                    elif self._expectedInitialParameters.hasKey(key):
                        if type(value) == bool:
                            self._initialParameters.setFromPathAsBool(key, value)
                        else:
                            self._initialParameters.setFromPath(key, value)
                    else:
                        self.log.WARN("Illegal trial to set parameter ({})"
                            " which was not described in the expectedParameters section".format(key))
                self._ss.emit("signalChanged", flat.unflatten(), self._devInstId, self._classId)

    def get(self,key):
        with self._lock:
            if self._monitoredParameters.hasFromPath(key):
                return self._monitoredParameters.getFromPath(key)
            elif self._reconfigurableParameters.hasFromPath(key):
                return self._reconfigurableParameters.getFromPath(key)
            elif self._initialParameters.hasFromPath(key):
                return self._initialParameters.getFromPath(key)
            else:
                raise ParameterException("Illegal trial to get parameter (" + key + ") "
                    "which was not described in the expectedParameters section")
  
    @classmethod
    def getExpectedParameters(cls, at = AccessType(INIT|WRITE), state = ""):
        """
        Get expected parameters for given class and all its parents in inheritance hierarchy.
        The resulting schema depends on given access type and current state
        """
        cls.inheritanceChain = inheritanceChain
        schema = Schema()
        schema.initParameterDescription(cls.__name__, at, state)
        for cl in cls.inheritanceChain():
            if hasattr(cl, 'expectedParameters'):
                schema.addExternalSchema(cl.expectedParameters())
        return schema

    @classmethod
    def getFullExpectedParameters(cls):
        """Helper method to get all types of expected parameters"""
        return cls.getExpectedParameters(AccessType(INIT|READ|WRITE))
    
    @classmethod
    def getExpectedInitialParameters(cls):
        """Get only writable parameters"""
        return cls.getExpectedParameters(AccessType(INIT))
    
    @classmethod
    def getExpectedMonitorableParameters(cls):
        """Get only readable parameters"""
        return cls.getExpectedParameters(AccessType(READ))
    
    @classmethod
    def getExpectedReconfigurableParameters(cls):
        """Get only writable parameters"""
        return cls.getExpectedParameters(AccessType(WRITE))
    
    def getFullSchema(self):
        if not self._injectedExpectedParameters.empty():
            schema = self.getFullExpectedParameters()
            return schema.addExternalSchema(self._injectedExpectedParameters)
        return self._allExpectedParameters
        
    def appendSchema(self, schema):
        with self._stateChangeLock:
            self._stateDependentSchema = dict()
            self._injectedExpectedParameters.addExternalSchema(schema)
    
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
    
    @classmethod
    def convertToXsd(cls, schema):
        """Helper method to convert schema to XSD format"""
        filename = "/tmp/" + cls.__name__ + ".xsd"
        conf = Hash("TextFile.filename", filename, "TextFile.format.Xsd.indentation", -1)
        out = WriterSchema.create(conf)
        out.write(schema)
        xsd = open(filename, 'r').read()
        return xsd
    
    def getInitialParameters(self):
        return self._initialParameters.flatten()
    
    def getReconfigurableParameters(self):
        return self._reconfigurableParameters.flatten()
    
    def getMonitorableParameters(self):
        return self._monitoredParameters.flatten()
    
    def getCurrentConfiguration(self):
        config = Hash()
        config.update(self._initialParameters)
        config.update(self._reconfigurableParameters)
        config.update(self._monitoredParameters)
        ret = Hash(self._classId, config)
        return ret
    
    def reconfigure(self, instanceId, configuration):
        self._ss.call(instanceId, "slotReconfigure", configuration)
    
    def errorFoundAction(self, shortMessage, detailedMessage):
        print "*** ERROR Found ...\n\tDescription: {}\n\tDetails    : {}".format(shortMessage, detailedMessage)
    
    def updateCurrentState(self, state):
        self.set("state", state)
        self._ss.reply(state)
    
    def processEvent(self, event):
        if self.fsm is not None:
            self.updateCurrentState("Changing...")
            try:
                self.fsm.process_event(event)
            except Exception,e:
                m1 = "Exception while processing \"{}\" event".format(event.__class__.__name__)
                m2 = str(e)
                self.triggerErrorFound(m1, m2)
                KARABO_FSM_ON_ERROR.onError(m1, m2)
            self.updateCurrentState(self.fsm.get_state())
    
    def startStateMachine(self):
        """Start state machine"""
        if self.fsm is not None:
            self.fsm.start()
            self.updateCurrentState(self.fsm.get_state())
     
    def onException(self, m1, m2): print "*** WARN **** 'onException(self, m1, m2) method is not implemented in subclass"
    
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
    
    def _applyReconfiguration(self, user):
        with self._stateChangeLock:
            self._reconfigurableParameters.update(self._incomingValidatedReconfiguration)
            self._ss.emit("signalChanged", self._incomingValidatedReconfiguration, self._devInstId, self._classId)
            self._incomingValidatedReconfiguration.clear()
    
    def noStateTransition(self):
        self._ss.emit("signalNoTransition", "No transition possible", self._devInstId)
        
    @staticmethod
    def parseCommandLine(args):
        script, modname, clsname, configname = tuple(args)
        cfg = Hash("TextFile.filename", configname)
        input = ReaderHash.create(cfg)
        configuration = Hash()
        input.read(configuration)
        return (modname, clsname, configuration,)
 
    @staticmethod
    def create(modname, clsname, configuration):
        module = __import__(modname)
        userDeviceClass = getattr(module, clsname)
        schema = userDeviceClass.getExpectedParameters()
        schema.validate(configuration)
        input = configuration.get(userDeviceClass.__name__)
        device =  userDeviceClass(input)
        device.postprocessing()
        return device
   
    @abstractmethod
    def run(self): pass

    def stopEventLoop(self):
        self.running = False
    
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
    
    def slotGetSchema(self, onlyCurrentState):
        if onlyCurrentState:
            currentState = self.get("state")
            self._ss.reply(self._getStateDependentSchema(currentState))
        else:
            self._ss.reply(self._allExpectedParameters)

    def registerSlot(self, slotFunctionString):
        self._ss.registerSlot(getattr(self, slotFunctionString), self);
        
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

def launchPythonDevice():
    modname, clsname, configuration = PythonDevice.parseCommandLine(sys.argv)
    demo = PythonDevice.create(modname, clsname, configuration)
    demo.run()
    
