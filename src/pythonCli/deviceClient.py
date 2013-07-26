#!/usr/bin/python
#
# Author: <burkhard.heisen@xfel.eu>
#

from karabo.karathon import DeviceClient as CppDeviceClient
from karabo.karathon import Hash
from karabo.karathon import Schema
from karabo.karathon import Authenticator
from karabo.karathon import Timestamp

import IPython.core.ipapi
import re
import time
import getpass
import socket

import numpy as np

# Not yet supported on MacOSX
HAS_GUIDATA=True
try:

    import guidata
    import guidata.dataset.datatypes as dt
    import guidata.dataset.dataitems as di
    from guiqwt.plot import CurveDialog
    from guiqwt.plot import ImageDialog
    from guiqwt.builder import make

except ImportError:
    print "Missing module (guidata): Interactive image visualization disabled (feature will be enabled later on MacOSX)"
    HAS_GUIDATA=False
    


ip = IPython.core.ipapi.get()


# Create one instance (global singleton) of a DeviceClient
cpp_client = None

# Create one instance (global singleton) of a qapplication
if HAS_GUIDATA: qapplication = guidata.qapplication()

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
        print "Distributed auto-completion failed"
    
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
        print "Distributed auto-completion failed"
    
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
        print "Distributed auto-completion failed"
        
def auto_complete_instantiate(self, event):
    try:
        # Third argument
        if (re.match('.*\(.*\,.*\,\s*$', event.line)):
            return [' Hash("']

        if (re.match('.*\(.*\,.*\,\s*Hash\(\"$', event.line)):
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
        print "Distributed auto-completion failed"
    
# Register hooks
ip.set_hook('complete_command', auto_complete_full, re_key = '.*get')
ip.set_hook('complete_command', auto_complete_full, re_key = '.*registerPropertyMonitor')
ip.set_hook('complete_command', auto_complete_full, re_key = '.*registerDeviceMonitor')
ip.set_hook('complete_command', auto_complete_full, re_key = '.*help')
ip.set_hook('complete_command', auto_complete_full, re_key = '.*killDevice')
ip.set_hook('complete_command', auto_complete_full, re_key = '.*show')


ip.set_hook('complete_command', auto_complete_set, re_key = '.*set')
ip.set_hook('complete_command', auto_complete_execute, re_key = '.*execute')
ip.set_hook('complete_command', auto_complete_instantiate, re_key = '.*instantiate')
ip.set_hook('complete_command', auto_complete_instantiate, re_key = '.*getClassSchema')
ip.set_hook('complete_command', auto_complete_instantiate, re_key = '.*killServer')


