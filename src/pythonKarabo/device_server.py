#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
Created on Jul 26, 2012

@author: esenov
'''

import os
import sys
import socket
import platform
import re
import threading
import time
import inspect

from fsm import event_instance, ParameterException
from python_device import *
from karabo_decorators import schemamethod
from libkarabo import *

class DeviceServer(object):
    '''
    Device server
    '''

    live_threads = []
    dead_threads = []
    cLock = threading.Lock()
    instanceCountLock = threading.Lock()
    instanceCountPerDeviceServer = dict()
    
    @schemamethod
    def expectedParameters(expected):
        
        e = CHOICE_ELEMENT_BROKERCONNECTION(expected)
        e.key("connection").displayedName("Connection")
        e.description("The connection to the communication layer of the distributed system")
        e.assignmentOptional().defaultValue("Jms").advanced().init().commit()

        e = STRING_ELEMENT(expected)
        e.key("devSrvInstId").displayedName("Device-Server Instance Id")
        e.description("The device-server instance id uniquely identifies a device-server instance in the distributed system")
        e.assignmentOptional().noDefaultValue().commit()
        
        e = UINT32_ELEMENT(expected)
        e.key("nameRequestTimeout").displayedName("Name Request Timeout")
        e.description("Time to wait for name resolution (via name-server) until timeout [ms]")
        e.advanced().assignmentOptional().defaultValue(5000).commit()
        
        e = STRING_ELEMENT(expected)
        e.key("pluginsDir").displayedName("Plugins dir").description("Directory inspected for plugins")
        e.assignmentOptional().defaultValue(os.environ['PWD'] + '/plugins').reconfigurable().commit()
    
    def setupFsm(self):
        '''
        Description of state machine
        '''
        #**************************************************************
        #*                        Events                              *
        #**************************************************************
        
        EXFEL_FSM_EVENT2('ErrorFoundEvent', self.errorFound, str, str)
        EXFEL_FSM_EVENT0('EndErrorEvent', self.endError)
        EXFEL_FSM_EVENT0('NewPluginAvailableEvent', self.newPluginAvailable)
        EXFEL_FSM_EVENT0('InbuildDevicesAvailableEvent', self.inbuildDevicesAvailable)
        EXFEL_FSM_EVENT1('StartDeviceEvent', self.slotStartDevice, Hash)
        EXFEL_FSM_EVENT1('RegistrationOkEvent', self.slotRegistrationOk, str)
        EXFEL_FSM_EVENT1('RegistrationFailedEvent', self.slotRegistrationFailed, str)

        #**************************************************************
        #*                        States                              *
        #**************************************************************

        EXFEL_FSM_STATE_E('RegistrationState', self.registrationStateOnEntry)
        EXFEL_FSM_STATE('ErrorState')
        EXFEL_FSM_STATE_E('IdleState', self.idleStateOnEntry)
        EXFEL_FSM_STATE('ServingState')

        #**************************************************************
        #*                    Transition Actions                      *
        #**************************************************************

        EXFEL_FSM_ACTION2('ErrorFoundAction', self.errorFoundAction, str, str)
        EXFEL_FSM_ACTION0('EndErrorAction', self.endErrorAction)
        EXFEL_FSM_ACTION0('NotifyNewDeviceAction', self.notifyNewDeviceAction)
        EXFEL_FSM_ACTION1('StartDeviceAction', self.startDeviceAction, Hash)
        EXFEL_FSM_ACTION1('RegistrationFailedAction', self.registrationFailed, str)
        EXFEL_FSM_ACTION1('RegistrationOkAction', self.registrationOk, str)
        
        EXFEL_FSM_NO_TRANSITION_ACTION(self.noStateTransition)
            
        #**************************************************************
        #*                      AllOk Machine                         *
        #**************************************************************

        AllOkSTT = [
                    ('RegistrationState', 'RegistrationOkEvent',     'IdleState',  'RegistrationOkAction',     'none'),
                    ('RegistrationState', 'RegistrationFailedEvent', 'ErrorState', 'RegistrationFailedAction', 'none'),
                    ('IdleState',         'NewPluginAvailableEvent', 'none',       'NotifyNewDeviceAction',    'none'),
                    ('IdleState',    'InbuildDevicesAvailableEvent', 'none',       'NotifyNewDeviceAction',    'none'),
                    ('IdleState',         'StartDeviceEvent',      'ServingState', 'StartDeviceAction',        'none'),
                    ('ServingState',      'StartDeviceEvent',        'none',       'StartDeviceAction',        'none')
                   ]
        
        EXFEL_FSM_STATE_MACHINE('AllOkState', AllOkSTT, 'RegistrationState')
        
        DeviceServerMachineSTT=[
                                ('AllOkState', 'ErrorFoundEvent', 'ErrorState', 'ErrorFoundAction', 'none'),
                                ('ErrorState', 'EndErrorEvent',   'AllOkState', 'EndErrorAction',   'none')
                               ]
        
        EXFEL_FSM_STATE_MACHINE('DeviceServerMachine', DeviceServerMachineSTT, 'AllOkState')
        
        return EXFEL_FSM_CREATE_MACHINE('DeviceServerMachine')
        

    class PluginLoader(object):
        
        def __init__(self, pluginsDirectory):
            self.plugins = pluginsDirectory
            sys.path.append(self.plugins)
            self.pattern = re.compile(r'.py')
        
        def getPluginsDirectory(self):
            return self.plugins
        
        def update(self):
            matches = filter(lambda m: not m is None, [self.pattern.search(x) for x in os.listdir(self.plugins)])
            modules = list()
            for m in matches:
                name = m.string[0:m.start()]              # module name
                path = self.plugins + "/" + m.string      # path to module source
                if path.endswith(".pyc"):                 # skip compiled version
                    continue
                modules.append((name, path,))
            return modules
            
    
    class Launcher(threading.Thread):
    
        def __init__(self, pluginsdir, modname, clsname, conf):
            threading.Thread.__init__(self)
            filename = "/tmp/" + modname + "." + clsname + ".configuration.xml"
            cfg = Hash("TextFile.filename", filename, "TextFile.format.Xml.indentation", 2)
            out = WriterHash.create(cfg)
            out.write(conf)
            try:
                self.device = conf.getFromPath(clsname + ".devInstId")
            except RuntimeError, e:
                print e
            self.script = os.path.realpath(pluginsdir + "/" + modname + ".py")
            self.args = [self.script, modname, clsname, filename]
        
        def run(self):
            with DeviceServer.cLock:
                DeviceServer.live_threads.append(self)

            self.pid = os.fork()
            if self.pid == 0:
                os.chmod(self.script, 0755)
                os.execvpe(self.script, self.args, os.environ)
            else:
                print "\nLaunch device", self.device
                id, status = os.waitpid(self.pid, 0)
                print "Device %r finally died" % (self.device)
                
            with DeviceServer.cLock:
                DeviceServer.dead_threads.append(self)
                DeviceServer.live_threads.remove(self)
        
    
    def __init__(self, input):
        '''Constructor'''
        super(DeviceServer, self).__init__()
        # describe FSM
        self.fsm = self.setupFsm()
        cls = self.__class__
        self._classId = cls.__name__
        self.ss = None
        self.availableModules = dict()
        self.availableDevices = dict()
        self.deviceInstanceMap = dict()
        self.selfDestroyFlag = False
        self._devSrvInstId = None
        self.nameRequestTimeout = 10
        if input.has('nameRequestTimeout'):
            self.nameRequestTimeout = input.get('nameRequestTimeout')
        if input.has('devSrvInstId'):
            self._devSrvInstId = input.get('devSrvInstId')
        self.running = True
        self.pluginsDir = os.environ['PWD'] + '/plugins'
        if input.has('pluginsDir'):
            self.pluginsDir = input.get("pluginsDir")
        self.pluginLoader = DeviceServer.PluginLoader(self.pluginsDir)
        self.isRegistered = False
        
        print "Initialize SignalSlotable object...\n"
        if self._devSrvInstId is None:
            s = SignalSlotable()
            (self._devSrvInstId, ) = s.request("*", "slotDeviceServerProvideName").waitForReply(self.nameRequestTimeout)
            del s
        self.ss = SignalSlotable(self._devSrvInstId)
        
        print "\nInitialize Logging...\n"
        connectionConfig = input.get("connection")

        logConfig = Hash()
        logConfig.setFromPath("Logger.categories[0].Category.name", self._devSrvInstId)
        logConfig.setFromPath("Logger.categories[0].Category.priority", "INFO")
        logConfig.setFromPath("Logger.categories[0].Category.appenders[0].Network.layout.Pattern.pattern", "%d{%F %H:%M:%S} | %p | %c | %m")
        logConfig.setFromPath("Logger.categories[0].Category.appenders[0].Network.connection", connectionConfig)
        Logger.create(logConfig).initialize()
        self.log = Logger.logger(self._devSrvInstId)

        self._registerAndConnectSignalsAndSlots()
    
    def _registerAndConnectSignalsAndSlots(self):
        cls = self.__class__
        self.ss.registerSignal("signalNewDeviceClassAvailable", str, str, str) # DeviceServerInstanceId, classId, xsd
        self.ss.registerSignal("signalNewDeviceInstanceAvailable", str, Hash)  # DeviceServerInstanceId, currentConfig
        self.ss.registerSignal("signalDeviceServerInstanceGone", str)          # DeviceServerInstanceId
        self.ss.registerSlot(self.slotStartDevice, self)
        self.ss.registerSlot(self.slotRegistrationOk, self)
        self.ss.registerSlot(self.slotRegistrationFailed, self)
        self.ss.registerSlot(self.slotKillDeviceServerInstance, self)
        self.ss.registerSlot(self.slotKillDeviceInstance, self)
        self.ss.connect("", "signalNewDeviceClassAvailable", "*", "slotNewDeviceClassAvailable", ConnectionType.NO_TRACK, False)
        self.ss.connect("", "signalNewDeviceInstanceAvailable", "*", "slotNewDeviceInstanceAvailable", ConnectionType.NO_TRACK, False)

    def run(self):
        self.fsm.start()
        while self.running:
            with DeviceServer.cLock:
                # critical section
                for c in DeviceServer.dead_threads:
                    self.log.INFO("Join dead thread {}".format(c.getName()))
                    c.join()
                DeviceServer.dead_threads = []
                # end of critical section
            if self.isRegistered:
                self.scanPlugins()
            if self.selfDestroyFlag:
                with DeviceServer.cLock:
                    if len(DeviceServer.live_threads) == 0 and len(DeviceServer.dead_threads) == 0:
                        self.ss.call("*", "slotDeviceServerInstanceGone", self._devSrvInstId)
                        self.stopDeviceServer()
            time.sleep(3)
    
    def stopDeviceServer(self):
        self.running = False
    
    def scanPlugins(self):
        modules = self.pluginLoader.update()   # just list of modules in plugins dir
        for name, path in modules:
            if name in self.availableModules:
                continue
            try:
                dname = os.path.dirname( os.path.realpath(path) )
                if dname not in sys.path:
                    sys.path.append(dname)
                module = __import__(name)
            except ImportError,e:
                self.log.WARN("scanPlugins: Cannot import module {} -- {}".format(name,e))
                continue
            if "PythonDevice" not in dir(module):
                raise ParameterException("Module '" + name + "' has no use of PythonDevice class")
                continue
            candidates = [module.PythonDevice]
            for item in dir(module):
                obj = getattr(module, item)
                # take class candidate that originated from this module and
                # has 'expectedParameters' attribute
                if inspect.isclass(obj) and issubclass(obj, module.PythonDevice):
                    candidates.append(obj)
            tree = inspect.getclasstree(candidates)  # build inheritance tree
            # convert tree to the list
            classList = []
            self._getListOfClasses(tree, classList)
            deviceClass = classList.pop()  # most derived class in hierarchy
            deviceClass.inheritanceChain = inheritanceChain
            classId = deviceClass.__name__
            try:
                schema = deviceClass.getExpectedParameters(AccessType(READ|INIT|WRITE))
                xsd = deviceClass.convertToXsd(schema)
                self.availableModules[name] = classId
                self.availableDevices[classId] = {"mustNotify": True, "module": name, "xsd": xsd}
                self.newPluginAvailable()
            except RuntimeError, e:
                self.log.ERROR("Failure while building schema for class {}: {}".format(classId, e.message))
    
    def _getListOfClasses(self, tree, outList):
        (cl, bases) = tree[0]
        if hasattr(cl, 'expectedParameters'):
            outList.append(cl)
        if len(tree) == 1:
            return
        self._getListOfClasses(tree[1], outList)
    
    def errorFound(self, m1, m2):
        self.fsm.process_event(event_instance('ErrorFoundEvent', (m1, m2,)))
    
    def endError(self):
        self.fsm.process_event(event_instance('EndErrorEvent', ()))
    
    def newPluginAvailable(self):
        self.fsm.process_event(event_instance('NewPluginAvailableEvent', ()))
    
    def inbuildDevicesAvailable(self):
        self.fsm.process_event(event_instance('InbuildDevicesAvailableEvent', ()))
    
    def slotStartDevice(self, h):
        self.fsm.process_event(event_instance('StartDeviceEvent', (h,)))
    
    def slotRegistrationOk(self, a1):
        self.fsm.process_event(event_instance('RegistrationOkEvent', (a1,)))
    
    def slotRegistrationFailed(self, a1):
        self.fsm.process_event(event_instance('RegistrationFailedEvent', (a1,)))
    
    def updateCurrentState(self, currentState):
        self.ss.reply(currentState)
    
    def registrationStateOnEntry(self):
        self.ss.call("*", "slotNewDeviceServerAvailable", platform.node(), self._devSrvInstId)
    
    def idleStateOnEntry(self):
        id = self._devSrvInstId
        WriterHash.create("TextFile", Hash("filename", "autoload.xml")).write(Hash("DeviceServer.devSrvInstId", id))
        #TODO: check what we have to implement here
        self.isRegistered = True   # this is the last statement!
    
    def errorFoundAction(self, m1, m2):
        self.log.ERROR("{} -- {}".format(m1,m2))
    
    def endErrorAction(self):
        pass

    def notifyNewDeviceAction(self):
        for (classId, d) in self.availableDevices.items():
            if d['mustNotify']:
                d['mustNotify'] = False
                self.log.DEBUG("Notifying about {}".format(d['module']))
                self.ss.emit("signalNewDeviceClassAvailable", self.ss.getInstanceId(), classId, d["xsd"])

    def startDeviceAction(self, conf):
        self.log.DEBUG("Trying to start device with the following configuration:\n{}".format(conf))
        modified = Hash(conf)
        classId = iter(modified).next().key
        modified.setFromPath(classId + ".devSrvInstId", self._devSrvInstId)
        if modified.hasFromPath(classId + ".devInstId"):
            devInstId = modified.getFromPath(classId + ".devInstId")
        else:
            devInstId = self._generateDefaultDeviceInstanceId(classId)
            modified.setFromPath(classId + ".devInstId", devInstId)
        d = self.availableDevices[classId]
        module_name = d["module"]
        # check that configuration parameters are valid
        try:
            module = __import__(module_name)
            userDeviceClass = getattr(module, classId)
            schema = userDeviceClass.getExpectedParameters()
            schema.validate(modified)
        except RuntimeError, e:
            self.log.WARN("Wrong input configuration for class '{}': {}".format(classId, e.message))
            return
        launcher = DeviceServer.Launcher(self.pluginsDir, module_name, classId, modified).start()
        self.deviceInstanceMap[devInstId] = launcher

    def _generateDefaultDeviceInstanceId(self, devClassId):
        cls = self.__class__
        with cls.instanceCountLock:
            if self._devSrvInstId not in cls.instanceCountPerDeviceServer:
                cls.instanceCountPerDeviceServer[self._devSrvInstId] = 0
            cls.instanceCountPerDeviceServer[self._devSrvInstId] += 1
            _index = cls.instanceCountPerDeviceServer[self._devSrvInstId]
            if self._devSrvInstId == "":
                #myHostName, someList, myHostAddrList = socket.gethostbyaddr(socket.gethostname())
                myHostName = socket.gethostname()
                return myHostName + "/" + devClassId + "/" + str(_index)
            tokens = self._devSrvInstId.split("/")
            _domain = tokens.pop(0) + "-" + tokens.pop()
            _id = _domain + "/" + devClassId + "/" + str(_index)
            return _id
     
    def slotKillDeviceServerInstance(self):
        self.log.INFO("Received kill signal")
        for devInstId in self.deviceInstanceMap.keys():
            self.ss.call(devInstId, "slotKillDeviceInstance")
        self.selfDestroyFlag = True
        
    def slotKillDeviceInstance(self, id):
        self.log.INFO("Received kill signal for device {}".format(id))
        if id in self.deviceInstanceMap:
            self.ss.call(id, "slotKillDeviceInstance")
            del self.deviceInstanceMap[id]
            self.log.DEBUG("Device {} was instructed to die".format(id))

    def registrationFailed(self, a1):
        self.log.WARN("{}".format(a1))
    
    def registrationOk(self, a1):
        self.log.INFO("Master says: {}".format(a1))
    
    def noStateTransition(self):
        self.log.DEBUG("No transition")
        

if __name__ == '__main__':
    myHost = socket.gethostname()
    devSrvId = myHost + "/" + "DeviceServer" + "/0" 
    conf = Hash('DeviceServer.devSrvInstId', devSrvId)
    schema = DeviceServer.expectedParameters()
    #print schema
    try:
        conf = schema.validate(conf)
    except RuntimeError, e:
        print e
        sys.exit(1)
    server = DeviceServer(conf.get("DeviceServer"))
    server.run()
