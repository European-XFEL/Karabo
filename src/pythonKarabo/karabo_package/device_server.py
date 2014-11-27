#!/usr/bin/env python
# -*- coding: utf-8 -*-

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Jul 26, 2012 10:06:25 AM$"

from asyncio import (coroutine, get_event_loop, set_event_loop, sleep,
                     create_subprocess_exec, wait, wait_for)
import os
import os.path
import sys
import signal
import copy
import socket
import platform
import threading
import time
import inspect
from importlib import import_module
from subprocess import Popen, PIPE

if __name__ == "__main__":
    sys.karabo_api = 2

from karabo.decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from karabo.plugin_loader import PluginLoader
from karabo.device import Device, SignalSlotable
from karabo.hash import Hash, BinaryParser, BinaryWriter, XMLParser, saveToFile
from karabo.hashtypes import Schema, String, Int32
from karabo import hashtypes
from karabo.runner import Runner
from karabo.schema import Validator
from karabo.logger import Logger
from karabo.signalslot import (ConnectionType, Signal, replySlot)
from karabo.enums import AccessLevel, AccessMode, Assignment
from karabo.eventloop import EventLoop


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("DeviceServer", "1.0")
class DeviceServer(SignalSlotable):
    '''
    Device server serves as a launcher of python devices. It scans
    the 'plugins' directory for new plugins (python scripts) available
    and communicates its findings to master device server. It
    communicates XSD form of schema of user devices and starts such
    devices as separate process if user push "Initiate" button in GUI.
    '''

    serverId = String(
        displayedName="Server ID",
        description="The device-server instance id uniquely identifies a "
                    "device-server instance in the distributed system",
        assignment=Assignment.OPTIONAL)

    visibility = Int32(
        enum=AccessLevel, displayedName="Visibility",
        description="Configures who is allowed to see this server at all",
        assignment=Assignment.OPTIONAL,
        defaultValue=AccessLevel.OBSERVER, options=[0, 1, 2, 3, 4],
        requiredAccessLevel=AccessLevel.ADMIN,
        accessMode=AccessMode.RECONFIGURABLE)

    pluginDirectory = String(
        displayedName="Plugin Directory",
        description="Directory to search for plugins",
        assignment=Assignment.OPTIONAL, defaultValue="plugins",
        displayType="directory", requiredAccessLevel=AccessLevel.EXPERT)


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
            if hasattr(self, "serverId"):
                saveToFile(Hash("DeviceServer.serverId", self.serverId),
                           serverIdFileName)
            elif 'DeviceServer.serverId' in hash:
                self.serverId = hash['DeviceServer.serverId'] # If file exists, it has priority
            else:
                print("WARN : Found serverId.xml without serverId contained")
                self.serverId = self._generateDefaultServerId() # If nothing configured -> generate
                saveToFile(Hash("DeviceServer.serverId", m_serverId),
                           serverIdFileName)
        else: # No file
            if not hasattr(self, "serverId"):
                self.serverId = self._generateDefaultServerId()
            saveToFile(Hash("DeviceServer.serverId", self.serverId),
                       serverIdFileName)
        self.deviceId = self._deviceId_ = self.serverId

        self.pluginLoader = PluginLoader(
            Hash("pluginDirectory", self.pluginDirectory))
        self.pid = os.getpid()
        self.seqnum = 0


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
                Hash("Network", Hash("layout", Hash(
                    "Pattern", Hash("format",
                                    "%d{%F %H:%M:%S} | %p | %c | %m"))))])


    def run(self):
        self.log = Logger.getLogger(self.serverId)
        self.log.INFO("Starting Karabo DeviceServer on host: {}".
                      format(self.hostname))
        self._registerAndConnectSignalsAndSlots()
        info = self.info
        info["type"] = "server"
        info["serverId"] = self.serverId
        info["version"] = self.__class__.__version__
        info["host"] = self.hostname
        info["visibility"] = self.visibility.value
        self.async(self.scanPlugins())
        super().run()


    def _generateDefaultServerId(self):
        return self.hostname + "_Server_" + str(os.getpid())

    signalNewDeviceClassAvailable = Signal(
        hashtypes.String(), hashtypes.String(), hashtypes.Schema())
    signalClassSchema = Signal(hashtypes.Schema(), hashtypes.String(),
                               hashtypes.String())

    def _registerAndConnectSignalsAndSlots(self):
        self.signalNewDeviceClassAvailable.connect(
            "*", "slotNewDeviceClassAvailable", ConnectionType.NO_TRACK)

        self.signalClassSchema.connect("*", "slotClassSchema",
                                       ConnectionType.NO_TRACK)

    @coroutine
    def scanPlugins(self):
        availableModules = set()
        while self.running:
            for name in self.pluginLoader.update():
                if name in availableModules:
                    continue
                try:
                    module = import_module(name)
                except Exception as e:
                    self.log.WARN("scanPlugins: Cannot import module {} -- {}".
                                  format(name,e))
                else:
                    availableModules.add(name)
                    self.notifyNewDeviceAction()
            yield from sleep(3)


    def errorFoundAction(self, m1, m2):
        self.log.ERROR("{} -- {}".format(m1,m2))
    
    def endErrorAction(self):
        pass

    @coroutine
    def slotStartDevice(self, hash):
        config = Hash()

        if 'classId' in hash:
            classid, deviceid, config = self.parseNew(hash)
        else:
            classid, deviceid, config = self.parseOld(hash)
        config["Logger"] = self.loggerConfiguration

        # create temporary instance to check the configuration parameters are valid
        try:
            pluginDir = self.pluginLoader.pluginDirectory
            cls = Device.subclasses[classid]
            self.deviceInstanceMap[deviceid] = yield from cls.launch(config)
            return (True, deviceid)
        except Exception as e:
            self.log.WARN("Wrong input configuration for class '{}': {}".
                          format(classid, e))
            raise
            return

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
            "deviceClasses", [k for k in Device.subclasses.keys()],
            "visibilities", [c.visibility.defaultValue.value
                             for c in Device.subclasses.values()]))

    @coroutine
    def slotKillServer(self):
        if self.deviceInstanceMap:
            self._ss.emit("call", {k: ["slotKillDevice"]
                                   for k in self.deviceInstanceMap})
            done, pending = yield from wait(
                [v.wait() for v in self.deviceInstanceMap.values()], timeout=10)
            if pending:
                print("some devices could not be killed")
        self.stopEventLoop()
        self._ss.emit("call", {"*": ["slotDeviceServerInstanceGone"]},
                      self.serverId)


    @coroutine
    def slotDeviceGone(self, id):
        gone = self.deviceInstanceMap.pop(id, None)
        if gone is not None:
            try:
                yield from wait_for(gone.wait(), 10)
            except TimeoutError:
                print('device "{}" claimed to have gone but did not'.
                      format(id))

    @replySlot("slotClassSchema")
    @coroutine
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
    server = Runner(DeviceServer).instantiate(args)
    if server:
        server.run()
        try:
            loop.run_forever()
        finally:
            loop.close()


if __name__ == '__main__':
    main(sys.argv)
