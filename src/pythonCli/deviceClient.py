#!/usr/bin/python

from libkarathon import DeviceClient
from libkarathon import Hash
from libkarathon import Schema


import IPython.core.ipapi
ip = IPython.core.ipapi.get()
import re
import time

# Create one instance (global singleton) of a DeviceClient
device_client = None


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
    	    return device_client.getFullSchema(arg1).getPaths()
    
    if (re.match('.*\(\s*\"$', event.line) ):
        if len(device_client.getDevices()) > 0 :
            return device_client.getDevices()
        else: return ["NO_INSTANCES_AVAILABLE"]
    
    if (re.match('.*\(\s*\"[^"]+$', event.line)):
        if len(device_client.getDevices()) > 0 :
            return device_client.getDevices()
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
    	    return device_client.getCurrentlySettableProperties(arg1)
    
    if (re.match('.*\(\s*\"$', event.line) ):
        return device_client.getDevices()
    
    if (re.match('.*\(\s*\"[^"]+$', event.line)):
        return device_client.getDevices()
	
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
    	    return device_client.getCurrentlyExecutableCommands(arg1)
    
    if (re.match('.*\(\s*\"$', event.line) ):
        return device_client.getDevices()
    
    if (re.match('.*\(\s*\"[^"]+$', event.line)):
        return device_client.getDevices()
	
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
    	    return device_client.getClasses(arg1)
    
    if (re.match('.*\(\s*\"$', event.line) ):
        return device_client.getServers()
    
    if (re.match('.*\(\s*\"[^"]+$', event.line)):
        return device_client.getServers()
	
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


def init(connectionType = "Jms", config = Hash()):
    global device_client
    if device_client is None:
        device_client = DeviceClient(connectionType, config)
                      
        
def instantiate(deviceServerInstanceId, classId, initialConfiguration = Hash()):
    device_client.instantiateNoWait(deviceServerInstanceId, classId, initialConfiguration)


def kill(instanceId):
    device_client.kill(instanceId)


def help(instanceId, parameter = None):
    """This function provides help on a full instance or a specific parameter of an instance"""
    if parameter is None:
        device_client.getFullSchema(instanceId).help()
    else:
        device_client.getFullSchema(instanceId).help(parameter)


def get(instanceId, propertyName = None):
    if propertyName is None: 
        return device_client.get(instanceId)
    else: 
        return device_client.get(instanceId, propertyName)


def registerDeviceMonitor(instanceId, callbackFunction, userData = None):
    """This function can be used to register an asynchronous call-back on change of the specified parameter"""
    if userData is None:
        return device_client.registerDeviceMonitor(instanceId, callbackFunction)
    else :
        return device_client.registerDeviceMonitor(instanceId, callbackFunction, userData)


def unregisterDeviceMonitor(instanceId):
    device_client.unregisterDeviceMonitor(instanceId)


def registerPropertyMonitor(instanceId, propertyName, callbackFunction, userData = None):
    """This function can be used to register an asynchronous call-back on change of the specified parameter"""
    if userData is None:
        return device_client.registerPropertyMonitor(instanceId, propertyName, callbackFunction)
    else :
        return device_client.registerPropertyMonitor(instanceId, propertyName, callbackFunction, userData)


def unregisterPropertyMonitor(instanceId, propertyName):
    device_client.unregisterPropertyMonitor(instanceId, propertyName)


def set(instanceId, propertyName, propertyValue, timeout = -1):
    return device_client.setWait(instanceId, propertyName, propertyValue, ".", timeout)


def execute(instanceId, command):
    """Executes a command"""
    device_client.executeNoWait(instanceId, command)

    
def sleep(secs):
    time.sleep(secs)

   
        
        
    
        
    
    
    
        
    
    
    
    
    
    
    
        
