# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import copy
import getpass
import os
import socket
import sys
import traceback
from asyncio import (
    TimeoutError, all_tasks, create_subprocess_exec, gather, get_event_loop,
    set_event_loop, sleep, wait_for)
from collections import ChainMap
from enum import Enum
from importlib.metadata import entry_points
from json import loads
from signal import SIGTERM
from subprocess import PIPE

import numpy

from karabo import __version__ as karaboVersion
from karabo.common.api import KARABO_LOGGER_CONTENT_DEFAULT, ServerFlags
from karabo.native import (
    AccessLevel, AccessMode, Assignment, Bool, Descriptor, Hash, Int32,
    KaraboError, Node, String, TimeMixin, VectorString, decodeBinary,
    encodeBinary, get_timestamp, isSet)

from .configuration import validate_init_configuration
from .eventloop import EventLoop
from .heartbeat_mixin import HeartBeatMixin
from .logger import CacheLog, build_logger_node
from .output import KaraboStream
from .signalslot import SignalSlotable, slot
from .synchronization import (
    FutureDict, allCompleted, background, firstCompleted)

INIT_DESCRIPTION = """A JSON object representing the devices to be initialized.
It should be formatted like a dictionary of dictionaries.

For example:
        {"deviceId" :{
            "classId": "TheClassName",
            "stringProperty": "Value",
            "floatProperty": 42.0
            }
        }.
"""
SCAN_PLUGINS_TIME = 3


