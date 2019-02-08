#!/usr/bin/python
#
# Author: <burkhard.heisen@xfel.eu>
#

from karathon import Hash
from karathon import TextSerializerHash
import karathon as krb

from karabo.bound import DeviceClient as BoundDeviceClient
from karabo.middlelayer_api.deviceclientproject import DeviceClientProject

import IPython
import re
import time
import getpass
import socket
import datetime
import os.path
from dateutil import parser
import pytz
import tzlocal
from threading import Thread
from sys import platform
import numpy as np

#ip = IPython.core.ipapi.get()
ip = IPython.get_ipython()


# Create one instance (global singleton) of a DeviceClient
cpp_client = None

def _getVersion():
        if "win" in platform:        
            # TODO: use current working path pythonGui/VERSION
            filePath = os.path.join(os.environ['USERPROFILE'], "karabo", "karaboFramework")
        else:
            filePath = os.path.join(os.environ['HOME'], ".karabo", "karaboFramework")
            
        try:
            with open(filePath, 'r') as file:
                karaboVersionPath = os.path.join(file.readline().rstrip(), "VERSION")
        except IOError as e:
            print(e)
            return ""

        try:
            with open(karaboVersionPath, 'r') as file:
                return file.readline().rstrip('\n\r')
        except IOError:
            return ""

# Welcome
print("\n#### Karabo Device-Client (version:", _getVersion(),") ####")
print("To start you need a DeviceClient object, e.g. type:\n")
print("  d = DeviceClient()\n")
print("Using this object you can remote control Karabo devices.")
print("You may query servers and devices and set/get properties or execute commands on them.")
print("Hint: use the TAB key for auto-completion.")


# The global autocompleter
def auto_complete_full(self, event):
    try:
        if (re.match('.*\,\s*$', event.line)):
            return [" \""]

        if (re.match('.*\(\s*\".+\",\".+\"\\s*\).*$', event.line)):
            return [" # What do you want to complete here, stupid?"]	

        if (re.match('.*\(.*\,\s*\".*', event.line)):
            r = re.compile('\"(.*?)\"')
            m = r.search(event.line)
            if m:
                deviceId = m.group(1)	    
                return cpp_client.getProperties(deviceId)

        if (re.match('.*\(\s*\"$', event.line) ):
            if len(cpp_client.getDevices()) > 0 :
                return cpp_client.getDevices()
            else: return ["NO_INSTANCES_AVAILABLE"]

        if (re.match('.*\(\s*\"[^"]+$', event.line)):
            if len(cpp_client.getDevices()) > 0 :
                return cpp_client.getDevices()
            else: return ["NO_INSTANCES_AVAILABLE"]

        if (re.match('.*\($', event.line)):
            return ["\""]
    except:
        print("Distributed auto-completion failed")
    
def auto_complete_set(self, event):
    try:
        if (re.match('.*\,\s*$', event.line)):
            return [" \""]

        if (re.match('.*\(\s*\".+\",\".+\"\\s*\).*$', event.line)):
            return [" # What do you want to complete here, stupid?"]	

        if (re.match('.*\(.*\,\s*\".*', event.line)):
            r = re.compile('\"(.*?)\"')
            m = r.search(event.line)
            if m:
                arg1 = m.group(1)	    
                return cpp_client.getCurrentlySettableProperties(arg1)

        if (re.match('.*\(\s*\"$', event.line) ):
            return cpp_client.getDevices()

        if (re.match('.*\(\s*\"[^"]+$', event.line)):
            return cpp_client.getDevices()

        if (re.match('.*\($', event.line)):
            return ["\""]
    except:
        print("Distributed auto-completion failed")
    
def auto_complete_execute(self, event):
    try:
        if (re.match('.*\,\s*$', event.line)):
            return [" \""]

        if (re.match('.*\(\s*\".+\",\".+\"\\s*\).*$', event.line)):
            return [" # What do you want to complete here, stupid?"]	

        if (re.match('.*\(.*\,\s*\".*', event.line)):
            r = re.compile('\"(.*?)\"')
            m = r.search(event.line)
            if m:
                arg1 = m.group(1)	    
                return cpp_client.getCurrentlyExecutableCommands(arg1)

        if (re.match('.*\(\s*\"$', event.line) ):
            return cpp_client.getDevices()

        if (re.match('.*\(\s*\"[^"]+$', event.line)):
            return cpp_client.getDevices()

        if (re.match('.*\($', event.line)):
            return ["\""]
    except:
        print("Distributed auto-completion failed")
        
