#!/usr/bin/env python
# -*- coding: utf-8 -*-

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Jul 26, 2012 10:06:25 AM$"

import os
import sys
import copy
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
        (
            STRING_ELEMENT(expected).key("serverId")
                    .displayedName("Server ID")
                    .description("The device-server instance id uniquely identifies a device-server instance in the distributed system")
                    .assignmentOptional().noDefaultValue().commit()
                    ,
            INT32_ELEMENT(expected).key("visibility")
                    .displayedName("Visibility")
                    .description("Configures who is allowed to see this server at all")
                    .assignmentOptional().defaultValue(AccessLevel.OBSERVER).options("0 1 2 3 4")
                    .adminAccess().reconfigurable()
                    .commit()
                    ,
            CHOICE_ELEMENT(expected).key("connection")
                    .displayedName("Connection")
                    .description("The connection to the communication layer of the distributed system")
                    .appendNodesOfConfigurationBase(BrokerConnection)
                    .assignmentOptional().defaultValue("Jms")
                    .expertAccess()
                    .init()
                    .commit()
                    ,                    
            #LIST_ELEMENT(expected).key("autoStart")
            #        .displayedName("Auto start")
            #        .description("Auto starts selected devices")
            #        .appendNodesOfConfigurationBase(PythonDevice)
            #        .assignmentOptional().noDefaultValue()
            #        .commit()
            #        ,
            #BOOL_ELEMENT(expected).key("scanPlugins")
            #        .displayedName("Scan plug-ins?")
            #        .description("Decides whether the server will scan the content of the plug-in folder and dynamically load found devices")
            #        .expertAccess()
            #        .assignmentOptional().defaultValue(True)
            #        .commit()
            #        ,
            PATH_ELEMENT(expected).key("pluginDirectory")
                    .displayedName("Plugin Directory")
                    .description("Directory to search for plugins")
                    .assignmentOptional().defaultValue("plugins")
                    .isDirectory()
                    .expertAccess()
                    .commit()
                    ,
            NODE_ELEMENT(expected).key("Logger")
                    .description("Logging settings")
                    .displayedName("Logger")
                    .appendParametersOfConfigurableClass(Logger,"Logger")
                    .commit()
                    ,
            NODE_ELEMENT(expected).key("Logger.rollingFile")
                    .description("Log Appender settings for file")
                    .displayedName("Rolling File Appender")
                    .appendParametersOfConfigurableClass(AppenderConfigurator,"RollingFile")
                    .advanced()
                    .commit()
                    ,
            OVERWRITE_ELEMENT(expected).key("Logger.rollingFile.layout")
                    .setNewDefaultValue("Pattern")
                    .commit()
                    ,
            OVERWRITE_ELEMENT(expected).key("Logger.rollingFile.layout.Pattern.format")
                    .setNewDefaultValue("%d{%F %H:%M:%S} %p  %c  : %m%n")
                    .commit()
                    ,
            OVERWRITE_ELEMENT(expected).key("Logger.rollingFile.filename")
                    .setNewDefaultValue("device-server.log")
                    .commit()
                    ,
            NODE_ELEMENT(expected).key("Logger.network")
                    .description("Log Appender settings for Network")
                    .displayedName("Network Appender")
                    .appendParametersOfConfigurableClass(AppenderConfigurator,"Network")
                    .advanced()
                    .commit()
                    ,
            NODE_ELEMENT(expected).key("Logger.ostream")
                    .description("Log Appender settings for terminal")
                    .displayedName("Ostream Appender")
                    .appendParametersOfConfigurableClass(AppenderConfigurator,"Ostream")
                    .advanced()
                    .commit()
                    ,
            OVERWRITE_ELEMENT(expected).key("Logger.ostream.layout")
                    .setNewDefaultValue("Pattern")
                    .commit()
                    ,
            OVERWRITE_ELEMENT(expected).key("Logger.ostream.layout.Pattern.format")
                    .setNewDefaultValue("%p  %c  : %m%n")
                    .commit()
                    ,
            NODE_ELEMENT(expected).key("Logger.karabo")
                    .description("Logger category for karabo framework")
                    .displayedName("Karabo framework logger")
                    .appendParametersOfConfigurableClass(CategoryConfigurator, "Category")
                    .advanced()
                    .commit()
                    ,
            OVERWRITE_ELEMENT(expected).key("Logger.karabo.name")
                    .setNewAssignmentOptional()
                    .setNewDefaultValue("karabo")
                    .commit()
                    ,
            OVERWRITE_ELEMENT(expected).key("Logger.karabo.additivity")
                    .setNewDefaultValue(False)
                    .commit()
                    ,
            OVERWRITE_ELEMENT(expected).key("Logger.karabo.appenders")
                    .setNewDefaultValue("RollingFile")
                    .commit()
                    ,
            OVERWRITE_ELEMENT(expected).key("Logger.karabo.appenders.RollingFile.layout")
                    .setNewDefaultValue("Pattern")
                    .commit()
                    ,
            OVERWRITE_ELEMENT(expected).key("Logger.karabo.appenders.RollingFile.layout.Pattern.format")
                    .setNewDefaultValue("%d{%F %H:%M:%S} %p  %c  : %m%n")
                    .commit()
                    ,
            OVERWRITE_ELEMENT(expected).key("Logger.karabo.appenders.RollingFile.filename")
                    .setNewDefaultValue("device-server.log")
                    .commit()
                    ,
        )

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
        #self.autoStart = []
        self.needScanPlugins = True
        
        # set serverId
        serverIdFileName = "serverId.xml"
        if os.path.isfile(serverIdFileName): 
            hash = loadFromFile(serverIdFileName) 
            if 'serverId' in input:
                self.serverid = input['serverId'] # Else whatever was configured
                saveToFile(Hash("DeviceServer.serverId", self.serverid), serverIdFileName, Hash("format.Xml.indentation", 3))
            elif 'DeviceServer.serverId' in hash:
                self.serverid = hash['DeviceServer.serverId'] # If file exists, it has priority
            else:
                print "WARN : Found serverId.xml without serverId contained"
                self.serverid = self._generateDefaultServerId() # If nothing configured -> generate
                saveToFile(Hash("DeviceServer.serverId", m_serverId), serverIdFileName, Hash("format.Xml.indentation", 3))
        else: # No file
            if 'serverId' in input:
                self.serverid = input['serverId']
            else:
                self.serverid = self._generateDefaultServerId()
            saveToFile(Hash("DeviceServer.serverId", self.serverid), serverIdFileName, Hash("format.Xml.indentation", 3))
        
        # Device configurations for those to automatically start
        #if "autoStart" in input:
        #    self.autoStart = input['autoStart']
            
        # Whether to scan for additional plug-ins at runtime
        #if "scanPlugins" in input:
        #    self.needScanPlugins = input['scanPlugins']
        
        # What visibility this server should have
        self.visibility = input.get("visibility")
        
        self.connectionType = iter(input['connection']).next().getKey()
        self.connectionParameters = copy.copy(input['connection.' + self.connectionType])
        self.pluginLoader = PluginLoader.create("PythonPluginLoader", Hash("pluginDirectory", input['pluginDirectory']))
        self.loadLogger(input)                
    
    def _generateDefaultServerId(self):
        return self.hostname + "_Server_" + str(os.getpid())
    
    def loadLogger(self, input):
        config = input["Logger"]

        # make a copy of additional appenders defined by user
        appenders = config["appenders"]

        # handle predefined DeviceServer appenders
        newAppenders = [Hash(), Hash(), Hash()]
        newAppenders[0].set("Ostream", config["ostream"])
        newAppenders[1].set("RollingFile", config["rollingFile"])
        newAppenders[2].set("Network", config["network"])
            
        del config["ostream"]
        del config["rollingFile"]
        del config["network"]

        for appr in appenders:
            if appr.has("Ostream"):
                if appr["Ostream.name"] == "default":
                    continue

            newAppenders.append(appr)
                
        config.set("appenders", newAppenders)
            
        config["appenders[2].Network.layout"] = Hash()
        config["appenders[2].Network.layout.Pattern.format"] = "%d{%F %H:%M:%S} | %p | %c | %m"
        if "connection" in input:
            config["appenders[2].Network.connection"] = input["connection"]
            
        category = config["karabo"]
        category["name"] = "karabo"
        config["categories[0].Category"] = category
        config["categories[0].Category.appenders[1].Ostream.layout.Pattern.format"] = "%p  %c  : %m%n"
        del config["karabo"]
