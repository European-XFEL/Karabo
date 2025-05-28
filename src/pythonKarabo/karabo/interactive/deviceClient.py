#!/usr/bin/python
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
#
# Author: <burkhard.heisen@xfel.eu>
#

import datetime
import re
import time

import IPython
import pytz
import tzlocal
from dateutil import parser

from karabo._version import version as karaboVersion
from karabo.bound import (
    DeviceClient as BoundDeviceClient, Hash, TextSerializerHash)

# ip = IPython.core.ipapi.get()
ip = IPython.get_ipython()

# Create one instance (global singleton) of a DeviceClient
cpp_client = None


# Welcome
print("\n#### Karabo Device-Client (version:", karaboVersion, ") ####")
print("To start you need a DeviceClient object, e.g. type:\n")
print("  d = DeviceClient()\n")
print("Using this object you can remote control Karabo devices.")
print("You may query servers and devices and set/get properties or execute "
      "commands on them.")
print("Hint: use the TAB key for auto-completion.")


def distributed_autocomplete(func):
    def wrapper(*args, **kwargs):
        try:
            func(*args, **kwargs)
        except BaseException:
            print("Distributed auto-completion failed")
    return wrapper


# Auto complete function for methods like: f( deviceId, attribute )
@distributed_autocomplete
def auto_complete_full(self, event):
    if (re.match(r'.*\(\s*$', event.line)
            or re.match(r'.*\(\s*\"\w+\",\s*$', event.line)):
        return ["\""]
    elif (re.match(r'.*\(\s*\"\w+$', event.line)
            or re.match(r'.*\(\s*\"\w+\",\s*\"\w+$', event.line)):
        # These REs match correctly but completion does not work
        return ["\""]
    elif (re.match(r'.*\(\s*\"$', event.line)):
        dev = cpp_client.getDevices()
        if len(dev) > 0:
            return dev
        else:
            return ["NO_INSTANCES_AVAILABLE"]
    elif (re.match(r'.*\(.+,\s*\"$', event.line)):
        r = re.compile(r'\"(.*?)\"')
        m = r.search(event.line)
        if m:
            deviceId = m.group(1)
            return cpp_client.getProperties(deviceId)
        else:
            return [""]
    else:
        return [""]


@distributed_autocomplete
def auto_complete_devid(self, event):
    if (re.match(r'.*\(\s*$', event.line)):
        return ["\""]
    elif (re.match(r'.*\(\s*\"\w+$', event.line)):
        # These REs match correctly but completion does not work
        return ["\""]
    elif (re.match(r'.*\(\s*\"$', event.line)):
        dev = cpp_client.getDevices()
        if len(dev) > 0:
            return dev
        else:
            return ["NO_INSTANCES_AVAILABLE"]
    else:
        return [""]


@distributed_autocomplete
def auto_complete_serverid(self, event):
    if (re.match(r'.*\(\s*$', event.line)):
        return ["\""]
    elif (re.match(r'.*\(\s*\"\w+$', event.line)):
        # These REs match correctly but completion does not work
        return ["\""]
    elif (re.match(r'.*\(\s*\"$', event.line)):
        dev = cpp_client.getServers()
        if len(dev) > 0:
            return dev
        else:
            return ["NO_SERVERS_AVAILABLE"]
    else:
        return [""]


@distributed_autocomplete
def auto_complete_set(self, event):
    if (re.match(r'.*\(\s*$', event.line)
            or re.match(r'.*\(\s*\"\w+\",\s*$', event.line)):
        return ["\""]
    elif (re.match(r'.*\(\s*\"\w+$', event.line)
            or re.match(r'.*\(\s*\"\w+\",\s*\"\w+$', event.line)):
        # These REs match correctly but completion does not work
        return ["\""]
    elif (re.match(r'.*\(\s*\"$', event.line)):
        dev = cpp_client.getDevices()
        if len(dev) > 0:
            return dev
        else:
            return ["NO_INSTANCES_AVAILABLE"]
    elif (re.match(r'.*\(.+,\s*\"$', event.line)):
        r = re.compile(r'\"(.*?)\"')
        m = r.search(event.line)
        if m:
            deviceId = m.group(1)
            return cpp_client.getCurrentlySettableProperties(deviceId)
        else:
            return [""]
    else:
        return [""]


