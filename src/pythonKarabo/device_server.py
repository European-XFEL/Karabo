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

from fsm import event_instance
from python_device import *
from karabo_decorators import *
from libkarathon import *
from plugin_loader import PluginLoader


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
        
        e = CHOICE_ELEMENT(expected).key("connection").displayedName("Connection")
        e.description("The connection to the communication layer of the distributed system")
        e.appendNodesOfConfigurationBase(BrokerConnection)
        e.assignmentOptional().defaultValue("Jms").advanced().init().commit()

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
        self.nameRequestTimeout = 10
        if 'nameRequestTimeout' in input:
            self.nameRequestTimeout = input['nameRequestTimeout']
        if 'serverId' in input:
            self.serverid = input['serverId']
        self.running = True
        self.isRegistered = False
        
        print "Initialize SignalSlotable object...\n"
        if self.serverid is None:
            s = SignalSlotable()
            (self.serverid, ) = s.request("*", "slotDeviceServerProvideName").waitForReply(self.nameRequestTimeout)
            del s
        self.ss = SignalSlotable(self.serverid)
        
        print "\nInitialize Logging...\n"
        self.loadLogger(input)
        self.loadPluginLoader(input)
        if "autoStart" in input:
            self.autoStart = input["autoStart"]
    
    def _registerAndConnectSignalsAndSlots(self):
        cls = self.__class__
        self.ss.registerSignal("signalNewDeviceClassAvailable", str, str, str) # DeviceServerInstanceId, classid, xsd
        self.ss.registerSignal("signalNewDeviceInstanceAvailable", str, Hash)  # DeviceServerInstanceId, currentConfig
        self.ss.registerSignal("signalDeviceServerInstanceGone", str)          # DeviceServerInstanceId
        self.ss.registerSlot(self.slotStartDevice, self)
        self.ss.registerSlot(self.slotRegistrationOk, self)
        self.ss.registerSlot(self.slotRegistrationFailed, self)
        self.ss.registerSlot(self.slotKillDeviceServerInstance, self)
        self.ss.registerSlot(self.slotKillDeviceInstance, self)
        self.ss.connect("", "signalNewDeviceClassAvailable", "*", "slotNewDeviceClassAvailable", ConnectionType.NO_TRACK, False)
        self.ss.connect("", "signalNewDeviceInstanceAvailable", "*", "slotNewDeviceInstanceAvailable", ConnectionType.NO_TRACK, False)

    def logLogger(self, input):
        config = input["Logger"]
        appenders = config["appenders"]
        appenderConfig = Hash()
        appenderConfig["Network.layout.Pattern.format"] = "%d{%F %H:%M:%S} | %p | %c | %m"
        appenderConfig["Network.connection"] = input["connection"]
        appenders.append(appenderConfig)
        Logger.configure(config)
    
    def loadPluginLoader(self, input):
        self.pluginLoader = PluginLoader.createNode("PythonPluginLoader", "PythonPluginLoader", input)
        
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
        WriterHash.create("TextFile", Hash("filename", "autoload.xml")).write(Hash("DeviceServer.serverId", id))
        #TODO: check what we have to implement here
        self.isRegistered = True   # this is the last statement!
    
    def errorFoundAction(self, m1, m2):
        self.log.ERROR("{} -- {}".format(m1,m2))
    
    def endErrorAction(self):
        pass

    def notifyNewDeviceAction(self):
        for (classid, d) in self.availableDevices.items():
            if d['mustNotify']:
                d['mustNotify'] = False
                self.log.DEBUG("Notifying about {}".format(d['module']))
                self.ss.emit("signalNewDeviceClassAvailable", self.ss.getInstanceId(), classid, d["xsd"])

    def startDeviceAction(self, conf):
        self.log.DEBUG("Trying to start device with the following configuration:\n{}".format(conf))
        modified = Hash(conf)
        classid = iter(modified).next().getKey()
        modified[classid + ".serverId"] = self.serverid
        if classid + ".deviceId" in modified:
            deviceid = modified[classid + ".deviceId"]
        else:
            deviceid = self._generateDefaultDeviceInstanceId(classid)
            modified[classid + ".deviceId"] = deviceid
        # create temporary instance to check the configuration parameters are valid
        try:
            instance = PythonDevice.create(modified) # create instance in this interpreter
        except RuntimeError, e:
            self.log.WARN("Wrong input configuration for class '{}': {}".format(classid, e.message))
            return
        module_name = self.availableDevices[classid]["module"]
        input = Hash()
        input["classId"] = classid
        input["module"] = self.availableDevices[classid]["module"]
        input["plugins"] = self.pluginLoader.getPluginDirectory()
        input["configuration"] = modified
        launcher = Launcher.create("PythonLauncher", input).start()
        #launcher = DeviceServer.Launcher(self.pluginLoader.getPluginDirectory(), module_name, classid, modified).start()
        self.deviceInstanceMap[deviceid] = launcher

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
                return myHostName + "/" + devClassId + "/" + str(_index)
            tokens = self.serverid.split("/")
            _domain = tokens.pop(0) + "-" + tokens.pop()
            _id = _domain + "/" + devClassId + "/" + str(_index)
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
   

        
@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("PythonLauncher", "1.0")
class Launcher(threading.Thread):

    def __init__(self, input):       #pluginsdir, modname, clsname, conf):
        threading.Thread.__init__(self)
        modname = input["module"]
        clsname = input["classId"]
        pluginsdir = input["plugins"]
        configuration = input["configuration"]
        try:
            self.device = configuration[clsname + ".deviceId"]
        except RuntimeError, e:
            print e
        self.script = os.path.realpath(pluginsdir + "/" + modname + ".py")
        input["script"] = self.script
        
        filename = "/tmp/" + modname + "." + clsname + ".configuration.xml"
        config = Hash("filename", filename, "format.Xml.indentation", 2)
        cfg = Hash("TextFile.filename", filename, "TextFile.format.Xml.indentation", 2)
        out = OutputHash.create("TextFile", config)
        out.write(input)
        
        self.args = [self.script, filename]

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


def getConfigurationFromCommandLine(argv):
    if argv is None:
        hostname = socket.gethostname()
        host, dot, domain = hostname.partition('.')
        serverid = host + "/" + DeviceServer.__classid__ + "/0"
        return Hash("serverId", serverid)
    
def main(argv):
    try:
        server = DeviceServer.create("DeviceServer", getConfigurationFromCommandLine(argv))
        server.run()
    except Exception,e:
        print "Exception caught: " + str(e)
    
    
if __name__ == '__main__':
    main(sys.argv[1:])
