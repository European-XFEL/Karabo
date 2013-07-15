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
from karabo.karathon import *
from karabo.fsm import *
from karabo.decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from karabo.plugin_loader import PluginLoader
from karabo.configurator import Configurator
from karabo.device import PythonDevice
from karabo.runner import Runner


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
    Device server serves as a launcher of python devices.  It scans 'plugins' directory
    for new plugins (python scripts) available and communicates its findings to master device
    server.  It communicates XSD form of schema of user devices and starts such devices as
    separate process if user push "Initiate" button in GUI
    '''

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
        
        e = NODE_ELEMENT(expected).key("PluginLoader")
        e.displayedName("Plugin Loader").description("Plugin Loader sub-configuration")
        e.appendParametersOfConfigurableClass(PluginLoader, "PythonPluginLoader")
        e.commit()
        
        e = NODE_ELEMENT(expected).key("Logger").displayedName("Logger")
        e.description("Logging settings")
        e.appendParametersOfConfigurableClass(Logger, "Logger").commit()
    
        e = OVERWRITE_ELEMENT(expected).key("Logger.appenders")
        e.setNewDefaultValue("Ostream").commit()
        
        e = OVERWRITE_ELEMENT(expected).key("Logger.appenders.Ostream.layout")
        e.setNewDefaultValue("Pattern").commit()
        
        e = OVERWRITE_ELEMENT(expected).key("Logger.appenders.Ostream.layout.Pattern.format")
        e.setNewDefaultValue("%p  %c  : %m%n").commit()

    def setupFsm(self):
        '''
        Description of DeviceServer state machine
        '''
        #**************************************************************
        #*                        Events                              *
        #**************************************************************
        
        KARABO_FSM_EVENT2(self, 'ErrorFoundEvent', 'errorFound')
        KARABO_FSM_EVENT0(self, 'EndErrorEvent', 'endError')
        KARABO_FSM_EVENT0(self, 'NewPluginAvailableEvent', 'newPluginAvailable')
        KARABO_FSM_EVENT0(self, 'InbuildDevicesAvailableEvent', 'inbuildDevicesAvailable')
        KARABO_FSM_EVENT1(self, 'StartDeviceEvent', 'slotStartDevice')

        #**************************************************************
        #*                        States                              *
        #**************************************************************

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
        
        KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)
            
        #**************************************************************
        #*                      AllOk Machine                         *
        #**************************************************************

        AllOkSTT = [
                    ('IdleState',         'NewPluginAvailableEvent', 'none',       'NotifyNewDeviceAction',    'none'),
                    ('IdleState',    'InbuildDevicesAvailableEvent', 'none',       'NotifyNewDeviceAction',    'none'),
                    ('IdleState',         'StartDeviceEvent',      'ServingState', 'StartDeviceAction',        'none'),
                    ('ServingState',      'StartDeviceEvent',        'none',       'StartDeviceAction',        'none')
                   ]
        
        KARABO_FSM_STATE_MACHINE('AllOkState', AllOkSTT, 'IdleState')
        
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
        self.availableDevices = dict()
        self.deviceInstanceMap = dict()
        self.hostname, dotsep, self.domainname = socket.gethostname().partition('.')
        
        if 'serverId' in input:
            self.serverid = input['serverId']
            saveToFile(Hash("DeviceServer.serverId", self.serverid), "autoload.xml")
        else:
            self.serverid = self._generateDefaultServerId()
        
        self.loadLogger(input)
        self.loadPluginLoader(input)
        self.nameRequestTimeout = input['nameRequestTimeout']
    
    def _generateDefaultServerId(self):
        return self.hostname + "_Server_" + str(os.getpid())
    
    def loadLogger(self, input):
        config = input["Logger"]
        config["categories[0].Category.name"] = "karabo"
        config["categories[0].Category.appenders[0].Ostream.layout.Pattern.format"] = "%p  %c  : %m%n"
        config["categories[0].Category.additivity"] = False
        config["appenders[1].Network.layout.Pattern.format"] = "%d{%F %H:%M:%S} | %p | %c | %m"
        if "connection" in input:
            config["appenders[1].Network.connection"] = input["connection"]
        Logger.configure(config)
    
    def loadPluginLoader(self, input):
        self.pluginLoader = PluginLoader.createNode("PluginLoader", "PythonPluginLoader", input)
        
    def run(self):
        self.log = Logger.getLogger(self.serverid)
        self.log.INFO("Starting Karabo DeviceServer on host: {}".format(self.hostname))
        self.ss = SignalSlotable.create(self.serverid)
        self._registerAndConnectSignalsAndSlots()
        self.fsm.start()
        info = Hash("type", "server")
        info["serverId"] = self.serverid
        info["version"] = self.__class__.__version__
        info["host"] = self.hostname
        # TODO
        self.ss.runEventLoop(10, info)  # block
    
    def _registerAndConnectSignalsAndSlots(self):
        self.ss.registerSignal("signalNewDeviceClassAvailable", str, str, Schema) # serverid, classid, Schema
        self.ss.registerSlot(self.slotStartDevice)
        self.ss.registerSlot(self.slotKillServer)
        self.ss.registerSlot(self.slotDeviceGone)
        self.ss.registerSlot(self.slotGetClassSchema)
        self.ss.connect("", "signalNewDeviceClassAvailable", "*", "slotNewDeviceClassAvailable", ConnectionType.NO_TRACK, False)

    def onStateUpdate(self, currentState):
        self.ss.reply(currentState)
        
    def idleStateOnEntry(self):
        self.log.INFO("DeviceServer starts up with id: {}".format(self.serverid))
        self.log.INFO("Keep watching directory: \"{}\" for Device plugins".format(self.pluginLoader.getPluginDirectory()))
        self.pluginThread = threading.Thread(target = self.scanPlugins)
        self.scanning = True
        self.pluginThread.start()
    
    def scanPlugins(self):
        self.availableModules = dict()
        while self.scanning:
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
                    schema = Configurator(PythonDevice).getSchema(deviceClass.__classid__)
                    self.availableModules[name] = deviceClass.__classid__
                    self.availableDevices[deviceClass.__classid__] = {"mustNotify": True, "module": name, "xsd": schema}
                    self.newPluginAvailable()
                    print "Successfully loaded plugin: \"{}.py\"".format(name)
                except (RuntimeError, AttributeError), e:
                    self.log.ERROR("Failure while building schema for class {}, base class {} and bases {} : {}".format(
                        deviceClass.__classid__, deviceClass.__base_classid__, deviceClass.__bases_classid__, e.message))
            time.sleep(3)
        self.ss.stopEventLoop()
    
    def stopDeviceServer(self):
        self.scanning = False
    
    def errorFoundAction(self, m1, m2):
        self.log.ERROR("{} -- {}".format(m1,m2))
    
    def endErrorAction(self):
        pass

    def startDeviceAction(self, config):
        classid = iter(config).next().getKey()
        self.log.INFO("Trying to start {}...".format(classid))
        self.log.DEBUG("with the following configuration:\n{}".format(config))
        modified = Hash(config)
        modified[classid]["serverId"] = self.serverid
        if "deviceId" in modified[classid]:
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
            launcher = Launcher(pluginDir, modname, classid, modified)
            launcher.start()
            self.deviceInstanceMap[deviceid] = launcher
            del validated
        except Exception, e:
            self.log.WARN("Wrong input configuration for class '{}': {}".format(classid, e.message))
            return
        
    def notifyNewDeviceAction(self):
        deviceClasses = []
        for (classid, d) in self.availableDevices.items():
            deviceClasses.append(classid)
            if d['mustNotify']:
                d['mustNotify'] = False
        self.log.DEBUG("Sending instance update as new device plugins are available: {}".format(deviceClasses))
        self.ss.updateInstanceInfo(Hash("deviceClasses", deviceClasses))

    def noStateTransition(self):
        self.log.DEBUG("DeviceServer \"{}\" does not allow the transition for this event".format(self.serverid))
   
    def slotKillServer(self):
        self.log.INFO("Received kill signal")
        threads = []
        for deviceid  in self.deviceInstanceMap.keys():
            self.ss.call(deviceid, "slotKillDevice")
            threads.append(self.deviceInstanceMap[deviceid])
        for t  in threads:
            if t: t.join()
        self.deviceInstanceMap = {}
        self.ss.call("*", "slotDeviceServerInstanceGone", self.serverid)
        self.stopDeviceServer()
        
    def slotDeviceGone(self, id):
        self.log.WARN("Device \"{}\" notifies fulture death".format(id))
        if id in self.deviceInstanceMap:
            self.ss.call(id, "slotKillDeviceInstance")
            t = self.deviceInstanceMap[id]
            if t: t.join()
            del self.deviceInstanceMap[id]
            self.log.INFO("Device \"{}\" removed from server.".format(id))

    def slotGetClassSchema(self, classid):
        schema = Configurator(PythonDevice).getSchema(classid)
        self.ss.reply(schema)
        
    def processEvent(self, event):
        with self.processEventLock:
            self.fsm.process_event(event)
        
#    def updateCurrentState(self, currentState):
#        self.ss.reply(currentState)
    
    def _generateDefaultDeviceInstanceId(self, devClassId):
        cls = self.__class__
        with cls.instanceCountLock:
            if self.serverid not in cls.instanceCountPerDeviceServer:
                cls.instanceCountPerDeviceServer[self.serverid] = 0
            cls.instanceCountPerDeviceServer[self.serverid] += 1
            _index = cls.instanceCountPerDeviceServer[self.serverid]
            tokens = self.serverid.split("_")
            if tokens.pop() == str(os.getpid()):
                _domain = tokens.pop(0) + "-" + tokens.pop()
            _id = _domain + "_" + devClassId + "_" + str(_index)
            return _id
     

        
class Launcher(threading.Thread):

    def __init__(self, pluginDir, modname, classid, config):
        threading.Thread.__init__(self)
        
        # hide complaints from 'threading' module
        threading._DummyThread._Thread__stop = lambda x: 42
        
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
        self.pid = os.fork()
        if self.pid == 0:
            os.chmod(self.script, 0755)
            os.execvpe(self.script, self.args, os.environ)
        else:
            id, status = os.waitpid(self.pid, 0)
            print "Finally %r died" % (self.device)

    
def main(args):
    try:
        server = Runner(DeviceServer).instantiate(args)
        if server:
            server.run()
    except Exception,e:
        print "Exception caught: " + str(e)
    
    
if __name__ == '__main__':
    main(sys.argv)