def auto_complete_instantiate(self, event):
    try:
        # Fourth argument
        if (re.match('.*\(.*\,.*\,.*\,\s*$', event.line)):
            return [' Hash("']

        if (re.match('.*\(.*\,.*\,.*\,\s*Hash\(\"$', event.line)):
            r = re.compile('\"(.*?)\"\,\s*\"(.*?)\"')
            m = r.search(event.line)
            if m:
                serverId = m.group(1)
                classId = m.group(2)
                return cpp_client.getClassProperties(serverId, classId)

        if (re.match('.*\,\s*$', event.line)):
            return [" \""]

        if (re.match('.*\(.*\,\s*\".*', event.line)):
            r = re.compile('\"(.*?)\"')
            m = r.search(event.line)
            if m:
                arg1 = m.group(1)	    
                return cpp_client.getClasses(arg1)

        if (re.match('.*\(\s*\"$', event.line) ):
            return cpp_client.getServers()

        if (re.match('.*\(\s*\"[^"]+$', event.line)):
            return cpp_client.getServers()

        if (re.match('.*\($', event.line)):
            return ["\""]
    except:
        print("Distributed auto-completion failed")
    
# Register hooks
if (ip is not None):
    ip.set_hook('complete_command', auto_complete_full, re_key = '.*get')
    ip.set_hook('complete_command', auto_complete_full, re_key = '.*registerPropertyMonitor')
    ip.set_hook('complete_command', auto_complete_full, re_key = '.*registerDeviceMonitor')
    ip.set_hook('complete_command', auto_complete_full, re_key = '.*help')
    ip.set_hook('complete_command', auto_complete_full, re_key = '.*shutdownDevice')


    ip.set_hook('complete_command', auto_complete_set, re_key = '.*set')
    ip.set_hook('complete_command', auto_complete_execute, re_key = '.*execute')
    ip.set_hook('complete_command', auto_complete_instantiate, re_key = '.*instantiate')
    ip.set_hook('complete_command', auto_complete_instantiate, re_key = '.*getClassSchema')
    ip.set_hook('complete_command', auto_complete_instantiate, re_key = '.*shutdownServer')
    ip.set_hook('complete_command', auto_complete_instantiate, re_key = '.*getClasses')