class DeviceServerBase(SignalSlotable):
    __version__ = "1.3"

    serverId = String(
        displayedName="Server ID",
        description="The device-server instance id uniquely identifies a \n"
                    "device-server instance in the distributed system",
        assignment=Assignment.OPTIONAL)

    hostName = String(
        displayedName="Forced Hostname",
        description="The hostname can be optionally forced to a specific "
        "string. The host's definition will be used if not specified.",
        assignment=Assignment.OPTIONAL,
        requiredAccessLevel=AccessLevel.EXPERT)

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
        assignment=Assignment.OPTIONAL, defaultValue=[],
        requiredAccessLevel=AccessLevel.EXPERT)

    _device_initializer = {}

    scanPlugins = Bool(
        displayedName="Scan plug-ins?",
        description="Unused variable.",
        assignment=Assignment.OPTIONAL, defaultValue=True,
        accessMode=AccessMode.INITONLY,
        requiredAccessLevel=AccessLevel.EXPERT)

    log = Node(build_logger_node(),
               description="Logging settings",
               displayedName="Logger",
               requiredAccessLevel=AccessLevel.EXPERT)

    init = String(
        description=INIT_DESCRIPTION,
        assignment=Assignment.INTERNAL,
        accessMode=AccessMode.INITONLY)

    instanceCount = 1

    serverFlags = VectorString(
        defaultValue=[],
        description="ServerFlags describing the device server, "
                    "the values must correspond to the enum ServerFlags",
        assignment=Assignment.INTERNAL,
        accessMode=AccessMode.INITONLY)

    def list_plugins(self, namespace):
        return list(entry_points(group=namespace))

    def __init__(self, configuration):
        super().__init__(configuration)
        if not isSet(self.hostName):
            self.hostName = socket.gethostname().partition('.')[0]
        if not isSet(self.serverId):
            self.serverId = self._generateDefaultServerId()

        self.deviceId = self._deviceId_ = self.serverId
        self.deviceInstanceMap = {}

        self.plugins = {}
        self.plugin_errors = {}
        self.processes = {}
        self.bounds = {}
        self.pid = os.getpid()
        self.seqnum = 0

    def _initInfo(self):
        info = super()._initInfo()
        info["type"] = "server"
        info["serverId"] = self.serverId
        info["version"] = self.__class__.__version__
        info["host"] = self.hostName
        info["visibility"] = self.visibility.value
        info["user"] = getpass.getuser()
        info["lang"] = "python"
        info["log"] = self.log.level
        info.merge(self.deviceClassesHash())

        serverFlags = 0
        for flag in self.serverFlags.value:
            if flag in ServerFlags.__members__:
                serverFlags |= ServerFlags[flag]
            else:
                raise NotImplementedError(
                    f"Provided serverFlag is not supported: {flag}")
        info["serverFlags"] = serverFlags

        return info

    async def _run(self, **kwargs):
        # Scan the plugins very early to send the full instanceInfo
        # before coming online.
        await self.scanPluginsOnce()
        await super()._run(**kwargs)
        self._ss.enter_context(self.log.setBroker(self._ss))
        self.logger = self.log.logger
        self.logger.info(
            f"Starting Karabo {karaboVersion} DeviceServer "
            f"(pid: {self.pid}) on host {self.hostName}, topic "
            f"{get_event_loop().topic}")
        for name, detail in self.plugin_errors.items():
            self.logger.error(
                f"Problem loading device class {name}: {detail}")

        sys.stdout = KaraboStream(sys.stdout)
        sys.stderr = KaraboStream(sys.stderr)

    def _generateDefaultServerId(self):
        return self.hostName + "_Server_" + str(os.getpid())

    async def scanPluginsOnce(self):
        """load all available entry points, return whether new plugin found

        This is an endpoint for multiple inheritance. Specializations should
        try to load their plugins, and return whether a new plugin was found.
        They should then call ``super()``, as another specialization may
        also need to find new plugins. The default implementation returns
        `False`, as it can never load a plugin.
        """
        return False

    @slot
    async def slotStartDevice(self, hash):
        classId, deviceId, config = self.parse(hash)
        if deviceId in self.deviceInstanceMap:
            raise KaraboError(
                f"Device {deviceId} is already running/starting "
                "on this server")
        try:
            return (await self.startDevice(classId, deviceId, config))
        except BaseException as e:
            e.logmessage = ('Could not start device "%s" of class "%s"',
                            deviceId, classId)
            raise

    async def startDevice(self, classId, deviceId, config):
        """Start the device `deviceId`

        This is an endpoint of multiple inheritance. Every specialization
        should start a device if it can, or otherwise call super(). This is
        the failback that just raises an error.
        """
        raise RuntimeError(f"Unknown class '{classId}'")

    def parse(self, hash):
        classId = hash['classId']

        # Get configuration
        config = copy.copy(hash['configuration'])

        # Inject serverId (drop the timestamp)
        config['_serverId_'] = self.serverId.value

        # Inject hostname (drop the timestamp)
        config['hostName'] = self.hostName.value

        # Inject deviceId
        if 'deviceId' in hash and hash['deviceId']:
            deviceId = hash['deviceId']
        else:
            deviceId = self._generateDefaultDeviceId(classId)

        config['_deviceId_'] = deviceId
        self.logger.info('Trying to start "%s" of class "%s"',
                         deviceId, classId)
        self.logger.debug("with the following configuration:\n%s", hash)

        return classId, deviceId, config

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

    async def slotKillServer(self, message=None):
        await self.slotKillDevice(message)
        # Stop event loop on the next cycle ...
        await sleep(0.1)
        get_event_loop().call_soon(self.stopEventLoop)

    slotKillServer = slot(slotKillServer, passMessage=True)

    @slot
    def slotGetClassSchema(self, classId):
        """Return the schema of class `classId`

        This is an endpoint for multiple inheritance. Any specialization
        should return the schema for `classId`, or call super(). This fail
        back implementation just raises an error.
        """
        raise RuntimeError(f'Unknown class "{classId}"')

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
    def print_usage(cls, argv=None):
        def print_properties(klass, prefix):
            for attribute_name in dir(klass):
                attribute = getattr(klass, attribute_name)
                if isinstance(attribute, Descriptor):
                    if attribute.accessMode == AccessMode.READONLY:
                        continue
                    name = ""
                    if attribute.displayedName is not None:
                        name = attribute.displayedName
                    print(f"{prefix}{attribute_name}: {name}")
                    print(f"    Type: {type(attribute).__name__}")
                    if attribute.defaultValue is not None:
                        print(f"    Default: {attribute.defaultValue}")
                        if isinstance(attribute.defaultValue, Enum):
                            enum_value = attribute.defaultValue.value
                            print(f"           : {enum_value}")
                    if attribute.description is not None:
                        print("    Description:")
                        print(f"        {attribute.description}")
                    print()

                if isinstance(attribute, Node):
                    print_properties(attribute.cls, f"{attribute_name}.")

        print(f"usage: {argv[0]} [property=value] ...\n")
        print(cls.__doc__)
        print("available properties:")
        print("-------------------------------")
        print_properties(cls, "")

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

        if "--help" in argv or "-h" in argv:
            cls.print_usage(argv)
            return

        loop = EventLoop()
        set_event_loop(loop)

        # This function parses a dict with potentially nested values
        # ('.' seperated) and adds them flat as attributes to the class
        def get(para):
            d = cls
            for p in para.split('.'):
                try:
                    d = getattr(d, p, None) or getattr(d.cls, p)
                except AttributeError:
                    raise KaraboError(f"Device server does not have "
                                      f"property `{para}`!")
            return d

        # Split the params for basic properties
        params = {k: v for k, v in (a.split("=", 1) for a in argv[1:])}

        # Retrieve the init information if available and convert to json
        _device_initializer = loads(params.pop("init", "{}"))

        # The fromstring function takes over proper type conversion
        params = Hash({k: get(k).fromstring(v) for k, v in params.items()})
        # Servers have 20 seconds heartbeat interval
        params["heartbeatInterval"] = 20
        server = cls(params)
        if server:
            server._device_initializer = _device_initializer

            def sig_kill_handler():
                """Kill the server on sigterm"""
                loop = get_event_loop()
                loop.create_task(server.slotKillServer(), instance=server)

            if os.name == "nt":
                print(
                    "WARNING: process interruptions will not be handled "
                    "gracefully due to limitations of the operating system."
                )
            else:
                loop.add_signal_handler(SIGTERM, sig_kill_handler)
            # NOTE: The server listens to broadcasts and we set a flag in the
            # signal slotable
            server.is_server = True
            try:
                loop.run_until_complete(
                    server.startInstance(broadcast=True))
            except BaseException:
                # ServerId is already in use for KaraboError
                # User might cancel even with Interrupt and a
                # slotKillDevice is running. We wait for finish off
                pending = all_tasks(loop)
                loop.run_until_complete(
                    wait_for(gather(*pending, return_exceptions=False),
                             timeout=5))
            else:
                try:
                    loop.run_forever()
                except KeyboardInterrupt:
                    pass
                finally:
                    # Close the loop with the broker connection
                    loop.close()
                    # Always terminate the process
                    os.kill(os.getpid(), SIGTERM)

    async def onInitialization(self):
        """Initialization coroutine of the server to start up devices
        """
        if not self._device_initializer:
            return

        for deviceId, initializer in self._device_initializer.items():
            configuration = Hash(initializer)
            configuration["_deviceId_"] = deviceId
            configuration["_serverId_"] = self.serverId
            configuration["hostName"] = self.hostName
            classId = configuration.pop("classId")
            background(self.startDevice(classId, deviceId, configuration))
        # Not required information anymore!
        del self._device_initializer


