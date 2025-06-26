#!/usr/bin/env python
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
# -*- coding: utf-8 -*-

__author__ = "Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ = "$Jul 26, 2012 10:06:25 AM$"

import copy
import os
import signal
import socket
import sys
import threading
import time
import traceback
from itertools import chain
from subprocess import Popen, TimeoutExpired

from karabind import (
    INT32_ELEMENT, NODE_ELEMENT, OVERWRITE_ELEMENT, STRING_ELEMENT,
    VECTOR_STRING_ELEMENT, Broker, EventLoop, Hash, Logger, Schema,
    SignalSlotable, Unit, Validator, generateAutoStartHash, jsonToHash,
    saveToFile)
from karabo.common.api import KARABO_LOGGER_CONTENT_DEFAULT, ServerFlags

from .configurator import Configurator
from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from .device import PythonDevice
from .plugin_loader import DEFAULT_NAMESPACE, PluginLoader
from .runner import Runner


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("DeviceServer", "1.0")
class DeviceServer:
    """Device server serves as a launcher of python devices.

    It scans 'plugins' directory for new plugins (python scripts) available and
    communicates its findings to master device server.  It communicates XSD
    form of schema of user devices and starts such devices as separate process
    if user push "Initiate" button in GUI
    """
    instanceCountLock = threading.Lock()
    instanceCountPerDeviceServer = dict()

    @staticmethod
    def expectedParameters(expected):
        (
            STRING_ELEMENT(expected).key("serverId")
            .displayedName("Server ID")
            .description("The device-server instance id uniquely identifies a"
                         " device-server instance in the distributed system")
            .assignmentOptional().noDefaultValue()
            .commit(),

            STRING_ELEMENT(expected).key("hostName")
            .displayedName("Forced Hostname")
            .description(
                "The hostname can be optionally forced to a specific string. "
                "The host's definition will be used if not specified.")
            .assignmentOptional().noDefaultValue()
            .expertAccess()
            .init()
            .commit(),

            INT32_ELEMENT(expected).key("heartbeatInterval")
            .displayedName("Heartbeat interval")
            .description("The heartbeat interval")
            .assignmentOptional().defaultValue(20)
            .minInc(10)  # avoid too much traffic
            .expertAccess()
            .commit(),

            VECTOR_STRING_ELEMENT(expected).key("deviceClasses")
            .displayedName("Device Classes")
            .description("The device classes the server will manage")
            .assignmentOptional()
            .defaultValue([])
            .expertAccess()
            .commit(),

            STRING_ELEMENT(expected).key("init")
            .displayedName("Auto start")
            .description("Auto starts selected devices")
            .assignmentOptional().defaultValue("")
            .commit(),

            STRING_ELEMENT(expected).key("pluginNamespace")
            .displayedName("Plugin Namespace")
            .description("Namespace to search for plugins")
            .assignmentOptional().defaultValue(DEFAULT_NAMESPACE)
            .expertAccess()
            .commit(),

            VECTOR_STRING_ELEMENT(expected).key("serverFlags")
            .displayedName("Server Flags")
            .description("ServerFlags describing the device server, "
                         "the values must correspond to the enum ServerFlags")
            .assignmentOptional()
            .defaultValue([])
            .expertAccess()
            .commit(),

            NODE_ELEMENT(expected).key("log")
            .description("Logging settings")
            .displayedName("Logger")
            .appendParametersOf(Logger)
            .commit(),

            OVERWRITE_ELEMENT(expected).key("log.file.filename")
            # Will be assembled programmatically from environment and serverId
            .setNewAssignmentInternal()
            .commit(),

            STRING_ELEMENT(expected).key("timeServerId")
            .description("The instance id uniquely identifies a TimeServer"
                         " instance in the distributed system")
            .displayedName("TimeServer ID")
            .assignmentOptional().defaultValue("")
            .commit(),

            INT32_ELEMENT(expected).key("instantiationTimeout")
            .displayedName("Instantiation Timeout")
            .description("How long to wait for device coming up before "
                         "slotStartDevice fails")
            .unit(Unit.SECOND)
            .assignmentOptional().defaultValue(10)
            .commit(),
        )

    def signal_handler(self, signum, frame):
        if signum == signal.SIGINT:
            print('INTERRUPT : You pressed Ctrl-C!')
        else:
            print('INTERRUPT : You terminated me!')
        if self._sigslot is not None:
            # Better do not go via self._sigslot.call("", "slotKillServer"),
            # otherwise it will run later in another thread.
            self.slotKillServer()
        else:
            self.stopDeviceServer()

    def __init__(self, config):
        """Constructor"""
        if config is None:
            raise ValueError(
                "Input configuration for constructor should be Hash, not None")
        super().__init__()
        self._sigslot = self.logger = None
        self.availableDevices = dict()
        self.deviceInstanceMap = dict()
        self._startingDevices = dict()  # needs protection by lock below
        self.deviceInstanceMapLock = threading.RLock()
        self.instantiationTimeout = config["instantiationTimeout"]
        if config.get('hostName') is not None:
            self.hostname = config['hostName']
        else:
            self.hostname = socket.gethostname().partition('.')[0]
        # Check if obsolete syntax still used...
        if config.has('autoStart'):
            raise KeyError(
                "'autoStart' syntax not supported anymore, use 'init'")
        self.autoStart = None
        if config.get("init") != "":
            cfg = jsonToHash(config.get("init"))
            asv = generateAutoStartHash(cfg)
            self.autoStart = asv['autoStart']

        self.deviceClasses = config.get("deviceClasses")
        self.pluginNamespace = config.get("pluginNamespace")
        self.timeServerId = config.get("timeServerId")

        if 'serverId' in config:
            self.serverId = config['serverId']
        else:
            self.serverId = self._generateDefaultServerId()

        self.connectionParameters = Hash(Broker.brokerTypeFromEnv(),
                                         Hash("instanceId", self.serverId))
        self.loggerParameters = None  # assemble in loadLogger
        self.pid = os.getpid()
        self.seqnum = 0

        # Start the logging system before the scanning of plugins, so any
        # error during plugin loading can be logged.
        self.loadLogger(config)
        self.logger = Logger.getLogger(self.serverId)

        info = Hash("type", "server")
        info["serverId"] = self.serverId
        info["version"] = self.__class__.__version__
        info["host"] = self.hostname
        info["lang"] = "bound"
        info["log"] = config.get("log.level")

        self.serverFlags = config.get("serverFlags")
        serverFlags = 0
        for flag in self.serverFlags:
            if flag in ServerFlags.__members__:
                serverFlags |= ServerFlags[flag]
            else:
                raise NotImplementedError(
                    f"Provided serverFlag is not supported: {flag}")
        info["serverFlags"] = serverFlags

        devicesInfo, scanLogs = self.scanPlugins(self.pluginNamespace)
        info.merge(devicesInfo)

        signal.signal(signal.SIGINT, self.signal_handler)
        signal.signal(signal.SIGTERM, self.signal_handler)

        # Be aware: If signal handling is triggered before __init__ has
        # finished, self._sigslot might be set to None, causing problems below,
        # e.g. "'NoneType' object has no attribute 'getConnection'".
        # In worst case, SignalSlotable is constructed, but not yet assigned
        # to self._sigslot when the signal handler wants to reset it to None.
        # But the process stops nevertheless due to the EventLoop.stop().

        self._sigslot = SignalSlotable(
            self.serverId, self.connectionParameters,
            config["heartbeatInterval"], info)

        # Register before sigslot.start(), i.e before sending instanceNew:
        self._registerAndConnectSignalsAndSlots()

        # Start SignalSlotable object - multithreading begins
        self._sigslot.start()

        # Now we can log the postponed logging messages - could have been
        # done directly after assigning self.logger, but prefer to do after
        # instanceNew triggered by self._sigslot.start():
        for level, message in scanLogs:
            getattr(self.logger, level)(message)

        msg = "DeviceServer starts on host '{0.hostname}' " \
              "with pid {0.pid}, broker: {1}"
        self.logger.info(msg.format(
            self, self._sigslot.getConnection().getBrokerUrl()))

        self.doAutoStart()

    def _generateDefaultServerId(self):
        return self.hostname + "_Server_" + str(os.getpid())

    def loadLogger(self, inputCfg):
        self.loggerParameters = copy.copy(inputCfg["log"])
        # The file logger filename is completely specified here.
        path = os.path.join(os.environ['KARABO'], "var", "log", self.serverId)
        if not os.path.isdir(path):
            os.makedirs(path)
        path = os.path.join(path, 'device-server.log')
        self.loggerParameters.set('file.filename', path)

        Logger.configure(self.loggerParameters)
        Logger.useConsole()
        Logger.useFile()
        Logger.useCache()

    def _registerAndConnectSignalsAndSlots(self):
        self._sigslot.registerSlot(self.slotStartDevice)
        self._sigslot.registerSlot(self.slotKillServer)
        self._sigslot.registerSlot(self.slotDeviceUp)
        self._sigslot.registerSlot(self.slotDeviceGone)
        self._sigslot.registerSlot(self.slotGetClassSchema)
        self._sigslot.registerSlot(self.slotLoggerLevel)
        self._sigslot.registerSlot(self.slotLoggerContent)

    def scanPlugins(self, pluginNamespace):
        """Scan for available device classes

        Returns Hash with keys "deviceClasses" and "visibilities" to be merged
        into instance info and a list of log messages to be send.
        Inside the list there are tuples of two string: log level (e.g. "info")
        and message.
        Also fills self.availableDevices dictionary.
        """
        loaderCfg = Hash("pluginNamespace", pluginNamespace)
        loader = PluginLoader.create("PythonPluginLoader", loaderCfg)
        entrypoints = loader.update()
        logs = [("error",
                 f"scanPlugins: Cannot load device plugin {name} -- {exc}")
                for name, exc in loader.plugin_errors.items()]
        for ep in entrypoints:
            if not self.deviceClasses or ep.name in self.deviceClasses:
                # All entry points have been verified already
                deviceClass = ep.load()
                classid = deviceClass.__classid__
                try:
                    schema = Configurator(PythonDevice).getSchema(classid)
                    self.availableDevices[classid] = {"module": ep.name,
                                                      "schema": schema}
                    logs.append(("info",
                                 'Successfully loaded plugin: "{}"'.format(
                                     ep.name)))
                # A generic handler is used here due to the different kind
                # of exceptions that may be raised when obtained the schema
                # for the just loaded plugin.
                except Exception as e:
                    m = "Failure while building schema for class {}, base " \
                        "class {} and bases {} : {}" \
                        .format(classid, deviceClass.__base_classid__,
                                deviceClass.__bases_classid__, repr(e))
                    # repr(e) also includes type
                    logs.append(("error", m))

        instInfo = Hash("deviceClasses",
                        [classid for classid in self.availableDevices.keys()])

        return instInfo, logs

    def doAutoStart(self):
        if self.autoStart is None:
            return
        for entry in self.autoStart:
            # entry is a Hash with a single key which is the classid
            classId = entry.getKeys()[0]
            if classId in self.availableDevices:
                config = entry.get(classId)
                hsh = Hash("classId", classId,
                           "configuration", config)
                if 'deviceId' in config:
                    hsh.set('deviceId', config.get('deviceId'))
                self.instantiateDevice(hsh)

    def stopDeviceServer(self):
        # HACK: Sometimes the C++ destructor of SignalSlotable is not called
        #       and then instanceGone is not sent. This can happen when many
        #       variables are still referring to the SignalSlotable here.
        #       This has been seen when stopped early in the initialisation
        #       procedure, leading to a very high (>27000) refcount.
        #       If we are the last ones, it is guaranteed that the destructor
        #       is called.
        #
        # Take over SignalSlotable to avoid new references in other threads:
        localSigSlot = self._sigslot
        self._sigslot = None
        refCount = sys.getrefcount(localSigSlot)
        if refCount > 2:
            # count of 2: localSigSlot and the one internal to getrefcount
            print("Forced call to slotInstanceGone due to refcount", refCount)
            localSigSlot.call("*", "slotInstanceGone",
                              self.serverId, localSigSlot.getInstanceInfo())
        # HACK end

        EventLoop.stop()

    def slotStartDevice(self, configuration):
        self.instantiateDevice(configuration)

    def instantiateDevice(self, input_config):
        classId = input_config['classId']

        # Get configuration
        config = copy.copy(input_config['configuration'])

        # Inject serverId
        config['serverId'] = self.serverId

        # Inject deviceId
        config['deviceId'] = input_config.get("deviceId", default="")
        if not config['deviceId']:
            config.set('deviceId',
                       self._generateDefaultDeviceInstanceId(classId))

        self.logger.info("Trying to start a '{}' with device id '{}'"
                         "...".format(classId, config['deviceId']))
        self.logger.debug(
            f"with the following configuration:\n{input_config}")

        # Inject HostName
        config['hostName'] = self.hostname

        # If not explicitely specified, let device inherit logger level
        if not config.has("log.level"):
            config["log.level"] = self.loggerParameters["level"]

        # Before starting device process, validate config
        schema = Configurator(PythonDevice).getSchema(classId)
        validator = Validator()
        (ok, msg, _) = validator.validate(schema, config)
        if not ok:
            msg = msg.strip()  # cut-off trailing newline...
            self.logger.warning(
                f"Failed to start '{config['deviceId']}': {msg}")
            self._sigslot.reply(ok, msg)
            return

        # Now few config manipulation intentionally AFTER 'validate':
        # Add connection type and parameters used by device server for
        # connecting to broker.
        config['_connection_'] = self.connectionParameters

        # Add temporary namespace variable
        config['_pluginNamespace_'] = self.pluginNamespace

        # Add time server ID configured for device server.
        config['timeServerId'] = self.timeServerId

        # Also add config for Logger appenders
        for appender in ["console", "file", "cache"]:
            config["_logger_." + appender] = self.loggerParameters[appender]

        reply = self._sigslot.createAsyncReply()

        try:
            self._launchDevice(config, classId, reply)
        except Exception as e:
            msg = f"Device of class '{classId}' could not be started: {e}"
            details = traceback.format_exc()
            self.logger.warning(f"{msg}:\nFailure details:\n{details}")
            # Cannot call AsyncReply object directly in slot, so post:
            EventLoop.post(lambda: reply.error(msg, details))

    def _launchDevice(self, config, classId, reply):

        deviceid = config["deviceId"]  # exists, see instantiateDevice

        modname = self.availableDevices[classId]["module"]

        # Create unique filename in /tmp - without '/' from deviceid...
        # .bin indicates binary format:
        # XML has trouble with VECTOR_STRING where string contains comma
        while True:
            filename = (f"/tmp/{modname}.{classId}."
                        f"{deviceid.replace(os.path.sep, '_')}"
                        f".configuration_{self.pid}_{self.seqnum}.bin")
            if os.path.isfile(filename):
                self.seqnum += 1
            else:
                break

        saveToFile(config, filename)
        params = [modname, classId, filename]

        with self.deviceInstanceMapLock:
            if deviceid in self.deviceInstanceMap:
                # Already there! Check whether previous process is alive:
                prevLauncher = self.deviceInstanceMap[deviceid]
                if prevLauncher.child.poll() is None:
                    # Process still up. Check Karabo communication by ping:
                    request = self._sigslot.request(deviceid, "slotPing", 1)
                    try:
                        # Too lazy to use async techniques for this corner case
                        request.waitForReply(3000)  # in milliseconds
                    except TimeoutError:
                        # Indeed dead Karabo-wise:
                        self.logger.warning("Kill previous incarnation of "
                                            f"'{deviceid}' since reply to "
                                            "ping timed out.")
                        prevLauncher.kill()
                    except Exception as e:
                        # Unexpected exception - give up
                        raise e
                    else:
                        # Technically, it could be alive on another server,
                        # but who cares about that detail...
                        # Just raising RuntimeError is in principle also OK
                        # Cannot call AsyncReply() directly in slot, so post:
                        EventLoop.post(lambda: reply(False,
                                                     f"{deviceid} already "
                                                     "instantiated and alive"))
                        return

            launcher = Launcher(params)
            launcher.start()
            self.deviceInstanceMap[deviceid] = launcher
            # Keep track of being starting to reply when succeeded (or timeout)
            self._startingDevices[deviceid] = (reply, time.time())
            if len(self._startingDevices) == 1:  # otherwise already running
                EventLoop.post(self._checkStartingDevices, 1)

    def _checkStartingDevices(self):
        failed = []
        somethingLeft = False
        with self.deviceInstanceMapLock:
            for devId, (reply, startedAt) in self._startingDevices.items():
                if (time.time() - startedAt) > self.instantiationTimeout:
                    failed.append((devId, reply))
            for devId, _ in failed:
                del self._startingDevices[devId]
                # But don't touch self.deviceInstanceMap - device may be "late"
            if self._startingDevices:
                somethingLeft = True

        # Reply without lock since sending messages is blocking:
        for devId, reply in failed:
            msg = (f"Timeout of instantiation: {devId} did not confirm it is "
                   f"up within {self.instantiationTimeout} seconds")
            self.logger.warning(msg)
            reply(False, msg)

        if somethingLeft:
            EventLoop.post(self._checkStartingDevices, 0.5)

    def slotKillServer(self):
        if self.logger:
            self.logger.info("Received kill signal")
        else:  # might get killed by signal handler before setting up logging
            print("Received kill signal")
        with self.deviceInstanceMapLock:
            # Break "loop" to check for starting devices
            self._startingDevices.clear()
            # Loop twice: First to quickly tell all devices to go down and then
            #             to wait until they are indeed down (or need killing)
            for deviceid in self.deviceInstanceMap:
                self._sigslot.call(deviceid, "slotKillDevice")
            for deviceid, launcher in self.deviceInstanceMap.items():
                if launcher:
                    try:
                        launcher.join()
                    except TimeoutExpired:
                        self.logger.warning("Timeout on server shutdown while"
                                            " stopping the process for '{}'"
                                            "... SIGKILL".format(deviceid))
                        launcher.kill()
            self.deviceInstanceMap = {}
        try:
            self._sigslot.reply(self.serverId)
        except Exception as e:
            if self.logger:  # see above
                msg = ("Did not notify distributed system of server shutdown:"
                       "\n {}").format(e)
                self.logger.error(msg)
        finally:
            self.stopDeviceServer()

    def slotDeviceUp(self, instanceId, success, reason):
        "Slot for device to tell us whether it managed to get alive"
        awaited = None
        with self.deviceInstanceMapLock:
            awaited = self._startingDevices.pop(instanceId, None)
        if awaited:
            asyncReply = awaited[0]
            if success:
                asyncReply(success, instanceId)
            else:
                msg = (f"could not instantiate device {instanceId}."
                       f" Reason: {reason}")
                asyncReply(success, msg)
        else:
            self.logger.warning(f"Unexpected slotDeviceUp for {instanceId}")
            # No need to inform caller about failure via raising an exception

    def slotDeviceGone(self, instanceId):
        self.logger.info(
            f"Device '{instanceId}' notifies '{self.serverId}' about "
            "its future death.")
        with self.deviceInstanceMapLock:
            if instanceId in self.deviceInstanceMap:
                launcher = self.deviceInstanceMap[instanceId]
                if launcher:
                    try:
                        launcher.join()
                    except TimeoutExpired:
                        self.logger.warning("Timeout on device shutdown while"
                                            " stopping the process for '{}'"
                                            "... SIGKILL".format(instanceId))
                        launcher.kill()
                del self.deviceInstanceMap[instanceId]
            self.logger.info("Device '{}' removed from server."
                             .format(instanceId))

    def slotGetClassSchema(self, classId):
        try:
            schema = Configurator(PythonDevice).getSchema(classId)
            self._sigslot.reply(schema, classId, self.serverId)
        except AttributeError as e:
            self.logger.warning(f"Replied empty schema due to : {str(e)}")
            self._sigslot.reply(Schema(), classId, self.serverId)

    def slotLoggerLevel(self, newlevel):
        # In contrast to C++, the new level will not be "forwarded" to
        # existing devices. Python devices have their own slotLoggerLevel
        # which allows level setting device by device.
        # But future device instantiations should inherit their level from
        # the current value of the server:
        oldprio = Logger.getLevel()
        Logger.setLevel(newlevel)
        self.logger.info(
            f"log Level changed : {oldprio} ==> {newlevel}")
        # Also devices started in future get the new level by default:
        self.loggerParameters["level"] = newlevel
        # Merge the new log level into the instanceInfo
        self._sigslot.updateInstanceInfo(Hash("log", newlevel))

    def slotLoggerContent(self, info):
        """Slot call to receive logger content from the CacheLogger

        replies with a Hash containing a key, `serverId`
        and a `content` containing a vector of hashes formatted in the
        same way the broker based logging uses. For details:
        ``src/karabo/log/CacheLogger.cc``

        :param info: input Hash containing an optional `logs` integer
                     defining the maximum number of lines returned
        """
        # create a map where to store the devices' logs
        with self.deviceInstanceMapLock:
            log_map = dict.fromkeys(self.deviceInstanceMap)

        if not log_map:  # no devices => direct reply instead of an async one
            nMessages = info.get(
                "logs", default=KARABO_LOGGER_CONTENT_DEFAULT)
            content = Logger.getCachedContent(nMessages)
            self._sigslot.reply(
                Hash("serverId", self.serverId, "content", content))
            return

        replyLock = threading.Lock()

        areply = self._sigslot.createAsyncReply()
        # copy because the info object will be cleaned up after
        # this function has returned
        info = copy.copy(info)

        class Handler:
            def __init__(s, deviceId):
                s.deviceId = deviceId

            def on_reply(s, reply):
                # copy because the reply object will be
                # cleaned up after the handler has returned
                _copy = [copy.copy(entry) for entry in reply['content']]
                log_map[s.deviceId] = _copy
                s.done()

            def on_error(s, msg, details):
                msg = (f"Missing Logger Content from '{s.deviceId}' : "
                       f"{msg} - DETAILS {details}")
                self.logger.warning(msg)
                log_map.pop(s.deviceId, None)
                s.done()

            def done(s):
                # make sure that only one handler will call
                # `_replySlotLoggerContent`.
                with replyLock:
                    if all(log_map.values()):
                        self._replySlotLoggerContent(info, log_map, areply)

        for deviceId in log_map.keys():
            # request the content from the devices in the server.
            req = self._sigslot.request(deviceId, "slotLoggerContent", info)
            h = Handler(deviceId)
            req.receiveAsync(h.on_reply, h.on_error, timeoutMs=5000)

    def _replySlotLoggerContent(self, info, log_map, areply):
        nMessages = info.get("logs", default=KARABO_LOGGER_CONTENT_DEFAULT)
        content = list(log_map.values())
        # add the latest server logs.
        content.append(Logger.getCachedContent(nMessages))
        content = chain.from_iterable(content)
        # sort by timestamp and get the last "nMessages" entries
        content = sorted(content, key=lambda entry: entry['timestamp'])
        # reply
        areply(Hash("serverId", self.serverId,
                    "content", content[-1 * nMessages:]))

    def _generateDefaultDeviceInstanceId(self, devClassId: str) -> str:
        cls = self.__class__
        with cls.instanceCountLock:
            count = cls.instanceCountPerDeviceServer.setdefault(
                self.serverId, 0) + 1
            cls.instanceCountPerDeviceServer[self.serverId] = count
            tokens = self.serverId.split("_")
            if tokens[-1] == str(os.getpid()):
                instance_id = f"{tokens[0]}-{tokens[-2]}_{devClassId}_{count}"
            else:
                instance_id = f"{self.serverId}_{devClassId}_{count}"

            return instance_id


