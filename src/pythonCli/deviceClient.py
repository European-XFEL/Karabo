#!/usr/bin/python
#
# Author: <burkhard.heisen@xfel.eu>
#

from karabo.karathon import DeviceClient as CppDeviceClient
from karabo.karathon import Hash
from karabo.karathon import TextSerializerHash
import karabo.karathon as krb

from karabo.deviceclientproject import DeviceClientProject

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
print("Hint, use the TAB key for auto-completion.")


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
    ip.set_hook('complete_command', auto_complete_full, re_key = '.*show')


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
    You may specify which broker and topic should be used via those environmental variables
      KARABO_BROKER_HOST
      KARABO_BROKER_PORT
      KARABO_BROKER_TOPIC
    or by giving them to the constructor (taking precedence) like:

    >>> c = DeviceClient(Hash('hostname', 'exlf-broker', 'port', 7676, 'destinationName', 'myTopic'))

    If you do not specify the broker connection, defaults will be used (IMPORTANT: default topic is your username).
    
    """
    def __init__(self, config = Hash(), connectionType = "Jms"):
        global cpp_client
        if cpp_client is None:
            cpp_client = CppDeviceClient(connectionType, config)
        self.__client = cpp_client

        # Flags whether we tried to run Qapp
        self.triedStartQapp = False

        # Flags whether we can use GuiData
        self.hasGuiData = True

        # Dict of image dialogs
        self.__imageDialogs = dict()
        
        # Dict of imagesItems
        self.__imageItems = dict()
        
        # Dict of curve dialogs
        self.__curveDialogs = dict()
        
        # Dict of curveItems
        self.__curveItems = dict()
        
         # Dict of trendline dialogs
        self.__trendlineDialogs = dict()
        
        # Dict of curveItems
        self.__trendlineItems = dict()
        
        # Dict of trendline data
        self.__trendlineData = dict()
        
        # Default colors
        self.__colors = ["red", "green", "blue", "gray", "violet", "orange", "lightgreen", "black"]
        
        # Current curve color idx
        self.__curveColorIdx = 0
        
        # Current trendline color idx
        self.__trendlineColorIdx = 0
        
        # Spec like container holding counters and pseudo counters
        self.monitor = Hash()
        
        try:
            self.monitor = krb.loadFromFile("monitor.xml").get("monitor")
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
    
    
    def getFromPast(self, deviceId, propertyName, t0, t1 = None, maxNumData = 0):
        utc_t0 = self._fromTimeStringToUtcString(t0)
        if t1 is None:            
            return self.__client.getFromPast(deviceId, propertyName, utc_t0, datetime.datetime.now().isoformat(), maxNumData)
        else:
            utc_t1 = self._fromTimeStringToUtcString(t1)
            return self.__client.getFromPast(deviceId, propertyName, utc_t0, utc_t1, maxNumData)
                
    
    def getDeviceHistory(self, deviceId, timepoint):
        utc_timepoint = self._fromTimeStringToUtcString(timepoint)
        return self.__client.getDeviceHistory(deviceId, utc_timepoint)
    
        
    def getSystemInformation(self):
        return self.__client.getSystemInformation()
    
    
    def getSystemTopology(self):
        return self.__client.getSystemTopology()
    
    
    def getClassSchema(self, serverId, classId):
        return self.__client.getClassSchema(serverId, classId)
    
    
    def getDeviceSchema(self, deviceId):
        return self.__client.getDeviceSchema(deviceId)
    
        
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
    
        
    def execute(self, instanceId, command, a1 = None):
        """Executes a command"""
        if a1 is None:
            return self.__client.execute(instanceId, command)
        else:
            return self.__client.execute(instanceId, command, a1)
        
        
    def executeNoWait(self, deviceId, command):
        """Executes a command"""
        self.__client.executeNoWait(deviceId, command)
   
    
    def sleep(self, secs):
        time.sleep(secs)


    def waitOnCondition(self, conditionString, timeoutInSeconds = -1):
        try:
            conditionOk = False
            timeout = timeoutInSeconds * 1000
            while (not conditionOk and timeout != 0):
                conditionOk = eval(conditionString)
                time.sleep(0.05)
                timeout = timeout - 50
            if (timeout == 0):
                return False, "Condition evaluation timed out"
            else:
                return True, ""
        except Exception as e:
            return False, "Invalid condition string: " + str(e)
        

    def show(self, deviceId, key, monitor = True, dialog = None, dialogName = "Karabo Embedded Visualisation", x = None, y = None, t0 = None, t1 = None, displayType = None):
        
        try:
            import guidata
            import guidata.dataset.datatypes as dt
            import guidata.dataset.dataitems as di
            from guiqwt.plot import CurveDialog
            from guiqwt.plot import ImageDialog
            from guiqwt.builder import make
            from PyQt4 import Qwt5
            from PyQt4 import QtCore

            # Create one instance of a qapplication
            if not self.triedStartQapp:
                self.qapplication = guidata.qapplication()
                self.triedStartQapp = True

            class DateTimeScaleDraw( Qwt5.Qwt.QwtScaleDraw ):
                """
                Class used to draw a datetime axis on a plot.
                """
                def __init__( self, *args ):
                    Qwt5.Qwt.QwtScaleDraw.__init__( self, *args )


                def label( self, value ):
                    '''Function used to create the text of each label
                    used to draw the axis.
                    '''
                    try:
                        dt = datetime.datetime.fromtimestamp( value )
                    except:
                        dt = datetime.datetime.fromtimestamp( 0 )
                    return Qwt5.Qwt.QwtText( dt.isoformat() )

        except ImportError:
            print("Missing module (guidata): Interactive image visualization disabled (feature will be enabled later on MacOSX)")
            self.hasGuiData=False

        if not self.hasGuiData: return
        itemId = deviceId + ":" + key
        unit = str()
        
        if self.__client.exists(deviceId)[0]:
            schema = self.__client.getDeviceSchema(deviceId)            
            if (schema.hasMetricPrefix(key)): unit += schema.getMetricPrefixSymbol(key)
            if (schema.hasUnit(key)): unit += schema.getUnitSymbol(key)
            if (len(unit) > 0): unit = " [" + unit + "]"
            displayType = "Scalar"
            if (schema.hasDisplayType(key)): displayType = schema.getDisplayType(key)
        else:
            if displayType is None: displayType = "Scalar"
            
        if (displayType == "Image"):
            if x is None: x = 600
            if y is None: y = 600
            image = self.__client.get(deviceId, key)
            if (len(image.get("data")) > 0):
                if (dialog is None): 
                    dialog = ImageDialog(toolbar=True, wintitle=dialogName, options=dict(show_contrast=True))
                    dialog.resize(x, y)
                imageItem = self._hashImageToGuiqwtImage(image)
                plot = dialog.get_plot()
                plot.add_item(imageItem)
                dialog.show()
                if monitor:
                    self.__imageItems[itemId] = imageItem
                    self.__imageDialogs[itemId] = dialog
                    self.__client.registerPropertyMonitor(deviceId, key, self._onImageUpdate)
                return dialog
            else:
                print("WARN: Empty image")
        elif (displayType == "Curve"):
            if x is None: x = 800
            if y is None: y = 500
            data = self.__client.get(deviceId, key)
            if (len(data) > 0):
                if (dialog is None): 
                    dialog = CurveDialog(edit=False, toolbar=True, wintitle=dialogName)
                    dialog.get_plot().add_item(make.legend("TR"))
                    dialog.get_itemlist_panel().show()
                    dialog.resize(x, y)
                curveItem = make.curve(list(range(0, len(data), 1)), data, itemId + unit, color=self.__colors[self.__curveColorIdx % len(self.__colors)])
                self.__curveColorIdx += 1
                plot = dialog.get_plot()
                plot.add_item(curveItem)
                dialog.show()
                if monitor:
                    self.__curveItems[itemId] = curveItem
                    self.__curveDialogs[itemId] = dialog
                    self.__client.registerPropertyMonitor(deviceId, key, self._onCurveUpdate)
                return dialog
        elif (displayType == "Scalar"): # and schema.getValueType(key) != krb.Types.STRING):
            if x is None: x = 800
            if y is None: y = 500
            self._prepareTrendlineData(deviceId, key, t0, t1)
            if (dialog is None): 
                dialog = CurveDialog(edit=False, toolbar=True, wintitle=dialogName)
                dialog.get_itemlist_panel().show()
                dialog.resize(x, y)
                plot = dialog.get_plot()
                plot.add_item(make.legend("TR"))
                plot.set_antialiasing( True )
                plot.setAxisTitle(Qwt5.Qwt.QwtPlot.xBottom, "Time")
                plot.setAxisTitle(Qwt5.Qwt.QwtPlot.yLeft, "Value")    
                plot.setAxisScaleDraw(Qwt5.Qwt.QwtPlot.xBottom, DateTimeScaleDraw())
                plot.setAxisAutoScale(Qwt5.Qwt.QwtPlot.yLeft)
                plot.setAxisLabelRotation( Qwt5.Qwt.QwtPlot.xBottom, -45.0)
                plot.setAxisLabelAlignment(Qwt5.Qwt.QwtPlot.xBottom, QtCore.Qt.AlignLeft | QtCore.Qt.AlignBottom)                               
            trendlineItem = make.curve([x[ 1 ] for x in self.__trendlineData[itemId]], [x[ 0 ] for x in self.__trendlineData[itemId]], itemId + unit, color=self.__colors[self.__trendlineColorIdx % len(self.__colors)])
            self.__trendlineColorIdx += 1
            plot = dialog.get_plot()
            plot.add_item(trendlineItem)
            dialog.show()
            if monitor and (t1 is None):
                    self.__trendlineItems[itemId] = trendlineItem
                    self.__trendlineDialogs[itemId] = dialog
                    self.__client.registerPropertyMonitor(deviceId, key, self._onTrendlineUpdate)                    
            return dialog
                                  
    
    def _prepareTrendlineData(self, deviceId, key, t0, t1):        
        itemId = deviceId + ":" + key
        self.__trendlineData[itemId] = []
        if t0 is not None:
            data = []
            if t1 is not None:
                data = self.getFromPast(deviceId, key, t0, t1)
            else:
                data = self.getFromPast(deviceId, key, t0)
            data.reverse()
            for hash in data:
                value = hash.get("v")
                e = krb.Epochstamp.fromHashAttributes(hash.getAttributes("v"))
                self.__trendlineData[itemId].append((value, e.toTimestamp()))
        else:
            value = self.__client.get(deviceId, key)            
            self.__trendlineData[itemId].append((value, krb.Epochstamp().toTimestamp()))
                                    
                        
    def _hashImageToNumpyImage(self, hashImage):
        dims = (hashImage.get("dims")[0], hashImage.get("dims")[1]) 
        if (hashImage.get("channelSpace") == krb.f_64_8):
            return np.frombuffer(hashImage.get("data"), dtype=np.double).reshape(dims)
        if (hashImage.get("channelSpace") == krb.f_32_4):
            return np.frombuffer(hashImage.get("data"), dtype=np.float).reshape(dims)
        if (hashImage.get("channelSpace") == krb.u_32_4):
            return np.frombuffer(hashImage.get("data"), dtype=np.uint32).reshape(dims).astype(np.float)
        if (hashImage.get("channelSpace") == krb.s_32_4):
            return np.frombuffer(hashImage.get("data"), dtype=np.int32).reshape(dims).astype(np.float)
        if (hashImage.get("channelSpace") == krb.u_16_2):
            return np.frombuffer(hashImage.get("data"), dtype=np.uint16).reshape(dims).astype(np.float)
        if (hashImage.get("channelSpace") == krb.s_16_2):
            return np.frombuffer(hashImage.get("data"), dtype=np.int16).reshape(dims).astype(np.float)
        if (hashImage.get("channelSpace") == krb.u_8_1):
            return np.frombuffer(hashImage.get("data"), dtype=np.uint8).reshape(dims).astype(np.float)
        if (hashImage.get("channelSpace") == krb.s_8_1):
            return np.frombuffer(hashImage.get("data"), dtype=np.int8).reshape(dims).astype(np.float)
        else:
            return np.frombuffer(hashImage.get("data"), dtype=np.uint8).reshape(dims).astype(np.float)
        
        
    def _hashImageToGuiqwtImage(self, hashImage):
        # TODO encoding stuff and color images
        data = self._hashImageToNumpyImage(hashImage)
        return make.image(data, title="Image", colormap='gray') 
        
    
    def _onImageUpdate(self, deviceId, key, image, timestamp):
        itemId = deviceId + ":" + key
        if (len(image.get("data")) > 0):
            if itemId in self.__imageItems:
                imageItem = self.__imageItems[itemId]
                imageItem.set_data(self._hashImageToNumpyImage(image))
                
                
    def _onCurveUpdate(self, deviceId, key, data, timestamp):
        itemId = deviceId + ":" + key
        if (len(data) > 0):
            if itemId in self.__curveItems:
                curveItem = self.__curveItems[itemId]
                curveItem.set_data(list(range(0, len(data), 1)), data) 
                
                
    def _onTrendlineUpdate(self, deviceId, key, value, timestamp):
        itemId = deviceId + ":" + key
        if itemId in self.__trendlineItems:
            self.__trendlineData[itemId].append((value, timestamp.toTimestamp()))
            trendlineItem = self.__trendlineItems[itemId]            
            trendlineItem.set_data( [x[ 1 ] for x in self.__trendlineData[itemId]], [x[ 0 ] for x in self.__trendlineData[itemId]])
    
    
    def _fromTimeStringToUtcString(self, timestamp):
        date = parser.parse(timestamp)
        if date.tzname() is None:
            print("Assuming local time for given date ", date)
            local_tz = tzlocal.get_localzone()
            date = local_tz.localize(date)
            date = date.astimezone(pytz.utc)
        print(date.isoformat())
        return date.isoformat()
        
        
    def getMonitorValues(self):
        d = dict()
        s = str()
        t = krb.Epochstamp()
        s += str(t.getSeconds()) + ' ' + str(t.getFractionalSeconds())
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
        s += '# epochSec[s] epochFrac[as]'
        for node in self.monitor:
            entry = node.getValue()
            monitorName = node.getKey()
            if entry.has("unit"):
                monitorName += '[' + entry.get("unit") + ']'
            s += ' ' + monitorName
        return s
    
    
    def reloadMonitorFile(self, filename = "monitor.xml"):
        self.monitor = krb.loadFromFile(filename).get("monitor")


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

