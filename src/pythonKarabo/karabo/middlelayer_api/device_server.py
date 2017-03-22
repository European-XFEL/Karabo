from asyncio import (async, coroutine, create_subprocess_exec, Future, gather,
                     get_event_loop, set_event_loop, sleep, TimeoutError, wait,
                     wait_for)
import copy
import os
import sys
import socket
from subprocess import PIPE

import numpy

from .basetypes import isSet
from .enums import AccessLevel, AccessMode, Assignment
from .eventloop import EventLoop
from .hash import Bool, Hash, Int32, String, VectorString
from .logger import Logger
from .output import KaraboStream
from .plugin_loader import PluginLoader
from .schema import Node
from .serializers import decodeBinary, encodeXML
from .signalslot import SignalSlotable, slot, coslot
from .synchronization import firstCompleted


class DeviceServerBase(SignalSlotable):
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

    instanceCount = 1

    def __init__(self, configuration):
        super().__init__(configuration)
        self.newDeviceFutures = {}
        self.hostname, _, self.domainname = socket.gethostname().partition('.')
        self.needScanPlugins = True
        if not isSet(self.serverId):
            self.serverId = self._generateDefaultServerId()

        self.pluginLoader = PluginLoader(
            Hash("pluginDirectory", self.pluginDirectory))

        self.deviceId = self._deviceId_ = self.serverId

        self.plugins = {}
        self.processes = {}
        self.bounds = {}
        self.pid = os.getpid()
        self.seqnum = 0

    def _initInfo(self):
        info = super(DeviceServerBase, self)._initInfo()
        info["type"] = "server"
        info["serverId"] = self.serverId
        info["version"] = self.__class__.__version__
        info["host"] = self.hostname
        info["visibility"] = self.visibility.value
        info.merge(self.deviceClassesHash())
        return info

    @coroutine
    def _run(self, **kwargs):
        yield from super(DeviceServerBase, self)._run(**kwargs)
        self._ss.enter_context(self.log.setBroker(self._ss))
        self.logger = self.log.logger

        yield from self.pluginLoader.update()
        yield from self.scanPluginsOnce()
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
            if (yield from self.scanPluginsOnce()):
                self.updateInstanceInfo(self.deviceClassesHash())
            yield from sleep(3)

    @coroutine
    def scanPluginsOnce(self):
        """load all available entry points, return whether new plugin found

        This is an endpoint for multiple inheritance. Specializations should
        try to load their plugins, and return whether a new plugin was found.
        They should then call ``super()``, as another specialization may
        also need to find new plugins. The default implementation returns
        `False`, as it can never load a plugin.
        """
        return False

    @coslot
    def slotStartDevice(self, hash):
        classId, deviceId, config = self.parse(hash)
        try:
            return (yield from self.startDevice(classId, deviceId, config))
        except Exception as e:
            e.logmessage = ('could not start device "%s" of class "%s"',
                            deviceId, classId)
            raise

    @coroutine
    def startDevice(self, classId, deviceId, config):
        """Start the device `deviceId`

        This is an endpoint of multiple inheritance. Every specialization
        should start a device if it can, or otherwise call super(). This is
        the failback that just raises an error.
        """
        raise RuntimeError('Unknown class')

    def parse(self, hash):
        classId = hash['classId']
        self.logger.info("Trying to start %s...", classId)
        self.logger.debug("with the following configuration:\n%s", hash)

        # Get configuration
        config = copy.copy(hash['configuration'])

        # Inject serverId
        config['_serverId_'] = self.serverId

        # Inject deviceId
        if 'deviceId' in hash and hash['deviceId']:
            config['_deviceId_'] = hash['deviceId']
        else:
            config['_deviceId_'] = self._generateDefaultDeviceId(classId)

        return classId, config['_deviceId_'], config

    def deviceClassesHash(self):
        visibilities = self.getVisibilities()
        if visibilities:
            return Hash("deviceClasses", list(visibilities),
                        "visibilities",
                        numpy.array(list(visibilities.values())))
        else:
            return Hash()

    def getVisibilities(self):
        """return a dictionary of class visibility.

        This is an endpoint for multiple inheritance. This method should
        return a dictionary that maps the names of all available classes
        to their visibilities. The default implementation returns an empty
        dict, all specializations should add their classes to it.
        """
        return {}

    @coslot
    def slotKillServer(self):
        yield from self.slotKillDevice()
        self.stopEventLoop()
        self._ss.emit("call", {"*": ["slotDeviceServerInstanceGone"]},
                      self.serverId)

    @slot
    def slotInstanceNew(self, instanceId, info):
        future = self.newDeviceFutures.get(instanceId)
        if future is not None:
            future.set_result(info)
        super(DeviceServerBase, self).slotInstanceNew(instanceId, info)

    @slot
    def slotGetClassSchema(self, classId):
        """Return the schema of class `classId`

        This is an endpoint for multiple inheritance. Any specialization
        should return the schema for `classId`, or call super(). This fail
        back implementation just raises an error.
        """
        raise RuntimeError('Unknown class "{}"'.format(classId))

    def _generateDefaultDeviceId(self, devClassId):
        self.instanceCount += 1
        tokens = self.serverId.split("_")
        if tokens[-1] == str(self.pid):
            return "{}-{}_{}_{}".format(tokens[0], tokens[-2],
                                        devClassId, self.instanceCount)
        else:
            return "{}_{}_{}".format(self.serverId, devClassId,
                                     self.instanceCount)

    @classmethod
    def main(cls, argv=None):
        try:
            os.chdir(os.path.join(os.environ['KARABO'], 'var', 'data'))
        except KeyError:
            print("ERROR: $KARABO is not defined. Make sure you have sourced "
                  "the 'activate' script.")
            return

        if argv is None:
            argv = sys.argv

        loop = EventLoop()
        set_event_loop(loop)

        # This function parses a dict with potentially nested values
        # ('.' seperated) and adds them flat as attributes to the class
        def get(para):
            d = cls
            for p in para.split('.'):
                d = getattr(d, p)
            return d

        # The fromstring function takes over proper type conversion
        params = Hash({k: get(k).fromstring(v)
                       for k, v in (a.split("=", 1) for a in argv[1:])})
        server = cls(params)
        if server:
            server.startInstance()
            try:
                loop.run_forever()
            finally:
                loop.close()