class Launcher:
    """ Launches a PythonDevice in its own interpreter."""

    def __init__(self, params):
        self.args = params

    def start(self):
        args = [sys.executable, "-c"]
        args.append("from karabo.bound.device import launchPythonDevice;"
                    "launchPythonDevice()")
        args.extend(self.args)

        self.child = Popen(args)

    # TODO This should be renamed, has nothing to do with threads
    def join(self):
        if self.child.poll() is None:
            self.child.wait(timeout=5)

    def kill(self):
        self.child.kill()
        self.child.wait()


def main(argv=None):
    try:
        # Change directory to $KARABO/var/data
        os.chdir(os.path.join(os.environ['KARABO'], 'var', 'data'))
    except KeyError:
        print("ERROR: $KARABO is not defined. Make sure you have sourced the "
              "'activate' script.")
        return 1

    argv = argv or sys.argv
    # Load plugins already here to make them available for -h option
    PluginLoader.create("PythonPluginLoader", Hash()).update()
    t = threading.Thread(target=EventLoop.work)
    t.start()
    server = None
    try:
        server = Runner(DeviceServer).instantiate(argv)
        if not server:
            EventLoop.stop()
            # We get here with -h option: Avoid print after finally.
            return 0
    except Exception:
        EventLoop.stop()
        traceback.print_exc()
        return 2
    finally:
        del server  # increase chance that no log appears after print below
        t.join()
    print(os.path.basename(argv[0]), "has exited!\n")
    return 0


if __name__ == '__main__':
    sys.exit(main())
