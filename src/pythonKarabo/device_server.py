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
import threading
import time
import inspect
from libkarathon import *
from fsm import *
from karabo_decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from plugin_loader import PluginLoader
from runner import Runner


# Forward declaration of python class
class Launcher(threading.Thread):
    def __init__(self, input):
        pass
    def run(self):
        pass


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("DeviceServer", "1.0")
class DeviceServer(object):
    '''
    Device server
    '''

    live_threads = []
    dead_threads = []
    cLock = threading.Lock()
    instanceCountLock = threading.Lock()
    instanceCountPerDeviceServer = dict()
    
    @staticmethod
    def expectedParameters(expected):
        
        e = STRING_ELEMENT(expected).key("serverId").displayedName("Server ID")
        e.description("The device-server instance id uniquely identifies a device-server instance in the distributed system")
        e.assignmentOptional().noDefaultValue().commit()
        
        e = UINT32_ELEMENT(expected).key("nameRequestTimeout").displayedName("Name Request Timeout")
        e.description("Time to wait for name resolution (via name-server) until timeout [ms]")
        e.advanced().assignmentOptional().defaultValue(5000).commit()
        
        e = NODE_ELEMENT(expected).key("Logger").displayedName("Logger")
        e.description("Logging settings")
        e.appendParametersOfConfigurableClass(Logger, "Logger").commit()
        
        e = NODE_ELEMENT(expected).key("PluginLoader")
        e.displayedName("Plugin Loader").description("Plugin Loader sub-configuration")
        e.appendParametersOfConfigurableClass(PluginLoader, "PythonPluginLoader")
        e.commit()
        
    def setupFsm(self):
        '''
        Description of state machine
        '''
        #**************************************************************
        #*                        Events                              *
        #**************************************************************
        
        KARABO_FSM_EVENT2(self, 'ErrorFoundEvent', 'errorFound')
        KARABO_FSM_EVENT0(self, 'EndErrorEvent', 'endError')
        KARABO_FSM_EVENT0(self, 'NewPluginAvailableEvent', 'newPluginAvailable')
        KARABO_FSM_EVENT0(self, 'InbuildDevicesAvailableEvent', 'inbuildDevicesAvailable')
        KARABO_FSM_EVENT1(self, 'StartDeviceEvent', 'slotStartDevice')
        KARABO_FSM_EVENT1(self, 'RegistrationOkEvent', 'slotRegistrationOk')
        KARABO_FSM_EVENT1(self, 'RegistrationFailedEvent', 'slotRegistrationFailed')

        #**************************************************************
        #*                        States                              *
        #**************************************************************

        KARABO_FSM_STATE_E('RegistrationState', self.registrationStateOnEntry)
        KARABO_FSM_STATE('ErrorState')
        KARABO_FSM_STATE_E('IdleState', self.idleStateOnEntry)
        KARABO_FSM_STATE('ServingState')

        #**************************************************************
        #*                    Transition Actions                      *
        #**************************************************************

        KARABO_FSM_ACTION2('ErrorFoundAction', self.errorFoundAction, str, str)
        KARABO_FSM_ACTION0('EndErrorAction', self.endErrorAction)
        KARABO_FSM_ACTION0('NotifyNewDeviceAction', self.notifyNewDeviceAction)
        KARABO_FSM_ACTION1('StartDeviceAction', self.startDeviceAction, Hash)
        KARABO_FSM_ACTION1('RegistrationFailedAction', self.registrationFailed, str)
        KARABO_FSM_ACTION1('RegistrationOkAction', self.registrationOk, str)
        
        KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)
            
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
        
        KARABO_FSM_STATE_MACHINE('AllOkState', AllOkSTT, 'RegistrationState')
        
        DeviceServerMachineSTT=[
                                ('AllOkState', 'ErrorFoundEvent', 'ErrorState', 'ErrorFoundAction', 'none'),
                                ('ErrorState', 'EndErrorEvent',   'AllOkState', 'EndErrorAction',   'none')
                               ]
        
        KARABO_FSM_STATE_MACHINE('DeviceServerMachine', DeviceServerMachineSTT, 'AllOkState')
        
        return KARABO_FSM_CREATE_MACHINE('DeviceServerMachine')
        
    
    
    def __init__(self, input):
        '''Constructor'''
        if input is None:
            raise ValueError,"Input configuration for constructor should be Hash, not None"
        super(DeviceServer, self).__init__()
        # describe FSM
        self.processEventLock = threading.RLock()
        self.fsm = self.setupFsm()
        self.ss = None
        self.availableModules = dict()
        self.availableDevices = dict()
        self.deviceInstanceMap = dict()
        self.selfDestroyFlag = False
        self.serverid = None
        self.nameRequestTimeout = input['nameRequestTimeout']
        if 'serverId' in input:
            self.serverid = input['serverId']
        self.running = True
        self.isRegistered = False
        
        print "Initialize SignalSlotable object...\n"
        if self.serverid is None:
            possiblyFullHostName = socket.gethostname()
            myHostName, dotsep, domainName = possiblyFullHostName.partition('.')
            self.s = SignalSlotable()
            (self.serverid,) = self.s.request("*", "slotDeviceServerProvideName", myHostName).waitForReply(self.nameRequestTimeout)
            print "Request for serverId returns: %r" % self.serverid
        self.ss = SignalSlotable(self.serverid)
        
        self.loadLogger(input)
        self.loadPluginLoader(input)
        if "autoStart" in input:
            self.autoStart = input["autoStart"]
        print "\n... DeviceServer object constructed.\n"
    
    def _registerAndConnectSignalsAndSlots(self):
        cls = self.__class__
        self.ss.registerSignal("signalNewDeviceClassAvailable", str, str, str) # DeviceServerInstanceId, classid, xsd
        self.ss.registerSignal("signalNewDeviceInstanceAvailable", str, Hash)  # DeviceServerInstanceId, currentConfig
        self.ss.registerSignal("signalDeviceServerInstanceGone", str)          # DeviceServerInstanceId
        self.ss.registerSlot(self.slotStartDevice)
        self.ss.registerSlot(self.slotRegistrationOk)
        self.ss.registerSlot(self.slotRegistrationFailed)
        self.ss.registerSlot(self.slotKillDeviceServerInstance)
        self.ss.registerSlot(self.slotKillDeviceInstance)
        self.ss.connect("", "signalNewDeviceClassAvailable", "*", "slotNewDeviceClassAvailable", ConnectionType.NO_TRACK, False)
        self.ss.connect("", "signalNewDeviceInstanceAvailable", "*", "slotNewDeviceInstanceAvailable", ConnectionType.NO_TRACK, False)

    def loadLogger(self, input):
        if "Logger" in input:
            config = input["Logger"]
        appenders = config["appenders"]
        appenderConfig = Hash()
        appenderConfig["Network.layout.Pattern.format"] = "%d{%F %H:%M:%S} | %p | %c | %m"
        if "connection" in input:
            appenderConfig["Network.connection"] = input["connection"]
        appenders.append(appenderConfig)
        Logger.configure(config)
    
    def loadPluginLoader(self, input):
        self.pluginLoader = PluginLoader.createNode("PluginLoader", "PythonPluginLoader", input)
        
    def run(self):
        self.log = Logger.getLogger(self.serverid)
        self._registerAndConnectSignalsAndSlots()
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
                        self.ss.call("*", "slotDeviceServerInstanceGone", self.serverid)
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
                raise IndexError,"Module '" + name + "' has no use of PythonDevice class"
            candidates = [module.PythonDevice]
            #
            # IMPORTANT!
            # We do an assumption that module contains only one user device
            #
            for item in dir(module):
                obj = getattr(module, item)
                if inspect.isclass(obj) and issubclass(obj, module.PythonDevice):
                    candidates.append(obj)
                    
            def mostDerived(candidates):
                tree = inspect.getclasstree(candidates)  # build inheritance tree
                try:
                    while True:
                        c,b = tree[0]
                        tree = tree[1]
                except IndexError,e:
                    pass
                return c
            
            # get mostDerived from tree
            deviceClass = mostDerived(candidates)  # most derived class in hierarchy
            try:
                schema = deviceClass.getSchema(deviceClass.__classid__)
                config = Hash("indentation", -1)
                xsd = TextSerializerSchema.create('Xsd', config).save(schema)
                self.availableModules[name] = deviceClass.__classid__
                self.availableDevices[deviceClass.__classid__] = {"mustNotify": True, "module": name, "xsd": xsd}
                self.newPluginAvailable()
            except RuntimeError, e:
                self.log.ERROR("Failure while building schema for class {}, base class {} and bases {} : {}".format(
                    deviceClass.__classid__, deviceClass.__base_classid__, deviceClass.__bases_classid__, e.message))
    
    def processEvent(self, event):
        with self.processEventLock:
            self.fsm.process_event(event)
        
    def updateCurrentState(self, currentState):
        self.ss.reply(currentState)
    
    def registrationStateOnEntry(self):
        self.ss.call("*", "slotNewDeviceServerAvailable", platform.node(), self.serverid)
    
    def idleStateOnEntry(self):
        id = self.serverid
        OutputHash.create("TextFile", Hash("filename", "autoload.xml")).write(Hash("DeviceServer.serverId", id))
        #TODO: check what we have to implement here
        self.isRegistered = True   # this is the last statement!
    
    def errorFoundAction(self, m1, m2):
        self.log.ERROR("{} -- {}".format(m1,m2))
    
    def endErrorAction(self):
        pass

    def notifyNewDeviceAction(self):
        deviceClasses = []
        for (classid, d) in self.availableDevices.items():
            deviceClasses.append(classid)
            if d['mustNotify']:
                d['mustNotify'] = False
                self.log.DEBUG("Notifying about {}".format(d['module']))
                self.ss.emit("signalNewDeviceClassAvailable", self.ss.getInstanceId(), classid, d["xsd"])
        self.ss.updateInstanceInfo(Hash("deviceClasses", deviceClasses))

    def startDeviceAction(self, conf):
        modified = Hash(conf)
        classid = iter(modified).next().getKey()
        self.log.INFO("Trying to start {}...".format(classid))
        self.log.DEBUG("with the following configuration:\n".format(conf))
        modified[classid]["serverId"] = self.serverid
        if classid + ".deviceId" in modified:
            deviceid = modified[classid]["deviceId"]
        else:
            deviceid = self._generateDefaultDeviceInstanceId(classid)
            modified[classid]["deviceId"] = deviceid
        # create temporary instance to check the configuration parameters are valid
        try:
            configuration = modified[classid]
            pluginDir = self.pluginLoader.getPluginDirectory()
            modname = self.availableDevices[classid]["module"]
            module = __import__(modname)
            UserDevice = getattr(module, classid)
            schema = UserDevice.getSchema(classid)
            validator = Validator()
            validated = validator.validate(schema, configuration)
            #print "classId = {}, modname = {}, pluginDir = {}".format(classid, modname, pluginDir)
            #print "Validated configuration...\n", validated
            launcher = Launcher(pluginDir, modname, classid, modified).start()
            self.deviceInstanceMap[deviceid] = launcher
            #self.ss.emit("signalNewDeviceInstanceAvailable", self.getInstanceId(), Hash(classid, validated))
            del validated
        except Exception, e:
            self.log.WARN("Wrong input configuration for class '{}': {}".format(classid, e.message))
            return
        

    def _generateDefaultDeviceInstanceId(self, devClassId):
        cls = self.__class__
        with cls.instanceCountLock:
            if self.serverid not in cls.instanceCountPerDeviceServer:
                cls.instanceCountPerDeviceServer[self.serverid] = 0
            cls.instanceCountPerDeviceServer[self.serverid] += 1
            _index = cls.instanceCountPerDeviceServer[self.serverid]
            if self.serverid == "":
                #myHostName, someList, myHostAddrList = socket.gethostbyaddr(socket.gethostname())
                possiblyFullHostName = socket.gethostname()
                myHostName, dotsep, domainName = possiblyFullHostName.partition('.')
                return myHostName + "_" + devClassId + "_" + str(_index)
            tokens = self.serverid.split("_")
            _domain = tokens.pop(0) + "-" + tokens.pop()
            _id = _domain + "_" + devClassId + "_" + str(_index)
            return _id
     
    def slotKillDeviceServerInstance(self):
        self.log.INFO("Received kill signal")
        for deviceid in self.deviceInstanceMap.keys():
            self.ss.call(deviceid, "slotKillDeviceInstance")
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
   

        
class Launcher(threading.Thread):

    def __init__(self, pluginDir, modname, classid, config):
        threading.Thread.__init__(self)
        if "deviceId" in config[classid]:
            self.device = config[classid]["deviceId"]
        else:
            raise RuntimeError, "Access to " + classid + ".deviceId failed"
        
        try:
            self.script = os.path.realpath(pluginDir + "/" + modname + ".py")
            filename = "/tmp/" + modname + "." + classid + ".configuration.xml"
            cfg = Hash("filename", filename, "format.Xml.indentation", 2) 
            out = OutputHash.create("TextFile", cfg)
            out.write(config)
            self.args = [self.script, modname, classid, filename]
        except Exception,e:
            raise RuntimeError,"Exception happened while writing 'config' object: " + str(e) 

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

    
def main(args):
    try:
        server = Runner(DeviceServer).instantiate(args)
        if server:
            server.run()
    except Exception,e:
        print "Exception caught: " + str(e)
    
    
if __name__ == '__main__':
    main(sys.argv)
