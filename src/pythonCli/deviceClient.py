#!/usr/bin/python

from libkarathon import DeviceClient as CppDeviceClient
from libkarathon import Hash
from libkarathon import Schema

import IPython.core.ipapi
ip = IPython.core.ipapi.get()
import re
import time

# Create one instance (global singleton) of a DeviceClient
cpp_client = None

# The global autocompleter
def auto_complete_full(self, event):

    if (re.match('.*\,\s*$', event.line)):
    	return [" \""]
    
    if (re.match('.*\(\s*\".+\",\".+\"\\s*\).*$', event.line)):
        return [" # What do you want to complete here, stupid?"]	
    
    if (re.match('.*\(.*\,\s*\".*', event.line)):
    	r = re.compile('\"(.*?)\"')
	m = r.search(event.line)
    	if m:
	    arg1 = m.group(1)	    
    	    return cpp_client.getFullSchema(arg1).getPaths()
    
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
    
def auto_complete_set(self, event):

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
    
def auto_complete_execute(self, event):

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
    
def auto_complete_instantiate(self, event):

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
    
# Register hooks
ip.set_hook('complete_command', auto_complete_full, re_key = '.*get')
ip.set_hook('complete_command', auto_complete_full, re_key = '.*registerPropertyMonitor')
ip.set_hook('complete_command', auto_complete_full, re_key = '.*registerDeviceMonitor')
ip.set_hook('complete_command', auto_complete_full, re_key = '.*help')
ip.set_hook('complete_command', auto_complete_full, re_key = '.*kill')


ip.set_hook('complete_command', auto_complete_set, re_key = '.*set')
ip.set_hook('complete_command', auto_complete_execute, re_key = '.*execute')
ip.set_hook('complete_command', auto_complete_instantiate, re_key = '.*instantiate')


class DeviceClient(object):
    def __init__(self, connectionType = "Jms", config = Hash()):
        global cpp_client
        if cpp_client is None:
            cpp_client = CppDeviceClient(connectionType, config)
        self.__client = cpp_client
                      
        
    def instantiate(self, deviceServerInstanceId, classId, initialConfiguration = Hash()):
        self.__client.instantiateNoWait(deviceServerInstanceId, classId, initialConfiguration)
        
        
    def kill(self, instanceId):
        self.__client.kill(instanceId)


    def help(self, instanceId, parameter = None):
        """This function provides help on a full instance or a specific parameter of an instance"""
        if parameter is None:
            self.__client.getFullSchema(instanceId).help()
        else:
            self.__client.getFullSchema(instanceId).help(parameter)
    

    def get(self, instanceId, propertyName = None):
        if propertyName is None: 
            return self.__client.get(instanceId)
        else: 
            return self.__client.get(instanceId, propertyName)
        
        
    def registerDeviceMonitor(self, instanceId, callbackFunction, userData = None):
        """This function can be used to register an asynchronous call-back on change of the specified parameter"""
        if userData is None:
            return self.__client.registerDeviceMonitor(instanceId, callbackFunction)
        else :
            return self.__client.registerDeviceMonitor(instanceId, callbackFunction, userData)
        
        
    def unregisterDeviceMonitor(self, instanceId):
        self.__client.unregisterDeviceMonitor(instanceId)
         
         
    def registerPropertyMonitor(self, instanceId, propertyName, callbackFunction, userData = None):
        """This function can be used to register an asynchronous call-back on change of the specified parameter"""
        if userData is None:
            return self.__client.registerPropertyMonitor(instanceId, propertyName, callbackFunction)
        else :
            return self.__client.registerPropertyMonitor(instanceId, propertyName, callbackFunction, userData)
        
        
    def unregisterPropertyMonitor(self, instanceId, propertyName):
        self.__client.unregisterPropertyMonitor(instanceId, propertyName)
            
       
    def set(self, instanceId, propertyName, propertyValue, timeout = -1):
        return self.__client.setWait(instanceId, propertyName, propertyValue, ".", timeout)
    
        
    def execute(self, instanceId, command):
        """Executes a command"""
        self.__client.executeNoWait(instanceId, command)
        
        
    def executeNoWait(deviceId, command):
        """Executes a command"""
        self.__client.executeNoWait(deviceId, command)
   
    
    def sleep(secs):
        time.sleep(secs)