@distributed_autocomplete
def auto_complete_execute(self, event):
    if (re.match(r'.*\(\s*$', event.line)
            or re.match(r'.*\(\s*\"\w+\",\s*$', event.line)):
        return ["\""]
    elif (re.match(r'.*\(\s*\"\w+$', event.line)
            or re.match(r'.*\(\s*\"\w+\",\s*\"\w+$', event.line)):
        # These REs match correctly but completion does not work
        return ["\""]
    elif (re.match(r'.*\(\s*\"$', event.line)):
        dev = cpp_client.getDevices()
        if len(dev) > 0:
            return dev
        else:
            return ["NO_INSTANCES_AVAILABLE"]
    elif (re.match(r'.*\(.+,\s*\"$', event.line)):
        r = re.compile(r'\"(.*?)\"')
        m = r.search(event.line)
        if m:
            deviceId = m.group(1)
            return cpp_client.getCurrentlyExecutableCommands(deviceId)
        else:
            return [""]
    else:
        return [""]


@distributed_autocomplete
def auto_complete_instantiate(self, event):
    if (re.match(r'.*\(\s*$', event.line)
            or re.match(r'.*\(\s*\"\w+\",\s*$', event.line)):
        return ["\""]
    elif (re.match(r'.*\(\s*\"\w+$', event.line)
            or re.match(r'.*\(\s*\"\w+\",\s*\"\w+$', event.line)):
        # These REs match correctly but completion does not work
        return ["\""]
    elif (re.match(r'.*\(\s*\"$', event.line)):
        dev = cpp_client.getServers()
        if len(dev) > 0:
            return dev
        else:
            return ["NO_INSTANCES_AVAILABLE"]
    elif (re.match(r'.*\(.+\,\s*$', event.line)):
        return ["\""]
    elif (re.match(r'.*\(.+,\s*\"$', event.line)):
        r = re.compile(r'\"(.*?)\"')
        m = r.search(event.line)
        if m:
            serverId = m.group(1)
            return cpp_client.getClasses(serverId)
        else:
            return [""]
    elif (re.match(r'.*\(.+,.+,\s*$', event.line)
            or re.match(r'.*\(.+\,.+\,\s*\".*$', event.line)):
        return ["\""]
    else:
        return [""]


# Register hooks in IPython
if ip is not None:
    ip.set_hook('complete_command', auto_complete_full,
                re_key='.*getFromPast')
    ip.set_hook('complete_command', auto_complete_full,
                re_key='.*getPropertyHistory')
    ip.set_hook('complete_command', auto_complete_full,
                re_key='.*registerPropertyMonitor')
    ip.set_hook('complete_command', auto_complete_full,
                re_key='.*unregisterPropertyMonitor')
    ip.set_hook('complete_command', auto_complete_full,
                re_key='.*help')

    ip.set_hook('complete_command', auto_complete_serverid,
                re_key='.*shutdownServer')
    ip.set_hook('complete_command', auto_complete_serverid,
                re_key='.*getClasses')
    ip.set_hook('complete_command', auto_complete_serverid,
                re_key='.*getDevices')

    ip.set_hook('complete_command', auto_complete_devid,
                re_key='.*shutdownDevice')
    ip.set_hook('complete_command', auto_complete_devid,
                re_key='.*setHash')
    ip.set_hook('complete_command', auto_complete_devid,
                re_key='.*setPrio')
    ip.set_hook('complete_command', auto_complete_devid,
                re_key='.*lock')
    ip.set_hook('complete_command', auto_complete_devid,
                re_key='.*registerDeviceMonitor')
    ip.set_hook('complete_command', auto_complete_devid,
                re_key='.*unregisterDeviceMonitor')
    ip.set_hook('complete_command', auto_complete_devid,
                re_key='.*getDeviceHistory')
    ip.set_hook('complete_command', auto_complete_devid,
                re_key='.*getConfigurationFromPast')

    ip.set_hook('complete_command', auto_complete_set,
                re_key='.*setNoWait')
    ip.set_hook('complete_command', auto_complete_execute,
                re_key='.*execute')

    ip.set_hook('complete_command', auto_complete_instantiate,
                re_key='.*instantiate')
    ip.set_hook('complete_command', auto_complete_instantiate,
                re_key='.*getClassSchema')

    # Short keys go to end
    ip.set_hook('complete_command', auto_complete_full,
                re_key='.*get')
    ip.set_hook('complete_command', auto_complete_set,
                re_key='.*set')