class MiddleLayerDeviceServer(DeviceServerBase):
    pluginNamespace = String(
        displayedName="Plugin Namespace",
        description="Namespace to search for middle layer plugins",
        assignment=Assignment.OPTIONAL,
        defaultValue="karabo.middlelayer_device",
        requiredAccessLevel=AccessLevel.EXPERT)

    def __init__(self, configuration):
        super(MiddleLayerDeviceServer, self).__init__(configuration)
        self.deviceInstanceMap = {}

    @coroutine
    def scanPluginsOnce(self):
        changes = yield from super(
            MiddleLayerDeviceServer, self).scanPluginsOnce()
        if not self.pluginNamespace:
            return changes
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

    def getVisibilities(self):
        visibilities = super(MiddleLayerDeviceServer, self).getVisibilities()
        visibilities.update((k, v.visibility.defaultValue.value)
                            for k, v in self.plugins.items())
        return visibilities

    @slot
    def slotGetClassSchema(self, classId):
        cls = self.plugins.get(classId)
        if cls is not None:
            return cls.getClassSchema(), classId, self.serverId
        return super(MiddleLayerDeviceServer, self).slotGetClassSchema(classId)

    @coroutine
    def startDevice(self, classId, deviceId, config):
        cls = self.plugins.get(classId)
        if cls is None:
            return (yield from super(MiddleLayerDeviceServer, self)
                    .startDevice(classId, deviceId, config))
        obj = cls(config)
        task = obj.startInstance(self)
        yield from task
        return True, '"{}" started'.format(deviceId)

    @coslot
    def slotKillServer(self):
        if self.deviceInstanceMap:
            done, pending = yield from wait(
                [d.slotKillDevice() for d in self.deviceInstanceMap.values()],
                timeout=10)

            if pending:
                self.logger.warning("some devices could not be killed")
        yield from super(MiddleLayerDeviceServer, self).slotKillServer()

    def addChild(self, deviceId, child):
        self.deviceInstanceMap[deviceId] = child

    @slot
    def slotInstanceGone(self, id, info):
        self.deviceInstanceMap.pop(id, None)
        super(DeviceServerBase, self).slotInstanceGone(id, info)