class DeviceClient(object):
    def __init__(self, connectionType = "Jms", config = Hash()):
        global cpp_client
        if cpp_client is None:
            cpp_client = CppDeviceClient(connectionType, config)
        self.__client = cpp_client

        # Dict of image dialogs
        self.__imageDialogs = dict()
        
        # Dict of imagesItems
        self.__imageItems = dict()
    
              
    def login(self, username, passwordFile = None, provider = "LOCAL"):
        password = None
        if passwordFile is None:
            password = getpass.getpass()
        else:
            with open('passwordFile', 'r') as file:
                password = file.readline()
        return self.__client.login(username, password, provider)
    
    
    def logout(self):
        return self.__client.logout()
        
        
    def instantiate(self, deviceServerInstanceId, classId, initialConfiguration = Hash(), timeoutInSeconds = None):
        if timeoutInSeconds is None:
            return self.__client.instantiate(deviceServerInstanceId, classId, initialConfiguration)
        return self.__client.instantiate(deviceServerInstanceId, classId, initialConfiguration, timeoutInSeconds)
    
    
    def instantiateNoWait(self, deviceServerInstanceId, classId, initialConfiguration = Hash()):
        self.__client.instantiateNoWait(deviceServerInstanceId, classId, initialConfiguration)
        
        
    def killDevice(self, deviceId, timeoutInSeconds = None):
        if timeoutInSeconds is None:
            return self.__client.killDevice(deviceId)
        return self.__client.killDevice(deviceId, timeoutInSeconds)
        
        
    def killDeviceNoWait(self, deviceId):
        self.__client.killDeviceNoWait(deviceId)
        
        
    def killServer(self, serverId, timeoutInSeconds = None):
        if timeoutInSeconds is None:
            return self.__client.killServer(serverId)
        return self.__client.killServer(serverId, timeoutInSeconds)
        
        
    def killServerNoWait(self, serverId):
        self.__client.killServerNoWait(serverId)


    def getServers(self):
        return self.__client.getServers()

        
    def getDevices(self, serverId = None):
        if serverId is None:
            return self.__client.getDevices()
        return self.__client.getDevices(serverId)


    def getClasses(self, serverId):
        return self.__client.getClasses(serverId)


    def help(self, instanceId, parameter = None):
        """This function provides help on a full instance or a specific parameter of an instance"""
        if parameter is None:
            self.__client.getDeviceSchema(instanceId).help()
        else:
            self.__client.getDeviceSchema(instanceId).help(parameter)
    

    def get(self, instanceId, propertyName = None):
        if propertyName is None: 
            return self.__client.get(instanceId)
        return self.__client.get(instanceId, propertyName)
    
    def enableAdvancedMode(self):
        self.__client.enableAdvancedMode()
        
        
    def disableAdvancedMode(self):
        self.__client.disableAdvancedMode()
    
        
    def getSystemInformation(self):
        return self.__client.getSystemInformation()
    
    
    def getSystemTopology(self):
        return self.__client.getSystemTopology()
    
    
    def getClassSchema(self, serverId, classId):
        return self.__client.getClassSchema(serverId, classId)
    
    
    def getDeviceSchema(self, deviceId):
        return self.__client.getDeviceSchema(deviceId)
    
        
    def registerDeviceMonitor(self, instanceId, callbackFunction, userData = None):
        '''
        This function can be used to register an asynchronous call-back on change of any device property.
        The call-back function must have the following signature: f(str, Hash)
        arg1: deviceId
        arg2: currently changed part of the configuration
        
        
        Example:        
                      
        def onDeviceChange(deviceId, config):
            print deviceId, ":", config
            
        c = DeviceClient();
        c.registerDeviceMonitor("Test_MyDevice_0", onDeviceChange)                       
        '''
        if userData is None:
            return self.__client.registerDeviceMonitor(instanceId, callbackFunction)
        return self.__client.registerDeviceMonitor(instanceId, callbackFunction, userData)
        
        
    def unregisterDeviceMonitor(self, instanceId):
        self.__client.unregisterDeviceMonitor(instanceId)
         
         
    def registerPropertyMonitor(self, instanceId, propertyName, callbackFunction, userData = None):
        '''
        This function can be used to register an asynchronous call-back on change of the specified property.
        The call-back function must have the following signature: f(str, str, object, Timestamp)
        arg1: deviceId
        arg2: key
        arg3: value
        arg4: timeStamp
        
        Example:
            
            def onPropertyChange(deviceId, key, value, timeStamp):
                print deviceId, ":", key, "->", value, "(", timeStamp.getMsSinceEpoch(), ")"
        
        c = DeviceClient()
        c.registerPropertyMonitor("Test_MyDevice_0", "result", onPropertyChange)
        '''
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


    def show(self, deviceId, key, monitor = True, dialogName = None, widgetType = None, x = 600, y = 600):
        if not HAS_GUIDATA: return 
        schema = self.__client.getDeviceSchema(deviceId)
        imageId = deviceId + ":" + key
        
        if dialogName is None: dialogName = imageId
            
        if (schema.getDisplayType(key) == "Image"):
            image = self.__client.get(deviceId, key)
            if (len(image.get("data")) > 0):
                dialog = ImageDialog(toolbar=True, wintitle=dialogName, options=dict(show_contrast=True))
                dialog.resize(x, y)
                imageItem = self._hashImageToGuiqwtImage(image)
                plot = dialog.get_plot()
                plot.add_item(imageItem)
                dialog.show()
                if monitor:
                    self.__imageItems[imageId] = imageItem
                    self.__imageDialogs[imageId] = dialog
                    self.__client.registerPropertyMonitor(deviceId, key, self._onImageUpdate)             
            else:
                print "WARN: Empty image"
            
                       
    def _hashImageToNumpyImage(self, hashImage):
        return np.frombuffer(hashImage.get("data"), dtype=np.uint8).reshape(hashImage.get("dims"))
            
            
    def _hashImageToGuiqwtImage(self, hashImage):
        # TODO encoding stuff and color images
        data = self._hashImageToNumpyImage(hashImage)
        return make.image(data, title="Image", colormap='gray') 
        
    
    def _onImageUpdate(self, deviceId, key, image, timestamp):
        imageId = deviceId + ":" + key
        if (len(image.get("data")) > 0):
            if self.__imageItems.has_key(imageId):
                imageItem = self.__imageItems[imageId]
                imageItem.set_data(self._hashImageToNumpyImage(image))
    
    
        
                
            