class DeviceClient:
    """DeviceClient to remotely control Karabo

    The DeviceClient allows to remotely control a Karabo installation.
    A Karabo installation comprises all distributed end-points (servers,
    devices and clients), which talk to the same central message-broker as
    defined by its host, port and topic. The DeviceClient establishes a direct
    connection to the broker. You may specify which broker and topic should be
    used via the environment variables

      KARABO_BROKER       (default: amqp://xfel:karabo@exfl-broker-1:5672)
      KARABO_BROKER_TOPIC (default: $USER)

    where defaults stated above are given if the environment variable is not
    set.
    """

    def __init__(self):
        global cpp_client
        if cpp_client is None:
            cpp_client = BoundDeviceClient()
        self.__client = cpp_client

        self.values = dict()

    def instantiate(self, serverId, classId, deviceId, config=Hash(),
                    timeout=None):
        """Instantiate (and configure) a device on a running server.

        NOTE: This call is synchronous (blocking)

        Args:
        :param serverId: The serverId of the server on which the device should
                        be started.
        :param classId:  The classId of the device (corresponding plugin must
                        be loaded on the server)
        :param deviceId: The future name of the device in the Karabo
                        installation (will fail if not unique)
        :param config:   The initial configuration of the device (optional if
                        all parameters of the device are optional)
        :param timeout : Timeout in seconds until this function will be forced
                        to return
        :returns Tuple with (True, <deviceId>) in case of success or tuple with
                        (False, <errorMessage>) in case of failure
        """
        # This is hacked here and should be added to c++
        config.set("deviceId", deviceId)
        if timeout is None:
            return self.__client.instantiate(serverId, classId, config)
        return self.__client.instantiate(serverId, classId, config, timeout)

    def _instantiate(self, serverId, classId, deviceId, data, timeout=None):
        # serializer = BinarySerializerHash.create("Bin") # this does not work
        serializer = TextSerializerHash.create("Xml")
        self.instantiateNoWait(serverId, classId, deviceId,
                               serializer.load(data))

    def instantiateNoWait(self, serverId, classId, deviceId, config=Hash()):
        """Instantiate (and configure) a device on a running server.

        NOTE: This call is asynchronous (non-blocking)

        Args:
        :param serverId: The serverId of the server on which the device should
                        be started.
        :param classId:  The classId of the device (corresponding plugin must
                        be loaded on the server)
        :param deviceId: The future name of the device in the Karabo
                        installation (will fail if not unique)
        :param config:   The initial configuration of the device (optional if
                        all parameters of the device are optional)
        """
        # This is hacked here and should be added to c++
        config.set("deviceId", deviceId)
        self.__client.instantiateNoWait(serverId, classId, config)

    def shutdownDevice(self, deviceId, timeout=None):
        """Shuts down a device.

        NOTE: This call is synchronous (blocking)

        Args:
        :param deviceId: The deviceId of the device to be destructed.
        :param timeout : Timeout in seconds until this function will be forced
                        to return
        :returns Tuple with (True, <deviceId>) in case of success or tuple with
                        (False, <errorMessage>) in case of failure
        """
        if timeout is None:
            return self.__client.killDevice(deviceId)
        return self.__client.killDevice(deviceId, timeout)

    def shutdownDeviceNoWait(self, deviceId):
        """Shuts down a device.

        NOTE: This call is asynchronous (non-blocking)

        Args:
        :param deviceId: The deviceId of the device to be destructed.
        """
        self.__client.killDeviceNoWait(deviceId)

    def shutdownServer(self, serverId, timeout=None):
        """Shuts down a server.

        NOTE: This call is synchronous (blocking)

        Args:
        :param serverId: The serverId of the server to be destructed.
        :param timeout : Timeout in seconds until this function will be forced
                        to return
        :returns Tuple with (True, <serverId>) in case of success or tuple with
                (False, <errorMessage>) in case of failure
        """
        if timeout is None:
            return self.__client.killServer(serverId)
        return self.__client.killServer(serverId, timeout)

    def shutdownServerNoWait(self, serverId):
        """Shuts down a server.

        NOTE: This call is asynchronous (non-blocking)

        Args:
        :param serverId: The serverId of the server to be destructed.
        """
        self.__client.killServerNoWait(serverId)

    def getServers(self):
        """Returns a list of currently running servers."""
        return self.__client.getServers()

    def getDevices(self, serverId=None):
        """Returns a list of currently running devices.

        Args:
        :param serverId: Optionally only the running devices of a given server
                        can be listed.
        """
        if serverId is None:
            return self.__client.getDevices()
        return self.__client.getDevices(serverId)

    def getClasses(self, serverId):
        """Returns a list of available device classes (plugins) on a server

        Args:
        :param serverId: The server of whose plugins should be listed.
        """
        return self.__client.getClasses(serverId)

    def help(self, instanceId, parameter=None):
        """Provides help on device instance or parameter

        This function provides help on a full instance or a specific
        parameter of an instance. Instance is also called the name or address
        of the specific device.
        """
        if parameter is None:
            self.__client.getDeviceSchema(instanceId).help()
        else:
            self.__client.getDeviceSchema(instanceId).help(parameter)

    def get(self, instanceId, propertyName=None):
        """Retrieves attribute values from device

        It only retrieves attributes marked for DAQ, throws an exception
        otherwise.
        """
        if propertyName is None:
            return self.__client.get(instanceId)
        return self.__client.get(instanceId, propertyName)

    def getFromPast(self, deviceId, propertyName, t0, t1=None,
                    maxNumData=10000):
        """Deprecated, use getPropertyHistory instead"""
        return self.getPropertyHistory(deviceId, propertyName, t0, t1=t1,
                                       maxNumData=maxNumData)

    def getPropertyHistory(self, deviceId, propertyName, t0, t1=None,
                           maxNumData=10000):
        """Get the history of device properties

        With this function one can get all values of a property in a given
        timespan::

        getPropertyHistory(deviceId, propertyName, "2015-12-01", "2015-12-02")

        returns a list of Hashes, which contain all changes of *propertyName*
        between the two given dates. Each Hash has a node with key 'v'. Its
        value is the one of the property at the time defined by the attributes
        'sec' and 'frac' which holds the seconds and attoseconds, respectively,
        since 1970-01-01 UTC. The attribute 'tid' holds the train ID.

        The dates of the timespan are parsed using
        :func:`dateutil.parser.parse`, allowing many ways to write the date.
        The most precise way is to write "2015-12-01T15:32:12 UTC", but you may
        omit any part, like "10:32", only giving the time, where we assume
        the current day.  Unless specified otherwise, your local timezone is
        assumed.

        Another parameter, *maxNumData*, may be given, which gives the maximum
        number of data points to be returned. The returned data will be
        reduced appropriately to still span the full timespan. *maxNumData=0*
        means no reduction - note that the history request might timeout.
        """
        utc_t0 = self._fromTimeStringToUtcString(t0)
        utc_t1 = self._fromTimeStringToUtcString(
            t1) if t1 is not None else datetime.datetime.now().isoformat()

        return self.__client.getFromPast(deviceId, propertyName, utc_t0,
                                         utc_t1, maxNumData)

    def getDeviceHistory(self, deviceId, timepoint):
        """Get configuration of a device at a given time point

        Similar to 'getConfigurationFromPast' of the C++ DeviceClient.
        Concerning the format of the timepoint, see getPropertyHistory.
        """
        return self.getConfigurationFromPast(deviceId, timepoint)

    def getConfigurationFromPast(self, deviceId, timepoint):
        """Same as getDeviceHistory

        Kept for interface similarity with the C++ DeviceClient.
        """
        utc_timepoint = self._fromTimeStringToUtcString(timepoint)
        return self.__client.getConfigurationFromPast(deviceId, utc_timepoint)

    def getSystemInformation(self):
        return self.__client.getSystemInformation()

    def getSystemTopology(self):
        return self.__client.getSystemTopology()

    def getClassSchema(self, serverId, classId):
        return self.__client.getClassSchema(serverId, classId)

    def getDeviceSchema(self, deviceId):
        return self.__client.getDeviceSchema(deviceId)

    def getDeviceSchemaNoWait(self, deviceId):
        return self.__client.getDeviceSchemaNoWait(deviceId)

    def registerSchemaUpdatedMonitor(self, callbackFunction):
        """Registers an async call-back on schema update

        This function can be used to register an asynchronous call-back on
        schema update from the distributed system.

        Args:
        :param callbackFunction: the call-back function to be registered.
                    It must have the following signature: f(str, Schema)

        Note:
        Currently, registering only a schema update monitor with an instance
        of a DeviceClient is not enough to have the registered call-back
        activated.
        A workaround for this is to also register a property monitor with the
        same instance of DeviceClient that has been used to register the schema
        update monitor.

        Example:

        def onSchemaUpdate(deviceId, schema):
            print("{}: {}".format(deviceId, schema))

        def onPropertyChange(deviceId, key, value, timeStamp):
            # In this example the property monitor is empty.
            pass

        c = DeviceClient()
        c.registerSchemaUpdatedMonitor(onSchemaUpdate)
        c.registerPropertyMonitor("Test_Device_0", "result", onPropertyChange)
        """
        return self.__client.registerSchemaUpdatedMonitor(callbackFunction)

    def registerDeviceMonitor(self, instanceId, callbackFunction,
                              userData=None):
        """Registers an async call-back on change of device property

        This function can be used to register an asynchronous call-back on
        change of any device property. The call-back function must have the
        following signature: f(str, Hash)
        arg1: deviceId
        arg2: currently changed part of the configuration

        Example:

        def onDeviceChange(deviceId, config):
            print deviceId, ":", config

        c = DeviceClient();
        c.registerDeviceMonitor("Test_MyDevice_0", onDeviceChange)
        """
        if userData is None:
            return self.__client.registerDeviceMonitor(
                instanceId, callbackFunction)
        return self.__client.registerDeviceMonitor(
            instanceId, callbackFunction, userData)

    def unregisterDeviceMonitor(self, instanceId):
        self.__client.unregisterDeviceMonitor(instanceId)

    def registerChannelMonitor(self, channelName, dataHandler=None,
                               inputChannelCfg=None, eosHandler=None,
                               inputHandler=None, statusTracker=None):
        """Register an asynchronous call-back to monitor defined output channel

        Internally, an InputChannel is created and configured.
        :param channelName identifies the channel as a concatenation of the id
                            of its device, a colon (:) and the name of the
                            output channel (e.g. A/COOL/DEVICE:output)
        :param dataHandler called when data arrives, arguments are data (Hash)
                            and meta data (Hash/MetaData)
        :param inputChannelCfg configures the channel via
                        InputChannel.create(..) use default except you know
                        what your are doing.
                        For experts: "connectedOutputChannels" will be
                                        overwritten.
                                      "onSlowness" default is overwritten
                                        to "drop"
        :param eosHandler called on end of stream,
                        argument is the InputChannel
        :param inputHandler called when data arrives,
                        argument is the InputChannel
        :param statusTracker called with a 'ConnectionStatus' as argument when
                             the connection status of the underlying
                             InputChannel changes.

        :returns False if channel is already registered

        Example:

        def handler(data, meta):
            print(data.getPaths())
        def tracker(status):
            if status is ConnectionStatus.CONNECTED:
                print("Ready for data")
            elif status is ConnectionStatus.DISCONNECTED:
                print("Lost data connection")
        c = DeviceClient();
        c.registerChannelMonitor("DEV/ID/1:output", handler,
                                 statusTracker=tracker)
        """
        if inputChannelCfg is None:
            inputChannelCfg = Hash()
        return self.__client.registerChannelMonitor(channelName, dataHandler,
                                                    inputChannelCfg,
                                                    eosHandler, inputHandler,
                                                    statusTracker)

    def unregisterChannelMonitor(self, channelName):
        """Unregister monitoring of output channel

        :param channelName identifies the channel as a concatenation of the id
                            of its devices, a colon (:) and the name of the
                            output channel (e.g. A/COOL/DEVICE:output)
        :return False if channel was not registered
        """
        return self.__client.unregisterChannelMonitor(channelName)

    def setDeviceMonitorInterval(self, milliseconds):
        """Set wait interval between subsequent handlers

        Set interval to wait between subsequent (for the same instance)
        calls to handlers registered via registerDeviceMonitor.
        Changes received within that interval will be cached and, in case of
        several updates of the same property within the interval, only the most
        up-to-date value will be handled.
        If negative, switch off caching and call handler immediately.
        """
        self.__client.setDeviceMonitorInterval(milliseconds)

    def registerPropertyMonitor(self, instanceId, propertyName,
                                callbackFunction, userData=None):
        """Register an asynchronous call-back on change of property

        This function can be used to register an asynchronous call-back on
        change of the specified property. The call-back function must have the
        following signature: f(str, str, object, Timestamp)
        arg1: deviceId
        arg2: key
        arg3: value
        arg4: timeStamp

        Example:

        def onPropertyChange(deviceId, key, value, timeStamp):
            print( deviceId, ":", key, "->", value, "(",
                   timeStamp.toFormattedString(), ")" )

        c = DeviceClient()
        c.registerPropertyMonitor("Test_Device_0", "result", onPropertyChange)
        """
        if userData is None:
            return self.__client.registerPropertyMonitor(
                instanceId, propertyName, callbackFunction)
        else:
            return self.__client.registerPropertyMonitor(
                instanceId, propertyName, callbackFunction, userData)

    def unregisterPropertyMonitor(self, instanceId, propertyName):
        self.__client.unregisterPropertyMonitor(instanceId, propertyName)

    def set(self, instanceId, propertyName, propertyValue, timeoutInSeconds=-1,
            keySep="."):
        return self.__client.set(instanceId, propertyName, propertyValue,
                                 keySep, timeoutInSeconds)

    def setHash(self, instanceId, hash, timeoutInSeconds=-1):
        return self.__client.set(instanceId, hash, timeoutInSeconds)

    def setNoWait(self, instanceId, propertyName, propertyValue):
        return self.__client.setNoWait(instanceId, propertyName, propertyValue)

    def execute(self, instanceId, command):
        """Executes a command"""
        return self.__client.execute(instanceId, command)

    def executeN(self, instanceId, command, *args):
        """Executes a command with args and return values"""
        return self.__client.executeN(instanceId, command, *args)

    def executeNoWait(self, deviceId, command):
        """Executes a command without waiting"""
        self.__client.executeNoWait(deviceId, command)

    def sleep(self, secs):
        time.sleep(secs)

    def _fromTimeStringToUtcString(self, timestamp):
        date = parser.parse(timestamp)
        if date.tzname() is None:
            print("Assuming local time for given date ", date)
            local_tz = tzlocal.get_localzone()
            date = local_tz.localize(date)
            date = date.astimezone(pytz.utc)
        print(date.isoformat())
        return date.isoformat()