class BoundDeviceServer(DeviceServerBase):
    boundNamespace = String(
        displayedName="Bound Namespace",
        description="Namespace to search for bound API plugins",
        assignment=Assignment.OPTIONAL,
        defaultValue="karabo.bound_device",
        requiredAccessLevel=AccessLevel.EXPERT)

    bannedClasses = VectorString(
        displayedName="Banned Classes",
        description="Device classes banned from scanning "
                    "as they made problems",
        defaultValue=[], requiredAccessLevel=AccessLevel.EXPERT)

    @coroutine
    def scanPluginsOnce(self):
        changes = yield from super(BoundDeviceServer, self).scanPluginsOnce()
        if not self.boundNamespace:
            return changes
        classes = set(self.deviceClasses)
        class_ban = set(self.bannedClasses)
        entrypoints = self.pluginLoader.list_plugins(self.boundNamespace)
        for ep in entrypoints:
            if (ep.name in self.bounds or (classes and ep.name not in classes)
                    or ep.name in class_ban):
                continue
            try:
                env = dict(os.environ)
                env["PYTHONPATH"] = self.pluginDirectory
                process = yield from create_subprocess_exec(
                    sys.executable, "-m", "karabo.bound_api.launcher",
                    "schema", self.boundNamespace, ep.name,
                    env=env, stdout=PIPE)
                try:
                    schema = yield from process.stdout.read()
                    yield from process.wait()
                except:
                    process.kill()
                    raise
                self.bounds[ep.name] = decodeBinary(schema)[ep.name]
                changes = True
            except Exception:
                class_ban.add(ep.name)
                self.logger.exception('Cannot load bound plugin "%s"', ep.name)
        self.bannedClasses = list(class_ban)
        return changes

    def getVisibilities(self):
        visibilities = super(BoundDeviceServer, self).getVisibilities()
        visibilities.update((k, v.hash["visibility", "defaultValue"])
                            for k, v in self.bounds.items())
        return visibilities

    @slot
    def slotGetClassSchema(self, classId):
        schema = self.bounds.get(classId)
        if schema is not None:
            return schema, classId, self.serverId
        return super(BoundDeviceServer, self).slotGetClassSchema(classId)

    @coroutine
    def startDevice(self, classId, deviceId, config):
        if classId not in self.bounds:
            return (yield from super(BoundDeviceServer, self)
                    .startDevice(classId, deviceId, config))
        env = dict(os.environ)
        env["PYTHONPATH"] = self.pluginDirectory
        future = self.newDeviceFutures.setdefault(deviceId, Future())
        process = yield from create_subprocess_exec(
            sys.executable, "-m", "karabo.bound_api.launcher",
            "run", self.boundNamespace, classId,
            env=env, stdin=PIPE)
        self.processes[deviceId] = process
        process.stdin.write(encodeXML(config).encode('utf8'))
        process.stdin.close()
        done, pending = yield from firstCompleted(
            ok=future, error=process.wait())
        if "ok" in done:
            return True, '"{}" started'.format(deviceId)
        else:
            return False, '"{}" could not be started'.format(deviceId)

    @coslot
    def slotKillServer(self):
        for device in self.processes:
            self._ss.emit("call", {device: ["slotKillDevice"]})
        try:
            yield from wait_for(
                gather(*(p.wait() for p in self.processes.values())),
                timeout=10)
        except TimeoutError:
            self.logger.exception("some processes did not finish in time")
            for p in self.processes.values():
                p.kill()
        yield from super(BoundDeviceServer, self).slotKillServer()


class DeviceServer(MiddleLayerDeviceServer, BoundDeviceServer):
    pass


if __name__ == '__main__':
    DeviceServer.main(sys.argv)