class DeviceClient(object):
    """
    The DeviceClient allows to remotely control a Karabo installation.
    A Karabo installation comprises all distributed end-points (servers, devices and clients),
    which talk to the same central message-broker as defined by its host, port and topic.
    The DeviceClient establishes a direct connection to the broker.
    You may specify which broker and topic should be used via the environment variables

      KARABO_BROKER       (default: tcp://exfl-broker:7777)
      KARABO_BROKER_TOPIC (default: $USER)

    where defaults stated above are given if the environment variable is not
    set.
    """
    def __init__(self):
        global cpp_client
        if cpp_client is None:
            cpp_client = BoundDeviceClient()
        self.__client = cpp_client

        # Flags whether we tried to run Qapp
        self.triedStartQapp = False

        # Flags whether we can use GuiData
        self.hasGuiData = True

        # Spec like container holding counters and pseudo counters
        self.monitor = Hash()
        
        try:
            self.reloadMonitorFile()
        except:
            pass
        
        self.values = dict()

    def login(self, username, passwordFile = None, provider = "LOCAL"):
        """
        Logs the the given user into the Karabo installation.

        After successful login all user-specifc access rights are established and the
        auto-completion (in interactive mode) adapts accordingly.

        Args:

            username: The username (currently you may use observer, operator, expert, admin)
            passwordFile: Optionally provide a plain text file with containing the password
            provider: Optionally choose the authorization provider (LOCAL or KERBEROS)

        """
        password = None
        if passwordFile is None:
            password = getpass.getpass()
        else:
            with open('passwordFile', 'r') as file:
                password = file.readline()
        return self.__client.login(username, password, provider)
    
    
    def logout(self):
        """
        Logs the current user out.
        """
        return self.__client.logout()
        
        
    def instantiate(self, serverId, classId, deviceId, config = Hash(), timeout = None):
        """
        Instantiate (and configure) a device on a running server.

        NOTE: This call is synchronous (blocking)

        Args:
            serverId: The serverId of the server on which the device should be started.
            classId:  The classId of the device (corresponding plugin must be loaded on the server)
            deviceId: The future name of the device in the Karabo installation (will fail if not unique)
            config:   The initial configuration of the device (optional if all parameters of the device are optional)
            timeout : Timeout in seconds until this function will be forced to return

        Returns:
            Tuple with (True, <deviceId>) in case of success or tuple with (False, <errorMessage>) in case of failure

        """
        # This is hacked here and should be added to c++
        config.set("deviceId", deviceId)
        if timeout is None:
            return self.__client.instantiate(serverId, classId, config)
        return self.__client.instantiate(serverId, classId, config, timeout)


    def _instantiate(self, serverId, classId, deviceId, data, timeout = None):
        #serializer = BinarySerializerHash.create("Bin") # this does not work
        serializer = TextSerializerHash.create("Xml")
        self.instantiateNoWait(serverId, classId, deviceId, serializer.load(data))


    def instantiateNoWait(self, serverId, classId, deviceId, config = Hash()):
        """
        Instantiate (and configure) a device on a running server.

        NOTE: This call is asynchronous (non-blocking)

        Args:
            serverId: The serverId of the server on which the device should be started.
            classId:  The classId of the device (corresponding plugin must be loaded on the server)
            deviceId: The future name of the device in the Karabo installation (will fail if not unique)
            config:   The initial configuration of the device (optional if all parameters of the device are optional)

        """

        # This is hacked here and should be added to c++
        config.set("deviceId", deviceId)
        self.__client.instantiateNoWait(serverId, classId, config)
        
        
    def shutdownDevice(self, deviceId, timeout = None):
        """
        Shuts down a device.

        NOTE: This call is synchronous (blocking)

        Args:
            deviceId: The deviceId of the device to be destructed.
            timeout : Timeout in seconds until this function will be forced to return

        Returns:
            Tuple with (True, <deviceId>) in case of success or tuple with (False, <errorMessage>) in case of failure

        """
        if timeout is None:
            return self.__client.killDevice(deviceId)
        return self.__client.killDevice(deviceId, timeout)
        
        
    def shutdownDeviceNoWait(self, deviceId):
        """
        Shuts down a device.

        NOTE: This call is asynchronous (non-blocking)

        Args:
            deviceId: The deviceId of the device to be destructed.

        """
        self.__client.killDeviceNoWait(deviceId)
        
        
    def shutdownServer(self, serverId, timeout = None):
        """
        Shuts down a server.

        NOTE: This call is synchronous (blocking)

        Args:
            serverId: The serverId of the server to be destructed.
            timeout : Timeout in seconds until this function will be forced to return

        Returns:
            Tuple with (True, <serverId>) in case of success or tuple with (False, <errorMessage>) in case of failure

        """
        if timeout is None:
            return self.__client.killServer(serverId)
        return self.__client.killServer(serverId, timeout)
        
        
    def shutdownServerNoWait(self, serverId):
        """
        Shuts down a server.

        NOTE: This call is asynchronous (non-blocking)

        Args:
            serverId: The serverId of the server to be destructed.

        """
        self.__client.killServerNoWait(serverId)


    def getServers(self):
        """
        Returns a list of currently running servers.
        """
        return self.__client.getServers()

        
    def getDevices(self, serverId = None):
        """
        Returns a list of currently running devices.

        Args:
            serverId: Optionally only the running devices of a given server can be listed.

        """
        if serverId is None:
            return self.__client.getDevices()
        return self.__client.getDevices(serverId)


    def getClasses(self, serverId):
        """
        Returns a list of available device classes (plugins) on a server

        Args:
             serverId: The server of whose plugins should be listed.

        """
        return self.__client.getClasses(serverId)


    def help(self, instanceId, parameter = None):
        """
        This function provides help on a full instance or a specific parameter of an instance.
        """
        if parameter is None:
            self.__client.getDeviceSchema(instanceId).help()
        else:
            self.__client.getDeviceSchema(instanceId).help(parameter)
    

    def get(self, instanceId, propertyName = None):
        if propertyName is None: 
            return self.__client.get(instanceId)
        return self.__client.get(instanceId, propertyName)
    
    
    def getFromPast(self, deviceId, propertyName, t0, t1 = None, maxNumData = 10000):
        """Deprecated, use getPropertyHistory instead"""
        return self.getPropertyHistory(deviceId, propertyName, t0, t1=t1, maxNumData=maxNumData)


    def getPropertyHistory(self, deviceId, propertyName, t0, t1 = None, maxNumData = 10000):
        """
        get the history of device properties

        with this function one can get all values of a property in a given
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
        utc_t1 = self._fromTimeStringToUtcString(t1) if t1 is not None else datetime.datetime.now().isoformat()

        return self.__client.getFromPast(deviceId, propertyName, utc_t0, utc_t1, maxNumData)
                
    
    def getDeviceHistory(self, deviceId, timepoint):
        """
        Get full configuration and schema of device at given time point,
        similar to 'getConfigurationFromPast' of the C++ DeviceClient.
        Concerning the format of the timepoint, see getPropertyHistory.
        """
        return self.getConfigurationFromPast(deviceId, timepoint)


    def getConfigurationFromPast(self, deviceId, timepoint):
        """
        Same as getDeviceHistory, kept for interface similarity with the C++
        DeviceClient.
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
        """
        This function can be used to register an asynchronous call-back on
        schema update from the distributed system.

        Args:
            callbackFunction: the call-back function to be registered.
                It must have the following signature: f(str, Schema)

        Example:

        def onSchemaUpdate(deviceId, schema):
            print("{}: {}".format(deviceId, schema))

        c = DeviceClient()
        c.registerSchemaUpdatedMonitor(onSchemaUpdate)

        """
        return self.__client.registerSchemaUpdatedMonitor(callbackFunction)


    def registerDeviceMonitor(self, instanceId, callbackFunction, userData = None):
        """
        This function can be used to register an asynchronous call-back on change of any device property.
        The call-back function must have the following signature: f(str, Hash)
        arg1: deviceId
        arg2: currently changed part of the configuration
        
        
        Example:        
                      
        def onDeviceChange(deviceId, config):
            print deviceId, ":", config
            
        c = DeviceClient();
        c.registerDeviceMonitor("Test_MyDevice_0", onDeviceChange)

        """
        if userData is None:
            return self.__client.registerDeviceMonitor(instanceId, callbackFunction)
        return self.__client.registerDeviceMonitor(instanceId, callbackFunction, userData)
        
        
    def unregisterDeviceMonitor(self, instanceId):
        self.__client.unregisterDeviceMonitor(instanceId)

    def registerChannelMonitor(self, channelName, dataHandler,
                               inputChannelCfg=None, eosHandler=None):
        """
        Register an asynchronous call-back to monitor defined output channel.

        Internally, an InputChannel is created and configured.
        @param channelName identifies the channel as a concatenation of the id of
                            its device, a colon (:) and the name of the output
                            channel (e.g. A/COOL/DEVICE:output)
        @param dataHandler called when data arrives, arguments are data (Hash) and
                           meta data (Hash/MetaData)
        @param inputChannelCfg configures the channel via InputChannel.create(..)
                         use default except you know what your are doing
                         for experts: "connectedOutputChannels" will be overwritten,
                                      "onSlowness" default is overwritten to "drop"
        @param eosHandler called on end of stream, argument is the InputChannel

        @return False if channel is already registered

        Example:

        def handler(data, meta):
            print(data.getPaths())
        c = DeviceClient();
        c.registerChannelMonitor("DEV/ID/1:output", handler)
        """
        if inputChannelCfg is None:
            inputChannelCfg = Hash()
        return self.__client.registerChannelMonitor(channelName, dataHandler,
                                                    inputChannelCfg, eosHandler)

    def unregisterChannelMonitor(self, channelName):
        """
        Unregister monitoring of output channel

        @param channelName identifies the channel as a concatenation of the id of
                           its devices, a colon (:) and the name of the output
                          channel (e.g. A/COOL/DEVICE:output)
        @return False if channel was not registered
        """
        return self.__client.unregisterChannelMonitor(channelName)

    def setDeviceMonitorInterval(self, milliseconds):
        """
        Set interval to wait between subsequent (for the same instance)
        calls to handlers registered via registerDeviceMonitor.
        Changes received within that interval will be cached and, in case of
        several updates of the same property within the interval, only the most
        up-to-date value will be handled.
        If negative, switch off caching and call handler immediately.
        """
        self.__client.setDeviceMonitorInterval(milliseconds)


    def registerPropertyMonitor(self, instanceId, propertyName, callbackFunction, userData = None):
        """
        This function can be used to register an asynchronous call-back on change of the specified property.
        The call-back function must have the following signature: f(str, str, object, Timestamp)
        arg1: deviceId
        arg2: key
        arg3: value
        arg4: timeStamp
        
        Example:
            
        def onPropertyChange(deviceId, key, value, timeStamp):
            print deviceId, ":", key, "->", value, "(", timeStamp.toFormattedString(), ")"
        
        c = DeviceClient()
        c.registerPropertyMonitor("Test_MyDevice_0", "result", onPropertyChange)

        """
        if userData is None:
            return self.__client.registerPropertyMonitor(instanceId, propertyName, callbackFunction)
        else :
            return self.__client.registerPropertyMonitor(instanceId, propertyName, callbackFunction, userData)
        
        
    def unregisterPropertyMonitor(self, instanceId, propertyName):
        self.__client.unregisterPropertyMonitor(instanceId, propertyName)
            
       
    def set(self, instanceId, propertyName, propertyValue, timeoutInSeconds = -1, keySep = "."):
        return self.__client.set(instanceId, propertyName, propertyValue, keySep, timeoutInSeconds)
    
    
    def setHash(self, instanceId, hash, timeoutInSeconds = -1):
        return self.__client.set(instanceId, hash, timeoutInSeconds)
    
    
    def setNoWait(self, instanceId, propertyName, propertyValue):
         return self.__client.setNoWait(instanceId, propertyName, propertyValue)
    
        
    def execute(self, instanceId, command, *args):
        """Executes a command"""
        if len(args) == 0:
            return self.__client.execute(instanceId, command)
        elif len(args) == 1:
            return self.__client.execute1(instanceId, command, args[0])
        elif len(args) == 2:
            return self.__client.execute2(instanceId, command, args[0], args[1])
        elif len(args) == 3:
            return self.__client.execute3(instanceId, command, args[0], args[1], args[2])
        elif len(args) == 4:
            return self.__client.execute4(instanceId, command, args[0], args[1], args[2], args[3])
        else:
            raise NotImplementedError("Too many arguments.")
        
        
    def executeNoWait(self, deviceId, command, *args):
        """Executes a command"""
        if len(args) == 0:
            self.__client.executeNoWait(deviceId, command)
        elif len(args) == 1:
            self.__client.executeNoWait1(deviceId, command, args[0])
        elif len(args) == 2:
            self.__client.executeNoWait2(deviceId, command, args[0], args[1])
        elif len(args) == 3:
            self.__client.executeNoWait3(deviceId, command, args[0], args[1], args[2])
        elif len(args) == 4:
            self.__client.executeNoWait4(deviceId, command, args[0], args[1], args[2], args[3])
        else:
            raise NotImplementedError("Too many arguments.")
       
    def sleep(self, secs):
        time.sleep(secs)

    def getMonitorValues(self):
        d = dict()
        s = str()
        t = krb.Epochstamp()
        s += '{:f}'.format(t.toTimestamp())
        for node in self.monitor:
            entry = node.getValue()
            monitorName = node.getKey()
            value = None
            # Skip if disabled
            if (entry.has("disabled") and entry.get("disabled")): continue            
            
            if entry.has("deviceId"):
                deviceId = entry.get("deviceId")
                propertyName = entry.get("property")
                d[monitorName] = self.__client.get(deviceId, propertyName)        
            if entry.has("eval"):
                evalString = entry.get("eval")
                evalString = re.sub(r'\$(\w+)', r'd["\1"]', evalString)
                d[monitorName] = eval(evalString)
            if entry.has("format"):
                valueType = type(d[monitorName])
                tmp = '{:' + entry.get("format") + '}'
                formattedString = tmp.format(d[monitorName])
                s += ' ' + formattedString
                if valueType == float:
                    d[monitorName] = float(formattedString)
                elif valueType == int:
                    d[monitorName] = int(formattedString)
                elif valueType == int:
                    d[monitorName] = int(formattedString)
                elif valueType == complex:
                    d[monitorName] = complex(formattedString)
                else:
                    d[monitorName] = formattedString
            else:
                s += ' ' + str(d[monitorName])
        self.values = d    
        return s
    
    
    def getMonitorHeadline(self):
        s = str()
        s += '# timestamp'
        for node in self.monitor:
            entry = node.getValue()
            monitorName = node.getKey()
            if entry.has("unit"):
                monitorName += '[' + entry.get("unit") + ']'
            s += ' ' + monitorName
        return s
    
    
    def reloadMonitorFile(self, filename = "monitor.xml"):
        self.__monitorFile = krb.loadFromFile(filename)
        if (self.__monitorFile.has("monitor")):
            self.monitor = self.__monitorFile.get("monitor")
        else:
            print("Missing \"monitor\" section")


    def loadProject(self, filename):
        """
        This function loads a project via \filename and returns a project object.
        """
        project = DeviceClientProject(filename, self)
        try:
            project.unzip()
        except Exception as e:
            e.message = "While reading the project a <b>critical error</b> " \
                        "occurred."
            raise

        return project

    def setPrio(self, deviceId, priority):
        """
        This function changes the Logger priority of the deviceId
        
        Example:
        
            setPrio('Karabo_DataLoggerServer', 'DEBUG')
            setPrio('Karabo_DataLoggerServer", 'INFO')
            
        """
        self.executeNoWait(deviceId, "slotLoggerPriority", priority)


    def lock(self, deviceId, recursive = False):
        return self.__client.lock(deviceId, recursive)
