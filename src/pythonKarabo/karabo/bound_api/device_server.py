#!/usr/bin/env python
# -*- coding: utf-8 -*-

__author__ = "Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ = "$Jul 26, 2012 10:06:25 AM$"

import os
import sys
import signal
import copy
import socket
from subprocess import Popen, TimeoutExpired
import threading

from karathon import (
    VECTOR_STRING_ELEMENT, INT32_ELEMENT, NODE_ELEMENT, OVERWRITE_ELEMENT,
    STRING_ELEMENT, LIST_ELEMENT,
    AccessLevel, JmsConnection,
    Hash, Logger, Schema, SignalSlotable, saveToFile, EventLoop
)

from karabo.common.states import State
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

            INT32_ELEMENT(expected).key("visibility")
            .displayedName("Visibility")
            .description("Configures who is allowed to see this server at all")
            .assignmentOptional().defaultValue(AccessLevel.OBSERVER)
            .options("0 1 2 3 4")
            .adminAccess()
            .reconfigurable()
            .commit(),

            NODE_ELEMENT(expected).key("connection")
            .displayedName("Connection")
            .description("The connection to the communication layer of the"
                         " distributed system")
            .appendParametersOf(JmsConnection)
            .expertAccess()
            .commit(),

            INT32_ELEMENT(expected).key("heartbeatInterval")
            .displayedName("Heartbeat interval")
            .description("The heartbeat interval")
            .assignmentOptional().defaultValue(10)
            .minInc(10)  # avoid too much traffic
            .adminAccess()
            .commit(),

            VECTOR_STRING_ELEMENT(expected).key("deviceClasses")
            .displayedName("Device Classes")
            .description("The device classes the server will manage")
            .assignmentOptional()
            .defaultValue(PythonDevice.getRegisteredClasses())
            .expertAccess()
            .commit(),

            LIST_ELEMENT(expected).key("autoStart")
            .displayedName("Auto start")
            .description("Auto starts selected devices")
            .appendNodesOfConfigurationBase(PythonDevice)
            .assignmentOptional().noDefaultValue()
            .commit(),

            STRING_ELEMENT(expected).key("pluginNamespace")
            .displayedName("Plugin Namespace")
            .description("Namespace to search for plugins")
            .assignmentOptional().defaultValue("karabo.bound_device")
            .expertAccess()
            .commit(),

            NODE_ELEMENT(expected).key("Logger")
            .description("Logging settings")
            .displayedName("Logger")
            .appendParametersOf(Logger)
            .commit(),

            OVERWRITE_ELEMENT(expected).key("Logger.file.filename")
            .setNewDefaultValue("device-server.log")
            .commit(),

            STRING_ELEMENT(expected).key("timeServerId")
            .description("The instance id uniquely identifies a TimeServer"
                         " instance in the distributed system")
            .displayedName("TimeServer ID")
            .assignmentOptional().defaultValue("")
            .commit(),

        )

    def setupFsm(self):
        """Description of DeviceServer state machine"""
        # **************************************************************
        # *                        Events                              *
        # **************************************************************

        KARABO_FSM_EVENT2(self, 'ErrorFoundEvent', 'errorFound')
        KARABO_FSM_EVENT0(self, 'ResetEvent', 'reset')

        # **************************************************************
        # *                        States                              *
        # **************************************************************

        KARABO_FSM_STATE_E(State.NORMAL, self.okStateOnEntry)
        KARABO_FSM_STATE(State.ERROR)

        # **************************************************************
        # *                    Transition Actions                      *
        # **************************************************************

        KARABO_FSM_ACTION2('ErrorFoundAction', self.errorFoundAction, str, str)
        KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)

        deviceServerMachineSTT = [
            (State.NORMAL, 'ErrorFoundEvent', State.ERROR,
             'ErrorFoundAction', 'none'),
            (State.ERROR, 'ResetEvent', State.NORMAL, 'none', 'none')]

        KARABO_FSM_STATE_MACHINE('DeviceServerMachine', deviceServerMachineSTT,
                                 State.NORMAL)

        return KARABO_FSM_CREATE_MACHINE('DeviceServerMachine')

    def signal_handler(self, signum, frame):
        if signum == signal.SIGINT:
            print('INTERRUPT : You pressed Ctrl-C!')
        else:
            print('INTERRUPT : You terminated me!')
        if self.ss is not None:
            # Better do not go via self.ss.call("", "slotKillServer"),
            # otherwise it will run later in another thread.
            self.slotKillServer()
        else:
            self.stopDeviceServer()

    def __init__(self, config):
        """Constructor"""
        if config is None:
            raise ValueError(
                "Input configuration for constructor should be Hash, not None")
        super(DeviceServer, self).__init__()
        # describe FSM
        self.processEventLock = threading.RLock()
        self.fsm = self.setupFsm()

        self.ss = self.log = None
        self.availableDevices = dict()
        self.deviceInstanceMap = dict()
        self.deviceInstanceMapLock = threading.RLock()
        if config.get('hostName') is not None:
            self.hostname = config['hostName']
        else:
            self.hostname = socket.gethostname().partition('.')[0]
        self.autoStart = config.get("autoStart")
        self.deviceClasses = config.get("deviceClasses")
        self.timeServerId = config.get("timeServerId")

        if 'serverId' in config:
            self.serverid = config['serverId']
        else:
            self.serverid = self._generateDefaultServerId()

        # What visibility this server should have
        self.visibility = config.get("visibility")

        self.connectionParameters = copy.copy(config['connection'])
        self.pid = os.getpid()
        self.seqnum = 0

        info = Hash("type", "server")
        info["serverId"] = self.serverid
        info["version"] = self.__class__.__version__
        info["host"] = self.hostname
        info["visibility"] = self.visibility
        info["lang"] = "bound"
        devicesInfo, scanLogs = self.scanPlugins(config["pluginNamespace"])
        info.merge(devicesInfo)

        signal.signal(signal.SIGINT, self.signal_handler)
        signal.signal(signal.SIGTERM, self.signal_handler)

        # Be aware: If signal handling is triggered before __init__ has
        # finished, self.ss might be set to None, causing problems below,
        # e.g. "'NoneType' object has no attribute 'getConnection'".
        # In worst case, SignalSlotable is constructed, but not yet assigned
        # to self.ss when the signal handler wants to reset it to None.
        # But the process stops nevertheless due to the EventLoop.stop().

        self.ss = SignalSlotable(self.serverid, "JmsConnection",
                                 self.connectionParameters,
                                 config["heartbeatInterval"], info)

        # Now we can start the logging system (needs self.ss.getTopic())
        self.loadLogger(config)
        self.log = Logger.getCategory(self.serverid)

        # Register before self.ss.start(), i.e before sending instanceNew:
        self._registerAndConnectSignalsAndSlots()

        # Start SignalSlotable object - multithreading begins
        self.ss.start()

        # Now we can log the postponed logging messages - could have been
        # done directly after assigning self.log, but prefer to do after
        # instanceNew triggered by self.ss.start():
        for level, message in scanLogs:
            getattr(self.log, level)(message)

        msg = "DeviceServer starts on host '{0.hostname}' " \
              "with pid {0.pid}, broker: {1}"
        self.log.INFO(msg.format(self,
                                 self.ss.getConnection().getBrokerUrl()))

        self.fsm.start()

    def _generateDefaultServerId(self):
        return self.hostname + "_Server_" + str(os.getpid())

    def loadLogger(self, config):
        if not config.has("Logger.network.topic"):
            # If not specified, use the local topic for log messages
            config.set("Logger.network.topic", self.ss.getTopic())
        path = os.path.join(os.environ['KARABO'], "var", "log", self.serverid)
        if not os.path.isdir(path):
            os.makedirs(path)
        path = os.path.join(path, 'device-server.log')
        config.set('Logger.file.filename', path)
        Logger.configure(config["Logger"])
        Logger.useOstream()
        Logger.useFile()
        Logger.useNetwork()

    def _registerAndConnectSignalsAndSlots(self):
        self.ss.registerSlot(self.slotStartDevice)
        self.ss.registerSlot(self.slotKillServer)
        self.ss.registerSlot(self.slotDeviceGone)
        self.ss.registerSlot(self.slotGetClassSchema)
        self.ss.registerSlot(self.slotLoggerPriority)

    def onStateUpdate(self, currentState):
        """This function is DEPRECATED and will be removed."""

    def okStateOnEntry(self):
        self.doAutoStart()

    def scanPlugins(self, pluginNamespace):
        """Scan for available device classes

        Returns Hash with keys "deviceClasses" and "visibilities" to be merged
        into instance info and a list of log messages to be send.
        Inside the list there are tuples of two string: log level (e.g. "INFO")
        and message.
        Also fills self.availableDevices dictionary.
        """
        loaderCfg = Hash("pluginNamespace", pluginNamespace)
        loader = PluginLoader.create("PythonPluginLoader", loaderCfg)
        entrypoints = loader.update()

        logs = []
        for ep in entrypoints:
            if ep.name in self.deviceClasses or not self.deviceClasses:
                try:
                    deviceClass = ep.load()
                except ImportError as e:
                    logs.append(("WARN",
                                 "scanPlugins: Cannot import module "
                                 "{} -- {}".format(ep.name, e)))
                    continue

                classid = deviceClass.__classid__
                try:
                    schema = Configurator(PythonDevice).getSchema(classid)
                    self.availableDevices[classid] = {"module": ep.name,
                                                      "schema": schema}
                    logs.append(("INFO",
                                 'Successfully loaded plugin: "{}:{}"'
                                 .format(ep.module_name, ep.name)))
                except (RuntimeError, AttributeError) as e:
                    m = "Failure while building schema for class {}, base " \
                        "class {} and bases {} : {}"\
                        .format(classid, deviceClass.__base_classid__,
                                deviceClass.__bases_classid__, e)
                    logs.append(("ERROR", m))

        instInfo = Hash("deviceClasses",
                        [classid for classid in self.availableDevices.keys()],
                        "visibilities",
                        [d['schema'].getDefaultValue("visibility")
                         for d in self.availableDevices.values()])
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
        localSigSlot = self.ss
        self.ss = None
        refCount = sys.getrefcount(localSigSlot)
        if refCount > 2:
            # count of 2: localSigSlot and the one internal to getrefcount
            print("Forced call to slotInstanceGone due to refcount", refCount)
            localSigSlot.call("*", "slotInstanceGone",
                              self.serverid, localSigSlot.getInstanceInfo())
        # HACK end

        EventLoop.stop()

    def errorFoundAction(self, m1, m2):
        self.log.ERROR("{} -- {}".format(m1, m2))

    def slotStartDevice(self, configuration):
        self.instantiateDevice(configuration)

    def instantiateDevice(self, input_config):
        classid = input_config['classId']

        # Get configuration
        config = copy.copy(input_config['configuration'])

        # Inject serverId
        config['_serverId_'] = self.serverid

        # Inject deviceId
        if ('deviceId' not in input_config
                or len(input_config['deviceId']) == 0):
            config['_deviceId_'] = self._generateDefaultDeviceInstanceId(
                classid)
        else:
            config['_deviceId_'] = input_config['deviceId']

        self.log.INFO("Trying to start a '{}' with device id '{}'"
                      "...".format(classid, config['_deviceId_']))
        self.log.DEBUG(
            "with the following configuration:\n{}".format(input_config))

        # Add connection type and parameters used by device server for
        # connecting to broker
        config['_connection_'] = self.connectionParameters

        # Inject HostName
        config['hostName'] = self.hostname

        # If not explicitely specified, let device inherit logger priority
        if not config.has("Logger.priority"):
            config["Logger.priority"] = Logger.getPriority()

        # Add time server ID configured for device server
        config['timeServerId'] = self.timeServerId

        # Create temporary instance to check the configuration parameters
        # are valid
        try:
            if "_deviceId_" in config:
                deviceid = config["_deviceId_"]
            else:
                msg = "Access to {}._deviceId_ failed".format(classid)
                raise RuntimeError(msg)

            modname = self.availableDevices[classid]["module"]

            # Create unique filename in /tmp - without '/' from deviceid...
            fn_tmpl = "/tmp/{mod}.{cls}.{dev}.configuration_{pid}_{num}.xml"
            filename_data = {'mod': modname, 'cls': classid, 'pid': self.pid,
                             'dev': deviceid.replace(os.path.sep, '_')}
            while True:
                filename = fn_tmpl.format(num=self.seqnum, **filename_data)
                if os.path.isfile(filename):
                    self.seqnum += 1
                else:
                    break

            saveToFile(config, filename, Hash("format.Xml.indentation", 2))
            params = [modname, classid, filename]

            with self.deviceInstanceMapLock:
                if deviceid in self.deviceInstanceMap:
                    # Already there! Check whether previous process is alive:
                    prevLauncher = self.deviceInstanceMap[deviceid]
                    if prevLauncher.child.poll() is None:
                        # Process still up. Check Karabo communication by ping:
                        request = self.ss.request(deviceid, "slotPing",
                                                  deviceid, 1, False)
                        try:
                            # Badly blocking here: We lack AsyncReply in bound!
                            request.waitForReply(2000)  # in milliseconds
                        except RuntimeError as innerE:
                            if "Reply timed out" in str(innerE):
                                # Indeed dead Karabo-wise:
                                self.log.WARN("Kill previous incarnation of "
                                              f"'{deviceid}' since reply to "
                                              "ping timed out.")
                                prevLauncher.kill()
                            else:
                                self.log.ERROR("Should never come here! "
                                               "Unexpected exception when "
                                               "trying to communicate with "
                                               f"'{deviceid}': {innerE}")
                                # Let outer 'try:' treat this:
                                raise innerE
                        else:
                            # Technically, it could be alive on another server,
                            # but who cares about that detail...
                            self.ss.reply(False, f"{deviceid} already "
                                          "instantiated and alive")
                            return
                launcher = Launcher(params)
                launcher.start()
                self.deviceInstanceMap[deviceid] = launcher

            self.ss.reply(True, deviceid)
        except Exception as e:
            self.log.WARN("Device '{}' could not be started because:"
                          " {}".format(classid, e))
            self.ss.reply(False, "Device '{}' could not be started because:"
                                 " {}".format(classid, e))

    def noStateTransition(self):
        self.log.WARN("DeviceServer \"{}\" does not allow the transition for"
                      " this event.".format(self.serverid))

    def slotKillServer(self):
        if self.log:
            self.log.INFO("Received kill signal")
        else:  # might get killed by signal handler before setting up logging
            print("Received kill signal")
        with self.deviceInstanceMapLock:
            # Loop twice: First to quickly tell all devices to go down and then
            #             to wait until they are indeed down (or need killing)
            for deviceid in self.deviceInstanceMap:
                self.ss.call(deviceid, "slotKillDevice")
            for deviceid, launcher in self.deviceInstanceMap.items():
                if launcher:
                    try:
                        launcher.join()
                    except TimeoutExpired:
                        self.log.WARN("Timeout on server shutdown while"
                                      " stopping the process for '{}'"
                                      "... SIGKILL".format(deviceid))
                        launcher.kill()
            self.deviceInstanceMap = {}
        try:
            self.ss.reply(self.serverid)
        except Exception as e:
            if self.log:  # see above
                msg = ("Did not notify distributed system of server shutdown:"
                       "\n {}").format(e)
                self.log.ERROR(msg)
        finally:
            self.stopDeviceServer()

    def slotDeviceGone(self, instanceId):
        # Would prefer a self.log.FRAMEWORK_INFO as in C++ instead of
        # self.log.DEBUG:
        self.log.DEBUG("Device '{0}' notifies '{1.serverid}' about its future"
                       " death.".format(instanceId, self))
        with self.deviceInstanceMapLock:
            if instanceId in self.deviceInstanceMap:
                launcher = self.deviceInstanceMap[instanceId]
                if launcher:
                    try:
                        launcher.join()
                    except TimeoutExpired:
                        self.log.WARN("Timeout on device shutdown while"
                                      " stopping the process for '{}'"
                                      "... SIGKILL".format(instanceId))
                        launcher.kill()
                del self.deviceInstanceMap[instanceId]
            self.log.INFO("Device '{}' removed from server."
                          .format(instanceId))

    def slotGetClassSchema(self, classid):
        try:
            schema = Configurator(PythonDevice).getSchema(classid)
            self.ss.reply(schema, classid, self.serverid)
        except AttributeError as e:
            self.log.WARN("Replied empty schema due to : {}".format(str(e)))
            self.ss.reply(Schema(), classid, self.serverid)

    def slotLoggerPriority(self, newprio):
        # In contrast to C++, the new priority will not be "forwarded" to
        # existing devices. Python devices have their own slotLoggerPriority
        # which allows priority setting device by device.
        # But future device instantiations should inherit their priority from
        # the current value of the server:
        oldprio = Logger.getPriority()
        Logger.setPriority(newprio)
        self.log.INFO(
            "Logger Priority changed : {} ==> {}".format(oldprio, newprio))

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
    """ Launches a PythonDevice in its own interpreter."""

    def __init__(self, params):
        self.args = params

    def start(self):
        args = [sys.executable, "-c"]
        args.append("from karabo.bound import launchPythonDevice;"
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


def main(args=None):
    try:
        # Change directory to $KARABO/var/data
        os.chdir(os.path.join(os.environ['KARABO'], 'var', 'data'))
    except KeyError:
        print("ERROR: $KARABO is not defined. Make sure you have sourced the "
              "'activate' script.")
        return

    args = args or sys.argv
    # Load plugins already here to make them available for -h option
    PluginLoader.create("PythonPluginLoader", Hash()).update()
    t = threading.Thread(target=EventLoop.work)
    t.start()
    try:
        server = Runner(DeviceServer).instantiate(args)
        if not server:
            EventLoop.stop()
            # Likely started with -h option: Avoid print after finally.
            return
    except Exception:
        EventLoop.stop()
        raise
    finally:
        del server  # increase chance that no log appears after print below
        t.join()
    print(os.path.basename(args[0]), "has exited!\n")


if __name__ == '__main__':
    main()
