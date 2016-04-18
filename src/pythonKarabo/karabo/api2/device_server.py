# !/usr/bin/env python
# -*- coding: utf-8 -*-

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Jul 26, 2012 10:06:25 AM$"

from asyncio import async, coroutine, set_event_loop, sleep, wait
import copy
import os
import sys
import socket
import traceback

import numpy

from .device import Device
from .enums import AccessLevel, AccessMode, Assignment
from .eventloop import EventLoop
from .hash import (Hash, XMLParser, saveToFile, SchemaHashType, String,
                   StringList, Int32)
from .logger import Logger
from .output import KaraboStream
from .plugin_loader import PluginLoader
from .schema import Validator, Node
from .signalslot import SignalSlotable, Signal, slot, coslot

# XXX: These imports are needed for their side-effects...
import karabo.api2.metamacro  # add a default Device MetaMacro
import karabo.api2.ipython


class DeviceServer(SignalSlotable):
    '''
    Device server serves as a launcher of python devices. It scans
    the 'plugins' directory for new plugins (python scripts) available
    and communicates its findings to master device server. It
    communicates XSD form of schema of user devices and starts such
    devices as separate process if user push "Initiate" button in GUI.
    '''
    __version__ = "1.3"

    serverId = String(
        displayedName="Server ID",
        description="The device-server instance id uniquely identifies a "
                    "device-server instance in the distributed system",
        assignment=Assignment.OPTIONAL)

    visibility = Int32(
        enum=AccessLevel, displayedName="Visibility",
        description="Configures who is allowed to see this server at all",
        assignment=Assignment.OPTIONAL,
        defaultValue=AccessLevel.OBSERVER,
        requiredAccessLevel=AccessLevel.ADMIN,
        accessMode=AccessMode.RECONFIGURABLE)

    pluginDirectory = String(
        displayedName="Plugin Directory",
        description="Directory to search for plugins",
        assignment=Assignment.OPTIONAL, defaultValue="plugins",
        displayType="directory", requiredAccessLevel=AccessLevel.EXPERT)

    pluginNamespace = String(
        displayedName="Plugin Namespace",
        description="Namespace to search for plugins",
        assignment=Assignment.OPTIONAL,
        defaultValue="karabo.python_device.api_2",
        requiredAccessLevel=AccessLevel.EXPERT)

    log = Node(Logger,
               description="Logging settings",
               displayedName="Logger",
               requiredAccessLevel=AccessLevel.EXPERT)

    instanceCountPerDeviceServer = { }


    def __init__(self, configuration):
        super().__init__(configuration)
        self.deviceInstanceMap = { }
        self.hostname, _, self.domainname = socket.gethostname().partition('.')
        self.needScanPlugins = True

        # set serverId
        serverIdFileName = "serverId.xml"
        if os.path.isfile(serverIdFileName):
            with open(serverIdFileName, 'r') as fin:
                p = XMLParser()
                hash = p.read(fin.read())
            if self.serverId != "__none__":
                saveToFile(Hash("DeviceServer.serverId", self.serverId),
                           serverIdFileName)
            elif 'DeviceServer.serverId' in hash:
                self.serverId = hash['DeviceServer.serverId'] # If file exists, it has priority
            else:
                print("WARN : Found serverId.xml without serverId contained")
                self.serverId = self._generateDefaultServerId() # If nothing configured -> generate
                saveToFile(Hash("DeviceServer.serverId", self.serverId),
                           serverIdFileName)
        else: # No file
            if self.serverId == "__none__":
                self.serverId = self._generateDefaultServerId()
            saveToFile(Hash("DeviceServer.serverId", self.serverId),
                       serverIdFileName)
        self.deviceId = self._deviceId_ = self.serverId

        self.pluginLoader = PluginLoader(
            Hash("pluginNamespace", self.pluginNamespace,
                 "pluginDirectory", self.pluginDirectory))
        self.pid = os.getpid()
        self.seqnum = 0

        # legacy devices need the following information
        self.loggerConfiguration = Hash(
            "categories", [Hash("Category", Hash(
                "name", "karabo", "additivity", False,
                "appenders", [Hash("RollingFile", Hash(
                    "layout", Hash("Pattern", Hash(
                        "format", "%d{%F %H:%M:%S} %p  %c  : %m%n")),
                    "filename", "device-server.log"))]))],
            "appenders", [
                Hash("Ostream", Hash("layout", Hash(
                    "Pattern", Hash("format", "%p %c  : %m%n")))),
                Hash("RollingFile", Hash(
                    "layout",
                    Hash("Pattern", Hash("format",
                                         "%d{%F %H:%M:%S} %p  %c  : %m%n")),
                    "filename", "device-server.log")),
                # network appender has fixed layout => nothing to specify here
                Hash("Network", Hash())
            ])

    def run(self):
        self._ss.enter_context(self.log.setBroker(self._ss))

        self.logger = self.log.logger
        super().run()

        info = Hash()
        info["type"] = "server"
        info["serverId"] = self.serverId
        info["version"] = self.__class__.__version__
        info["host"] = self.hostname
        info["visibility"] = self.visibility.value
        self.updateInstanceInfo(info)

        self.log.INFO("Starting Karabo DeviceServer on host: {}".
                      format(self.hostname))
        self.notifyNewDeviceAction()
        async(self.scanPlugins())
        sys.stdout = KaraboStream(sys.stdout)
        sys.stderr = KaraboStream(sys.stderr)

    def _generateDefaultServerId(self):
        return self.hostname + "_Server_" + str(os.getpid())

    @coroutine
    def scanPlugins(self):
        availableModules = set()
        while self.running:
            entrypoints = self.pluginLoader.update()
            for ep in entrypoints:
                if ep.name in availableModules:
                    continue
                try:
                    # Call the entrypoint,
                    # This registers the contained device class
                    ep.load()
                except Exception as e:
                    self.logger.exception(
                        "scanPlugins: Cannot load plugin {} -- {}"
                        .format(ep.name, e))
                    traceback.print_exc()
                else:
                    availableModules.add(ep.name)
                    self.notifyNewDeviceAction()
            yield from sleep(3)


    def errorFoundAction(self, m1, m2):
        self.log.ERROR("{} -- {}".format(m1,m2))
    
    def endErrorAction(self):
        pass

    @slot
    def slotStartDevice(self, hash):
        config = Hash()

        if 'classId' in hash:
            classId, deviceId, config = self.parseNew(hash)
        else:
            classId, deviceId, config = self.parseOld(hash)
        config["Logger"] = self.loggerConfiguration

        try:
            cls = Device.subclasses[classId]
            obj = cls(config)
            obj.startInstance(self)
            return True, '"{}" started'.format(deviceId)
        except Exception as e:
            self.logger.exception('could not start device "{}" of class "{}"'.
                                  format(deviceId, classId))
            return False, traceback.format_exc()

    def addChild(self, deviceId, child):
        self.deviceInstanceMap[deviceId] = child

    def parseNew(self, hash):
        classid = hash['classId']
        self.log.INFO("Trying to start {}...".format(classid))
        self.log.DEBUG("with the following configuration:\n{}".format(hash))

        # Get configuration
        config = copy.copy(hash['configuration'])

        # Inject serverId
        config['_serverId_'] = self.serverId

        # Inject deviceId
        if 'deviceId' in hash and hash['deviceId']:
            config['_deviceId_'] = hash['deviceId']
        else:
            config['_deviceId_'] = self._generateDefaultDeviceInstanceId(
                                                                    classid)

        # Add logger configuration from DeviceServer:
        config['Logger'] = copy.copy(self.loggerConfiguration)
        return classid, config['_deviceId_'], config


    def parseOld(self, hash):
         # Input 'config' parameter comes from GUI or DeviceClient
        classid = iter(hash).next().getKey()
        self.log.INFO("Trying to start {}...".format(classid))
        self.log.DEBUG("with the following configuration:\n{}".format(hash))        
        configuration = copy.copy(hash[classid])
        configuration["_serverId_"] = self.serverId
        deviceid = str()
        if "deviceId" in configuration:
            deviceid = configuration["deviceId"]
        else:
            deviceid = self._generateDefaultDeviceInstanceId(classid)
            
        configuration["_deviceId_"] = deviceid
        # Add logger configuration from DeviceServer:
        configuration["Logger"] = copy.copy(self.loggerConfiguration)
        return classid, deviceid, configuration

        
    def notifyNewDeviceAction(self):
        self.updateInstanceInfo(Hash(
            "deviceClasses", StringList([k for k in Device.subclasses.keys()]),
            "visibilities", numpy.array([c.visibility.defaultValue.value
                             for c in Device.subclasses.values()])))

    @coslot
    def slotKillServer(self):
        if self.deviceInstanceMap:
            done, pending = yield from wait(
                [d.slotKillDevice() for d in self.deviceInstanceMap.values()],
                timeout=10)

            if pending:
                self.logger.warning("some devices could not be killed")
        self.stopEventLoop()
        self._ss.emit("call", {"*": ["slotDeviceServerInstanceGone"]},
                      self.serverId)

    @slot
    def slotInstanceGone(self, id, info):
        self.deviceInstanceMap.pop(id, None)

    @coslot
    def slotGetClassSchema(self, classid):
        cls = Device.subclasses[classid]
        return (yield from cls.getClassSchema_async()), classid, self.serverId

    def _generateDefaultDeviceInstanceId(self, devClassId):
        cnt = self.instanceCountPerDeviceServer.setdefault(self.serverId, 0)
        self.instanceCountPerDeviceServer[self.serverId] += 1
        tokens = self.serverId.split("_")
        if tokens[-1] == "{}".format(os.getpid()):
            return "{}-{}_{}_{}".format(tokens[0], tokens[-2],
                                        devClassId, cnt + 1)
        else:
            return "{}_{}_{}".format(self.serverId, devClassId, cnt + 1)


def main(args):
    loop = EventLoop()
    set_event_loop(loop)
    v = Validator()
    h = Hash({k: v for k, v in (a.split("=", 2) for a in args[1:])})
    validated = v.validate(DeviceServer.getClassSchema(), h)
    server = DeviceServer(validated["DeviceServer"])
    if server:
        server.startInstance()
        try:
            loop.run_forever()
        finally:
            loop.close()


if __name__ == '__main__':
    main(sys.argv)
