#!/usr/bin/env python
# -*- coding: utf-8 -*-

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Jul 26, 2012 10:06:25 AM$"

import os
import os.path
import sys
import signal
import copy
import socket
import platform
from subprocess import Popen
import threading
import time
import inspect

from karathon import (
    CHOICE_ELEMENT, INT32_ELEMENT, NODE_ELEMENT, OVERWRITE_ELEMENT,
    PATH_ELEMENT, STRING_ELEMENT,
    AccessLevel, AppenderConfigurator, BrokerConnection, CategoryConfigurator,
    Hash, Logger, Priority, Schema, SignalSlotable,
    loadFromFile, saveToFile
)
from .configurator import Configurator
from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from .device import PythonDevice
from .fsm import (
    KARABO_FSM_EVENT0, KARABO_FSM_EVENT2,
    KARABO_FSM_ACTION2, KARABO_FSM_NO_TRANSITION_ACTION,
    KARABO_FSM_STATE, KARABO_FSM_STATE_E,
    KARABO_FSM_CREATE_MACHINE, KARABO_FSM_STATE_MACHINE
)
from .plugin_loader import PluginLoader
from .runner import Runner


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("DeviceServer", "1.0")
class DeviceServer(object):
    """
    Device server serves as a launcher of python devices.  It scans 'plugins' directory
    for new plugins (python scripts) available and communicates its findings to master device
    server.  It communicates XSD form of schema of user devices and starts such devices as
    separate process if user push "Initiate" button in GUI
    """

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
            STRING_ELEMENT(expected).key("pluginNamespace")
                    .displayedName("Plugin Namespace")
                    .description("Namespace to search for plugins")
                    .assignmentOptional().defaultValue("karabo.python_device.api_1")
                    .expertAccess()
                    .commit()
                    ,
            STRING_ELEMENT(expected).key("pluginNames")
                    .displayedName("Devices to Load")
                    .description("Comma separated list of class names of devices which should be loaded")
                    .assignmentOptional().defaultValue("")
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
        # **************************************************************
        # *                        Events                              *
        # **************************************************************
        
        KARABO_FSM_EVENT2(self, 'ErrorFoundEvent', 'errorFound')
        KARABO_FSM_EVENT0(self, 'ResetEvent', 'reset')

        # **************************************************************
        # *                        States                              *
        # **************************************************************

        KARABO_FSM_STATE_E('Ok', self.okStateOnEntry)
        KARABO_FSM_STATE('Error')

        # **************************************************************
        # *                    Transition Actions                      *
        # **************************************************************

        KARABO_FSM_ACTION2('ErrorFoundAction', self.errorFoundAction, str, str)
        KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)
                    
        DeviceServerMachineSTT=[
                                ('Ok', 'ErrorFoundEvent', 'Error', 'ErrorFoundAction', 'none'),
                                ('Error', 'ResetEvent',   'Ok',    'none',   'none')
                               ]
        
        KARABO_FSM_STATE_MACHINE('DeviceServerMachine', DeviceServerMachineSTT, 'Ok')
        
        return KARABO_FSM_CREATE_MACHINE('DeviceServerMachine')
        
    def signal_handler(self, signum, frame):
        self.signal_handled = True
        if signum == signal.SIGINT:
            print('INTERRUPT : You pressed Ctrl-C!')
        else:
            print('INTERRUPT : You terminated me!')
        if self.ss is not None:
            self.ss.call("", "slotKillServer")
        else:
            self.slotKillServer()
        self.signalSlotableThread.join()
        self.signalSlotableThread = None
        self.pluginThread.join()
        self.pluginThread = None
        self.ss = None
        self.logger = None
        # time.sleep(3)
        sys.exit()
        # os._exit(0)
    
    def __init__(self, input):
        '''Constructor'''
        if input is None:
            raise ValueError(
                "Input configuration for constructor should be Hash, not None")
        super(DeviceServer, self).__init__()
        # describe FSM
        self.processEventLock = threading.RLock()
        self.fsm = self.setupFsm()

        self.ss = self.log = self.logger = self.loggerConfiguration = None
        self.signal_handled = False
        signal.signal(signal.SIGINT, self.signal_handler)
        signal.signal(signal.SIGTERM, self.signal_handler)
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
                print("WARN : Found serverId.xml without serverId contained")
                self.serverid = self._generateDefaultServerId() # If nothing configured -> generate
                saveToFile(Hash("DeviceServer.serverId", self.serverid), serverIdFileName, Hash("format.Xml.indentation", 3))
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
        
        self.connectionType = next(iter(input['connection'])).getKey()
        self.connectionParameters = copy.copy(input['connection.' + self.connectionType])
        self.pluginLoader = PluginLoader.create(
            "PythonPluginLoader",
            Hash("pluginNamespace", input["pluginNamespace"],
                 "pluginDirectory", input["pluginDirectory"],
                 "pluginNames", input["pluginNames"]))
        self.loadLogger(input)
        self.pid = os.getpid()
        self.seqnum = 0
    
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
            
        # network appender has fixed format (the one expected by the GUI)
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
        self.logger = Logger.configure(config)
        
    def run(self):
        self.log = Logger.getLogger(self.serverid)
        self.ss = SignalSlotable.create(self.serverid, self.connectionType, self.connectionParameters)        
        self._registerAndConnectSignalsAndSlots()

        self.log.INFO("Starting Karabo DeviceServer on host: {0.hostname}, "
                      "serverId: {0.serverid}, Broker (host:port:topic): {1}:"
                      "{2}".format(self, self.ss.getBrokerHost(),
                                   self.ss.getBrokerTopic()))

        info = Hash("type", "server")
        info["serverId"] = self.serverid
        info["version"] = self.__class__.__version__
        info["host"] = self.hostname
        info["visibility"] = self.visibility
        # TODO
        self.signalSlotableThread = threading.Thread(target=self.ss.runEventLoop, args = (10, info))
        self.signalSlotableThread.start()
        time.sleep(0.01)  # for rescheduling, some garantie that runEventLoop will start before FSM
        
        # if our own instanceId is used on topic -- exit
        ok = self.ss.ensureOwnInstanceIdUnique()
        if not ok:
            self.log.ERROR("DeviceServer '{0.serverid}' could not start since "
                           "its id already exists.".format(self))
            self.signalSlotableThread.join()
            return
                
        self.fsm.start()
        signal.pause()
    
    def _registerAndConnectSignalsAndSlots(self):
        self.ss.registerSlot(self.slotStartDevice)
        self.ss.registerSlot(self.slotKillServer)
        self.ss.registerSlot(self.slotDeviceGone)
        self.ss.registerSlot(self.slotGetClassSchema)
        self.ss.registerSlot(self.slotLoggerPriority)

    def onStateUpdate(self, currentState):
        #self.ss.reply(currentState)
        pass
        
    def okStateOnEntry(self):
        self.log.INFO("DeviceServer with id '{0.serverid}' starts up on host "
                      "'{0.hostname}'".format(self))
        if self.needScanPlugins:
            self.log.INFO("Keep watching path: '{0.pluginDirectory}' and "
                          "namespace: '{0.pluginNamespace}' for Device "
                          "plugins".format(self.pluginLoader))
            self.pluginThread = threading.Thread(target = self.scanPlugins)
            self.scanning = True
            self.pluginThread.start()
    
    def import_plugin(self, name):
        """ Return the device class defined by an entrypoint.
        """
        entrypoint = self.pluginLoader.getPlugin(name)
        return entrypoint.load()

    def scanPlugins(self):
        self.availableModules = {}
        while self.scanning:
            entrypoints = self.pluginLoader.update()
            for ep in entrypoints:
                if ep.name in self.availableModules:
                    continue
                try:
                    deviceClass = ep.load()
                except ImportError as e:
                    self.log.WARN("scanPlugins: Cannot import module {} -- {}".format(ep.name, e))
                    continue

                classid = deviceClass.__classid__
                try:
                    schema = Configurator(PythonDevice).getSchema(classid)
                    self.availableModules[ep.name] = classid
                    self.availableDevices[classid] = {"mustNotify": True, "module": ep.name, "xsd": schema}
                    self.newPluginAvailable()
                    self.log.INFO('Successfully loaded plugin: "{}:{}"'.format(ep.module_name, ep.name))
                except (RuntimeError, AttributeError) as e:
                    self.log.ERROR("Failure while building schema for class {}, base class {} and bases {} : {}".format(
                        classid, deviceClass.__base_classid__, deviceClass.__bases_classid__, e.message))
            time.sleep(3)
        self.ss.stopEventLoop()
    
    def stopDeviceServer(self):
        self.scanning = False
        if not self.signal_handled:
            pid = os.getpid()
            os.kill(pid, signal.SIGTERM)
    
    def errorFoundAction(self, m1, m2):
        self.log.ERROR("{} -- {}".format(m1,m2))
    
    def slotStartDevice(self, configuration):
        self.instantiateDevice(configuration)

    def instantiateDevice(self, input_config):
        classid = input_config['classId']

        # Get configuration
        config = copy.copy(input_config['configuration'])

        # Inject serverId
        config['_serverId_'] = self.serverid

        # Inject deviceId
        if 'deviceId' not in input_config or len(input_config['deviceId']) == 0:
            config['_deviceId_'] = self._generateDefaultDeviceInstanceId(classid)
        else:
            config['_deviceId_'] = input_config['deviceId']

        self.log.INFO("Trying to start a '{}' with device id '{}'"
                      "...".format(classid, config['_deviceId_']))
        self.log.DEBUG("with the following configuration:\n{}".format(input_config))

        # Add connection type and parameters used by device server for connecting to broker
        config['_connection_.' + self.connectionType] = self.connectionParameters
        
        # Add logger configuration from DeviceServer:
        config['Logger'] = copy.copy(self.loggerConfiguration)

        # create temporary instance to check the configuration parameters are valid
        try:
            if "_deviceId_" in config:
                deviceid = config["_deviceId_"]
            else:
                msg = "Access to {}._deviceId_ failed".format(classid)
                raise RuntimeError(msg)

            modname = self.availableDevices[classid]["module"]
            # NOTE: Device class should be imported using self.import_plugin()
            # and device schema should be validated here.

            filename = "/tmp/{}.{}.configuration_{}_{}.xml".format(modname, classid, self.pid, self.seqnum)
            while os.path.isfile(filename):
                self.seqnum += 1
                filename = "/tmp/{}.{}.configuration_{}_{}.xml".format(modname, classid, self.pid, self.seqnum)
            saveToFile(config, filename, Hash("format.Xml.indentation", 2))
            params = [self.pluginLoader.pluginDirectory, modname, classid, filename]
            
            launcher = Launcher(params)
            launcher.start()
            self.deviceInstanceMap[deviceid] = launcher
            self.ss.reply(True, deviceid)
        except Exception as e:
            self.log.WARN("Device '{}' could not be started because: {}".format(classid, e))
            self.ss.reply(False, "Device '{}' could not be started because: {}".format(classid, e))

    def newPluginAvailable(self):
        deviceClasses = []
        visibilities = []
        for (classid, d) in list(self.availableDevices.items()):
            deviceClasses.append(classid)
            if d['mustNotify']:
                d['mustNotify'] = False
            visibilities.append(d['xsd'].getDefaultValue("visibility"))
        self.log.DEBUG("Sending instance update as new device plugins are available: {}".format(deviceClasses))
        self.ss.updateInstanceInfo(Hash("deviceClasses", deviceClasses, "visibilities", visibilities))

    
    def noStateTransition(self):
        self.log.WARN("DeviceServer \"{}\" does not allow the transition for this event.".format(self.serverid))
   
    def slotKillServer(self):
        self.log.INFO("Received kill signal")
        threads = []
        for deviceid  in list(self.deviceInstanceMap.keys()):
            self.ss.call(deviceid, "slotKillDevice")
            threads.append(self.deviceInstanceMap[deviceid])
        for t  in threads:
            if t: t.join()
        self.deviceInstanceMap = {}
        self.ss.reply(self.serverid)
        self.stopDeviceServer()
        self.log.DEBUG("slotKillServer DONE")
        
    def slotDeviceGone(self, instanceId):
        # Would prefer a self.log.FRAMEWORK_INFO as in C++ instead of self.log.DEBUG:
        self.log.DEBUG("Device '{0}' notifies '{1.serverid}' about its future "
                       "death.".format(instanceId, self))
        if instanceId in self.deviceInstanceMap:
            t = self.deviceInstanceMap[instanceId]
            if t: t.join()
            del self.deviceInstanceMap[instanceId]
            self.log.INFO("Device '{}' removed from server.".format(instanceId))

    def slotGetClassSchema(self, classid):
        try:
            schema = Configurator(PythonDevice).getSchema(classid)
            self.ss.reply(schema, classid, self.serverid)
        except AttributeError as e:
            self.log.WARN("Replied empty schema due to : {}".format(str(e)))
            self.ss.reply(Schema(), classid, self.serverid)
        
    def slotLoggerPriority(self, newprio):
        oldprio = Priority.getPriorityName(self.logger.getLogger("some_deviceId").getRootPriority())
        self.logger.getLogger("some_deviceId").setRootPriority(Priority.getPriorityValue(newprio))
        self.log.INFO("Logger Priority changed : {} ==> {}".format(oldprio, newprio))
        
    def processEvent(self, event):
        with self.processEventLock:
            self.fsm.process_event(event)
        
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


class Launcher(object):
    """ Launches a PythonDevice in its own interpreter.
    """

    def __init__(self, params):
        self.args = params

    def start(self):
        args = [sys.executable, "-c"]
        args.append("from karabo.api1.device import launchPythonDevice;"
                    "launchPythonDevice()")
        args.extend(self.args)

        self.child = Popen(args)

    def join(self):
        if self.child.poll() is None:
            self.child.wait()


def main(args=None):
    args = args or sys.argv
    try:
        server = Runner(DeviceServer).instantiate(args)
        if server:
            server.run()
    except Exception as e:
        print("Exception caught: " + str(e))

if __name__ == '__main__':
    main()