class MiddleLayerDeviceServer(HeartBeatMixin, DeviceServerBase):
    pluginNamespace = String(
        displayedName="Plugin Namespace",
        description="Namespace to search for middle layer plugins",
        assignment=Assignment.OPTIONAL,
        defaultValue="karabo.middlelayer_device",
        requiredAccessLevel=AccessLevel.EXPERT)

    timeServerId = String(
        displayedName="TimeServer",
        accessMode=AccessMode.INITONLY)

    def __init__(self, configuration):
        super().__init__(configuration)

    async def _run(self, **kwargs):
        await super()._run(**kwargs)
        if isSet(self.timeServerId):
            await self._ss.async_connect(self.timeServerId, "signalTimeTick",
                                         self.slotTimeTick)

    @slot
    def slotLoggerContent(self, info):
        """Slot call to receive logger content from the print logger

        :param info: input Hash
        """
        logs = int(info.get("logs", KARABO_LOGGER_CONTENT_DEFAULT))
        content = CacheLog.summary(logs)

        return Hash("serverId", self.serverId, "content", content)

    @slot
    def slotTimeTick(self, train_id, sec, frac, period):
        """Slot called by the timeServer

        :param train_id: Propagated train Id
        :param sec: time since epoch in seconds
        :param frac: remaining time in attoseconds
        :param period: update interval between train Id's in microsec
        """
        TimeMixin.set_reference(train_id, sec, frac, period)

    @slot
    def slotGetTime(self, info=None):
        """Return the actual time information of this server

        This slot call return a Hash with key ``time`` and the attributes
        provide an actual timestamp with train Id information.

        The slot call further provides a reference time information via key
        ``reference`` and the attributes provide the train id.

        This method has an empty input argument to allow a generic protocol.
        """
        h = Hash("time", True)
        h["time", ...].update(get_timestamp().toDict())
        h["timeServerId"] = self.timeServerId.value or "None"
        h["reference"] = True
        h["reference", ...].update(TimeMixin.toDict())

        return h

    @slot
    def slotLoggerPriority(self, level: str):
        """Set the logger priority of the server and all children"""
        self.log.level = level
        for device in self.deviceInstanceMap.values():
            device.log.level = level
        self.updateInstanceInfo(
            Hash("log", level))

    async def scanPluginsOnce(self):
        changes = await super().scanPluginsOnce()
        if not self.pluginNamespace:
            return changes
        classes = set(self.deviceClasses)
        entrypoints = self.list_plugins(self.pluginNamespace)
        for ep in entrypoints:
            if ep.name in self.plugins or (classes and ep.name not in classes):
                continue
            try:
                self.plugins[ep.name] = (await get_event_loop().
                                         run_in_executor(None, ep.load))
                changes = True
            except BaseException:
                details = traceback.format_exc()
                self.plugin_errors[ep.name] = details

        return changes

    def getVisibilities(self):
        visibilities = super().getVisibilities()
        visibilities.update((k, v.visibility.defaultValue.value)
                            for k, v in self.plugins.items())
        return visibilities

    @slot
    def slotGetClassSchema(self, classId):
        cls = self.plugins.get(classId)
        if cls is not None:
            return cls.getClassSchema(), classId, self.serverId
        return super().slotGetClassSchema(classId)

    async def startDevice(self, classId, deviceId, config):
        cls = self.plugins.get(classId)
        if cls is None:
            return (await super().startDevice(classId, deviceId, config))
        # Server validates the configuration of the device
        invalid = validate_init_configuration(
            cls.getClassSchema(), config)
        if invalid:
            raise RuntimeError(
                f'Configuration for {deviceId} validation failed: {invalid}"')
        if "log.level" not in config:
            config["log.level"] = self.log.level
        if isSet(self.timeServerId):
            config["__timeServerId"] = self.timeServerId
        obj = cls(config)
        task = obj.startInstance(self, broadcast=False)
        await task
        return True, deviceId

    async def slotKillServer(self, message=None):
        instanceId = (self._ss.get_property(message, "signalInstanceId")
                      if message is not None else "OS signal")
        self.logger.info("Received request to shutdown server "
                         f"from {instanceId}.")
        if self.deviceInstanceMap:
            # first check if there are device instances to be removed
            futures = {instanceId: dev.slotKillDevice(message)
                       for instanceId, dev in self.deviceInstanceMap.items()}
            done, *errors = await allCompleted(**futures, timeout=10)
            # SlotKillDevice returns a success boolean if all tasks could be
            # destroyed on time
            if not all(list(done.values())):
                devices = ', '.join(done.keys())
                self.logger.warning(
                    f"Some devices could not be killed: {devices}")

            errors = ChainMap(*errors)
            if errors:
                devices = ', '.join(errors.keys())
                self.logger.error(
                    "Some devices had exceptions when they "
                    f"were shutdown: {devices} --- {errors.values()}")

        # then kill the server
        await super().slotKillServer(message)

        return self.serverId

    slotKillServer = slot(slotKillServer, passMessage=True)

    def addChild(self, deviceId, child):
        self.deviceInstanceMap[deviceId] = child

    def removeChild(self, deviceId):
        self.deviceInstanceMap.pop(deviceId, None)

    @slot
    async def slotInstanceNew(self, instanceId, info):
        await super().slotInstanceNew(instanceId, info)
        if (info.get("classId") == "TimeServer" and
                instanceId == self.timeServerId):
            await self._ss.async_connect(self.timeServerId, "signalTimeTick",
                                         self.slotTimeTick)
        # Forward the broadcast to the device instances!
        await gather(*[dev.slotInstanceNew(instanceId, info)
                       for dev in self.deviceInstanceMap.values()])

    @slot
    def slotInstanceGone(self, instanceId, info):
        super().slotInstanceGone(instanceId, info)
        # Forward the broadcast to the device instances!
        for device in self.deviceInstanceMap.values():
            device.slotInstanceGone(instanceId, info)

    @slot
    def slotInstanceUpdated(self, instanceId, info):
        super().slotInstanceUpdated(instanceId, info)
        # Forward the broadcast to the device instances!
        for device in self.deviceInstanceMap.values():
            device.slotInstanceUpdated(instanceId, info)


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

    def __init__(self, configuration):
        super().__init__(configuration)
        self._process_futures = FutureDict()

    async def scanPluginsOnce(self):
        changes = await super().scanPluginsOnce()
        if not self.boundNamespace:
            return changes
        classes = set(self.deviceClasses)
        class_ban = set(self.bannedClasses)
        entrypoints = self.list_plugins(self.boundNamespace)
        for ep in entrypoints:
            if (ep.name in self.bounds or (classes and
                                           ep.name not in classes) or
                    ep.name in class_ban):
                continue
            try:
                env = dict(os.environ)
                env["PYTHONPATH"] = self.pluginDirectory
                process = await create_subprocess_exec(
                    sys.executable, "-m", "karabo.bound.launcher",
                    "schema", self.boundNamespace, ep.name,
                    env=env, stdout=PIPE)
                try:
                    schema = await process.stdout.read()
                    await process.wait()
                except BaseException:
                    process.kill()
                    raise
                if process.returncode != 0:
                    # the parsing failed. raise an error so we do not repeat
                    msg = f"schema conversion returned {process.returncode}"
                    raise RuntimeError(msg)
                self.bounds[ep.name] = decodeBinary(schema)[ep.name]
                changes = True
            except BaseException:
                class_ban.add(ep.name)
                self.logger.exception('Cannot load bound plugin "%s"', ep.name)
        self.bannedClasses = list(class_ban)
        return changes

    def getVisibilities(self):
        visibilities = super().getVisibilities()
        visibilities.update((k, v.hash["visibility", "defaultValue"])
                            for k, v in self.bounds.items())
        return visibilities

    @slot
    def slotGetClassSchema(self, classId):
        schema = self.bounds.get(classId)
        if schema is not None:
            return schema, classId, self.serverId
        return super().slotGetClassSchema(classId)

    @slot
    def slotDeviceGone(self, instanceId):
        self.logger.info("Device '{0}' notifies '{1.serverId}' about its "
                         "future death.".format(instanceId, self))
        self.deviceInstanceMap.pop(instanceId, None)
        self.processes.pop(instanceId, None)

    async def supervise(self, deviceId, process, info):
        async def supervisor():
            while True:
                await sleep(100)
                for i in range(5):
                    try:
                        nonlocal info
                        info = await wait_for(
                            self.call(deviceId, "slotPing", deviceId,
                                      1, False),
                            timeout=5)
                        break
                    except TimeoutError:
                        self.logger.info('could not ping device "%s"',
                                         deviceId)
                    except KaraboError as e:
                        self.logger.warn('ping failed for device "%s": %s',
                                         deviceId, e.args[0])
                else:
                    self.logger.warn('terminating non-responding device "%s"',
                                     deviceId)
                    process.terminate()
                    try:
                        await sleep(5)
                        self.logger.warn('killing non-responding device "%s"',
                                         deviceId)
                        process.kill()
                    finally:
                        self._ss.emit("call", {"*": ['slotInstanceGone']},
                                      deviceId, info)
                        self.processes.pop(deviceId, None)

        task = background(supervisor())
        try:
            await process.wait()
        finally:
            task.cancel()

    async def startDevice(self, classId, deviceId, config):
        if classId not in self.bounds:
            return (await super().startDevice(classId, deviceId, config))
        if "Logger.priority" not in config:
            config["Logger.priority"] = self.log.level
        # Logger settings for the three appenders (ostream, file, cache)
        # would have to be passed via config["_logger_"], as a copy of the
        # settings of the server. But there are no such parameters for the mdl
        # server, so we let the bound device choose the defaults...

        config["timeServerId"] = self.timeServerId

        # Would be nice to inject _connection_ config here from what the
        # server uses, i.e. domain, brokers and the broker type.
        # Since that is not easily available (except domain via
        # 'get_event_loop().topic'), we rely on the bound device extracting
        # the defaults from the environment.
        env = dict(os.environ)
        env["PYTHONPATH"] = self.pluginDirectory
        future = self._process_futures[deviceId]
        # the device ID is a parameter on the commandline only such that
        # one can identify via ps which process corresponds to which device
        process = await create_subprocess_exec(
            sys.executable, "-m", "karabo.bound.launcher",
            "run", self.boundNamespace, classId, deviceId,
            env=env, stdin=PIPE)
        # Pass as binary as expected by launcher
        # (XML has trouble with vector_string where str contains comma):
        process.stdin.write(encodeBinary(config))
        process.stdin.close()
        done, pending, error = await firstCompleted(
            ok=future, error=process.wait())
        if data := done.get("ok"):
            success, reason = data
            msg = deviceId
            if not success:
                msg = (f"could not instantiate device {deviceId}."
                       f" Reason: {reason}")
                return False, msg
            self.processes[deviceId] = process
            background(self.supervise(deviceId, process, done["ok"]))
            return True, deviceId
        else:
            return False, deviceId

    @slot
    def slotDeviceUp(self, instanceId, success, reason):
        """A Bound device will announce whether it is up and running

        here.
        success is a boolean and reason is a string"""
        is_ = "is" if success else "is not"
        msg = f"Bound device {instanceId} {is_} up : Reason {reason}"
        self.logger.info(msg)
        self._process_futures[instanceId] = (success, reason)

    async def slotKillServer(self, message=None):
        for device in self.processes:
            self._ss.emit("call", {device: ["slotKillDevice"]})
        try:
            await wait_for(
                gather(*(p.wait() for p in self.processes.values())),
                timeout=10)
        except TimeoutError:
            self.logger.exception("some processes did not finish in time")
            for p in self.processes.values():
                p.kill()
        self.processes.clear()
        await super().slotKillServer(message)

        return self.serverId

    slotKillServer = slot(slotKillServer, passMessage=True)


class DeviceServer(MiddleLayerDeviceServer, BoundDeviceServer):
    """A Python native Karabo Server

    It will load plugins and starts instances of the plugins.
    The plugins are loaded either from specific namespaces
    or from a specific plugin directory.
    """
    pass


if __name__ == '__main__':
    DeviceServer.main(sys.argv)
