from asyncio import (async, coroutine, create_subprocess_exec, Future, gather,
                     get_event_loop, set_event_loop, sleep, TimeoutError, wait,
                     wait_for)
import copy
import os
import sys
import socket
from subprocess import PIPE

import numpy

from .enums import AccessLevel, AccessMode, Assignment
from .eventloop import EventLoop
from .hash import Bool, Hash, Int32, String, VectorString
from .logger import Logger
from .output import KaraboStream
from .plugin_loader import PluginLoader
from .schema import Node
from .signalslot import SignalSlotable, slot, coslot


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
        description="Namespace to search for middle layer plugins",
        assignment=Assignment.OPTIONAL,
        defaultValue="karabo.middlelayer_device",
        requiredAccessLevel=AccessLevel.EXPERT)

    boundNamespace = String(
        displayedName="Bound Namespace",
        description="Namespace to search for bound API plugins",
        assignment=Assignment.OPTIONAL,
        defaultValue="karabo.bound_device",
        requiredAccessLevel=AccessLevel.EXPERT)

    deviceClasses = VectorString(
        displayedName="Device Classes",
        description="The device classes the server will manage",
        assignment=Assignment.OPTIONAL, defaultValue="",
        requiredAccessLevel=AccessLevel.EXPERT)

    scanPluginsTask = None

    @Bool(
        displayedName="Scan plug-ins?",
        description="Decides whether the server will scan the content of the "
                    "plug-in folder and dynamically load found devices",
        assignment=Assignment.OPTIONAL, defaultValue=True,
        requiredAccessLevel=AccessLevel.EXPERT)
    @coroutine
    def scanPlugins(self, value):
        if value and self.scanPluginsTask is None:
            self.scanPluginsTask = async(self.scanPluginsLoop())
        elif not value and self.scanPluginsTask is not None:
            self.scanPluginsTask.cancel()
            self.scanPluginsTask = None
        self.scanPlugins = self.scanPluginsTask is not None

    log = Node(Logger,
               description="Logging settings",
               displayedName="Logger",
               requiredAccessLevel=AccessLevel.EXPERT)

    instanceCountPerDeviceServer = {}

    def __init__(self, configuration):
        super().__init__(configuration)
        self.deviceInstanceMap = {}
        self.newDeviceFutures = {}
        self.hostname, _, self.domainname = socket.gethostname().partition('.')
        self.needScanPlugins = True
        if not hasattr(self, 'serverId'):
            self.serverId = self._generateDefaultServerId()

        self.deviceId = self._deviceId_ = self.serverId

        self.pluginLoader = PluginLoader(
            Hash("pluginNamespace", self.pluginNamespace,
                 "pluginDirectory", self.pluginDirectory))
        self.boundLoader = PluginLoader(
            Hash("pluginNamespace", self.boundNamespace,
                 "pluginDirectory", self.pluginDirectory))
        self.plugins = {}
        self.processes = {}
        self.bounds = {}
        self.pid = os.getpid()
        self.seqnum = 0

    def _initInfo(self):
        info = super(DeviceServer, self)._initInfo()
        info["type"] = "server"
        info["serverId"] = self.serverId
        info["version"] = self.__class__.__version__
        info["host"] = self.hostname
        info["visibility"] = self.visibility.value
        info.merge(self.deviceClassesHash())
        return info

    @coroutine
    def _run(self, **kwargs):
        yield from super(DeviceServer, self)._run(**kwargs)
        self._ss.enter_context(self.log.setBroker(self._ss))
        self.logger = self.log.logger

        yield from self.pluginLoader.update()
        if self.pluginNamespace:
            yield from self.scanPluginsOnce()
        if self.boundNamespace:
            yield from self.scanBoundsOnce()
        self.updateInstanceInfo(self.deviceClassesHash())

        self.logger.info("Starting Karabo DeviceServer on host: %s",
                         self.hostname)
        sys.stdout = KaraboStream(sys.stdout)
        sys.stderr = KaraboStream(sys.stderr)

    def _generateDefaultServerId(self):
        return self.hostname + "_Server_" + str(os.getpid())

    @coroutine
    def scanPluginsLoop(self):
        """every 3 s, check whether there are new entry points"""
        while True:
            yield from self.pluginLoader.update()
            if ((self.pluginNamespace and (yield from self.scanPluginsOnce()))
                    or (self.boundNamespace and (
                        yield from self.scanBoundsOnce()))):
                self.updateInstanceInfo(self.deviceClassesHash())
            yield from sleep(3)

    @coroutine
    def scanPluginsOnce(self):
        """load all available entry points, return whether new plugin found"""
        changes = False
        classes = set(self.deviceClasses)
        entrypoints = self.pluginLoader.list_plugins(self.pluginNamespace)
        for ep in entrypoints:
            if ep.name in self.plugins or (classes and ep.name not in classes):
                continue
            try:
                self.plugins[ep.name] = (yield from get_event_loop().
                                         run_in_executor(None, ep.load))
                changes = True
            except Exception:
                self.logger.exception('Cannot load middle layer plugin "%s"',
                                      ep.name)
        return changes

    @coroutine
    def scanBoundsOnce(self):
        changes = False
        classes = set(self.deviceClasses)
        entrypoints = self.pluginLoader.list_plugins(self.boundNamespace)
        for ep in entrypoints:
            if ep.name in self.bounds or (classes and ep.name not in classes):
                continue
            try:
                env = dict(os.environ)
                env["PYTHONPATH"] = self.pluginDirectory
                process = yield from create_subprocess_exec(
                    sys.executable, "-m", "karabo.bound_api.launcher",
                    "schema", self.boundNamespace, ep.name,
                    env=env, stdout=PIPE)
                schema = yield from process.stdout.read()
                yield from process.wait()
                self.bounds[ep.name] = Hash.decode(schema, "XML")[ep.name]
                changes = True
            except Exception:
                self.logger.exception('Cannot load bound plugin "%s"', ep.name)
        return changes


    def errorFoundAction(self, m1, m2):
        self.log.ERROR("{} -- {}".format(m1, m2))

    def endErrorAction(self):
        pass

    @coslot
    def slotStartDevice(self, hash):
        config = Hash()

        if 'classId' in hash:
            classId, deviceId, config = self.parseNew(hash)
        else:
            classId, deviceId, config = self.parseOld(hash)

        if classId in self.plugins:
            try:
                cls = self.plugins[classId]
                obj = cls(config)
                task = obj.startInstance(self)
                yield from task
                return True, '"{}" started'.format(deviceId)
            except Exception as e:
                e.logmessage = ('could not start device "%s" of class "%s"',
                                deviceId, classId)
                raise
        elif classId in self.bounds:
            try:
                env = dict(os.environ)
                env["PYTHONPATH"] = self.pluginDirectory
                future = self.newDeviceFutures.setdefault(deviceId, Future())
                process = yield from create_subprocess_exec(
                    sys.executable, "-m", "karabo.bound_api.launcher",
                    "run", self.boundNamespace, classId,
                    env=env, stdin=PIPE)
                self.processes[deviceId] = process
                process.stdin.write(config.encode("XML"))
                process.stdin.close()
                yield from future
                return True, '"{}" started'.format(deviceId)
            except Exception as e:
                e.logmessage = ('could not start device "%s" of class "%s"',
                                deviceId, classId)
                raise

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

        return classid, config['_deviceId_'], config

    def parseOld(self, hash):
        # Input 'config' parameter comes from GUI or DeviceClient
        classid = next(iter(hash))
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
        return classid, deviceid, configuration

    def deviceClassesHash(self):
        visibilities = ([c.visibility.defaultValue.value
                         for c in self.plugins.values()]
                        + [c.hash["visibility", "defaultValue"]
                           for c in self.bounds.values()])
        return Hash(
            "deviceClasses",
            list(self.plugins.keys()) + list(self.bounds.keys()),
            "visibilities", numpy.array(visibilities))

    @coslot
    def slotKillServer(self):
        for device in self.processes:
            self._ss.emit("call", {device: ["slotKillDevice"]})
        if self.deviceInstanceMap:
            done, pending = yield from wait(
                [d.slotKillDevice() for d in self.deviceInstanceMap.values()],
                timeout=10)

            if pending:
                self.logger.warning("some devices could not be killed")
        try:
            yield from wait_for(
                gather(*(p.wait() for p in self.processes.values())),
                timeout=10)
        except TimeoutError:
            self.logger.exception("some processes did not finish in time")
            for p in self.processes.values():
                p.kill()
        yield from self.slotKillDevice()
        self.stopEventLoop()
        self._ss.emit("call", {"*": ["slotDeviceServerInstanceGone"]},
                      self.serverId)

    @slot
    def slotInstanceNew(self, instanceId, info):
        future = self.newDeviceFutures.get(instanceId)
        if future is not None:
            future.set_result(info)
        super(DeviceServer, self).slotInstanceNew(instanceId, info)

    @slot
    def slotInstanceGone(self, id, info):
        self.deviceInstanceMap.pop(id, None)
        super(DeviceServer, self).slotInstanceGone(id, info)

    @slot
    def slotGetClassSchema(self, classid):
        if classid in self.plugins:
            cls = self.plugins[classid]
            return cls.getClassSchema(), classid, self.serverId
        elif classid in self.bounds:
            return self.bounds[classid], classid, self.serverId
        raise RuntimeError("Unknown class {}".format(classid))

    def _generateDefaultDeviceInstanceId(self, devClassId):
        cnt = self.instanceCountPerDeviceServer.setdefault(self.serverId, 0)
        self.instanceCountPerDeviceServer[self.serverId] += 1
        tokens = self.serverId.split("_")
        if tokens[-1] == "{}".format(os.getpid()):
            return "{}-{}_{}_{}".format(tokens[0], tokens[-2],
                                        devClassId, cnt + 1)
        else:
            return "{}_{}_{}".format(self.serverId, devClassId, cnt + 1)


def main(args=None):
    try:
        # Change directory to $KARABO/var/data
        os.chdir(os.path.join(os.environ['KARABO'], 'var', 'data'))
    except KeyError:
        print("ERROR: $KARABO is not defined. Make sure you have sourced the "
              "'activate' script.")
        return

    args = args or sys.argv
    loop = EventLoop()
    set_event_loop(loop)

    # This function parses a dict with potentially nested values
    # ('.' seperated) and adds them flat as attribtues to the class
    def get(para):
        d = DeviceServer
        for p in para.split('.'):
            d = getattr(d, p)
        return d

    # The fromstring function takes over proper type conversion
    params = Hash({k: get(k).fromstring(v)
                   for k, v in (a.split("=", 2) for a in args[1:])})
    server = DeviceServer(params)
    if server:
        server.startInstance()
        try:
            loop.run_forever()
        finally:
            loop.close()


if __name__ == '__main__':
    main(sys.argv)