#        print "loadLogger final:\n", config
        self.loggerConfiguration = Hash()
        self.loggerConfiguration += config
        Logger.configure(config)
        
    def run(self):
        self.log = Logger.getLogger(self.serverid)
        self.log.INFO("Starting Karabo DeviceServer on host: {}".format(self.hostname))
        self.ss = SignalSlotable.create(self.serverid, self.connectionType, self.connectionParameters)        
        self._registerAndConnectSignalsAndSlots()
        info = Hash("type", "server")
        info["serverId"] = self.serverid
        info["version"] = self.__class__.__version__
        info["host"] = self.hostname
        info["visibility"] = self.visibility
        # TODO
        t = threading.Thread(target=self.ss.runEventLoop, args = (10, info))
        t.start()
        time.sleep(0.01)  # for rescheduling, some garantie that runEventLoop will start before FSM
        self.fsm.start()
        t.join()
        self.pluginThread.join()
    
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
        if self.needScanPlugins:
            self.log.INFO("Keep watching directory: \"{}\" for Device plugins".format(self.pluginLoader.getPluginDirectory()))
            self.pluginThread = threading.Thread(target = self.scanPlugins)
            self.scanning = True
            self.pluginThread.start()
    
    def scanPlugins(self):
        self.blacklist = []
        self.availableModules = dict()
        while self.scanning:
            modules = self.pluginLoader.update()   # just list of modules in plugins dir
            for name, path in modules:
                if name in self.blacklist:
                    continue
                if name in self.availableModules:
                    continue
                try:
                    dname = os.path.dirname( os.path.realpath(path) )
                    if dname not in sys.path:
                        sys.path.append(dname)
                    module = __import__(name)
                except ImportError as e:
                    self.log.WARN("scanPlugins: Cannot import module {} -- {}".format(name,e))
                    continue
                if "PythonDevice" not in dir(module):
                    if name not in self.blacklist:
                        self.blacklist.append(name)
                    continue
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
                    tree = inspect.getclasstree(candidates, 1)  # build inheritance tree
                    try:
                        while True:
                            c,b = tree[0]
                            tree = tree[1]
                    except IndexError as e:
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
                except (RuntimeError, AttributeError) as e:
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
        # Input 'config' parameter comes from GUI or DeviceClient
        classid = iter(config).next().getKey()
        self.log.INFO("Trying to start {}...".format(classid))
        self.log.DEBUG("with the following configuration:\n{}".format(config))
        modified = copy.copy(config) 
        configuration = modified[classid]
        configuration["serverId"] = self.serverid
        if "deviceId" in configuration:
            deviceid = configuration["deviceId"]
        else:
            deviceid = self._generateDefaultDeviceInstanceId(classid)
            configuration["deviceId"] = deviceid
        # Add logger configuration from DeviceServer:
        configuration["Logger"] = copy.copy(self.loggerConfiguration)
        # create temporary instance to check the configuration parameters are valid
        try:
            pluginDir = self.pluginLoader.getPluginDirectory()
            modname = self.availableDevices[classid]["module"]
            module = __import__(modname)
            UserDevice = getattr(module, classid)
            schema = UserDevice.getSchema(classid)
            validator = Validator()
            self.log.DEBUG("Trying to validate  the configuration on device server")
            validated = validator.validate(schema, configuration)
            self.log.DEBUG("Validated configuration is ...\n{}".format(validated))
            launcher = Launcher(pluginDir, modname, classid, modified)
            launcher.start()
            self.deviceInstanceMap[deviceid] = launcher
            del validated
            self.ss.reply(True, deviceid)
        except Exception as e:
            self.log.WARN("Wrong input configuration for class '{}': {}".format(classid, e.message))
            return
        
    def notifyNewDeviceAction(self):
        deviceClasses = []
        visibilities = []
        for (classid, d) in self.availableDevices.items():
            deviceClasses.append(classid)
            if d['mustNotify']:
                d['mustNotify'] = False
            visibilities.append(d['xsd'].getDefaultValue("visibility"))
        self.log.DEBUG("Sending instance update as new device plugins are available: {}".format(deviceClasses))
        self.ss.updateInstanceInfo(Hash("deviceClasses", deviceClasses, "visibilities", visibilities))

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
            else:
                _id = self.serverid + "_" + devClassId + "_" + str(_index)
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
            # TODO Use higher level API here (i.e. writeToFile)
            cfg = Hash("filename", filename, "format.Xml.indentation", 2) 
            out = OutputHash.create("TextFile", cfg)
            out.write(config)
            self.args = [self.script, modname, classid, filename]
        except Exception as e:
            raise RuntimeError,"Exception happened while writing 'config' object: " + str(e) 

    def run(self):
        self.pid = os.fork()
        if self.pid == 0:
            os.chmod(self.script, 0755)
            os.execvp(self.script, self.args)
        else:
            id, status = os.waitpid(self.pid, 0)
            print "Finally %r died" % (self.device)

    
def main(args):
    try:
        server = Runner(DeviceServer).instantiate(args)
        if server:
            server.run()
    except Exception as e:
        print "Exception caught: " + str(e)
    
    
if __name__ == '__main__':
    main(sys.argv)
