#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 9, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents the project related datastructure.
"""

__all__ = ["DeviceClientProject", "ProjectDevice"]


from karabo.hash import Hash, XMLParser, XMLWriter
from karabo.project import Project, BaseDevice, BaseDeviceGroup

import csv
from datetime import datetime
import os.path

from threading import Timer


class ProjectDevice(BaseDevice):

    def __init__(self, serverId, classId, deviceId, ifexists):
        BaseDevice.__init__(self, serverId, classId, deviceId, ifexists)
        self.deviceId = deviceId
        self.initConfig = None


    def fromXml(self, xmlString):
        """
        This function loads the corresponding XML file of this configuration.
        """
        self._initConfig = XMLParser().read(xmlString)


    def toXml(self):
        """
        This function returns the configurations' XML file as a string.
        """
        return XMLWriter().write(Hash(self.classId, self._initConfig))



class ProjectDeviceGroup(BaseDeviceGroup):

    def __init__(self, id, serverId, classId, ifexists):
        BaseDeviceGroup.__init__(self, serverId, classId, id, ifexists)

        self.serverId = None
        self.classId = None



class DeviceClientProject(Project):
    Device = ProjectDevice
    DeviceGroup = ProjectDeviceGroup

    def __init__(self, filename, deviceClient):
        super(DeviceClientProject, self).__init__(filename)

        self.deviceClient = deviceClient


    def remove(self, object):
        """
        The \object should be removed from this project.
        
        Returns \index of the object in the list.
        """
        if isinstance(object, ProjectDevice):
            index = self.devices.index(object)
            self.devices.pop(index)
            return index


    def instantiate(self, deviceIds):
        """
        This function instantiates the list of \devices.
        """
        devices = self.getDevices(deviceIds)
        if not devices:
            print("The given devices do not belong to this project and " \
                  "therefore can not be instantiated.")
        
        runningDevices = self.deviceClient.getDevices()
        for d in devices:
            if d.deviceId in runningDevices:
                if d.ifexists == "ignore":
                    continue
                elif d.ifexists == "restart":
                    self.deviceClient.shutdownDevice(d.deviceId)
            
            data = XMLWriter().write(d.initConfig)
            self.deviceClient._instantiate(d.serverId, d.classId, d.deviceId,
                                           data)


    def instantiateAll(self):
        """
        This function instantiates all project devices.
        """
        runningDevices = self.deviceClient.getDevices()
        for d in self.devices:
            if d.deviceId in runningDevices:
                if d.ifexists == "ignore":
                    continue
                elif d.ifexists == "restart":
                    self.deviceClient.shutdownDevice(d.deviceId)
            
            #data = BinaryWriter().write(d.initConfig) # this does not work
            data = XMLWriter().write(d.initConfig)
            self.deviceClient._instantiate(d.serverId, d.classId, d.deviceId,
                                           data)


    def shutdown(self, deviceIds):
        """
        This function shuts down the list of \devices.
        """
        devices = self.getDevices(deviceIds)
        if not devices:
            print("The given devices do not belong to this project and " \
                  "therefore can not be shutdown.")
        
        for d in devices:
            self.deviceClient.shutdownDevice(d.deviceId)


    def shutdownAll(self):
        """
        This function shuts down all project devices.
        """
        for d in self.devices:
            self.deviceClient.shutdownDevice(d.deviceId)


    def startMonitoring(self, filename, interval=1):
        """
        Monitoring is started and all necessary variables are initiated.
        """
        if self.isMonitoring:
            print("Monitoring is already started.")
            return
        
        self.isMonitoring = True
        
        if not filename.endswith(".csv"):
            filename = "{}.csv".format(filename)
        
        self.monitorFile = open(filename, "a")
        self.monitorWriter = csv.writer(self.monitorFile)
        
        self.monitorTuples = []
        monitorString = []
        for m in self.monitors:
            # Connect box changes of properties to be monitored
            deviceId = m.config.get("deviceId")
            deviceProperty = m.config.get("deviceProperty")
            self.monitorTuples.append((deviceId, deviceProperty))
            
            monitorString.append("{}[{}{}]".format(m.config.get("name"),
                                             m.config.get("metricPrefixSymbol"),
                                             m.config.get("unitSymbol")))
        
        self.monitorWriter.writerow(["#timestamp"] + monitorString)
        if interval > 0:
            self.monitorTimer = MonitorTimer(interval, self.timerEvent)
            self.monitorTimer.start()
        else:
            self.monitorTimer = None
        
        print("Monitoring started...")


    def stopMonitoring(self):
        """
        Monitoring is stopped and all variables are reset.
        """
        if not self.isMonitoring:
            print("Monitoring is already stopped.")
            return
        
        self.isMonitoring = False

        if self.monitorTimer is not None:
            self.monitorTimer.stop()
        self.monitorFile.close()
        
        print("Monitoring stopped...")


    def timerEvent(self):
        """
        Write to monitor file.
        """
        if self.monitorWriter is None:
            return
        
        timestamp = datetime.now().isoformat()
        row = [timestamp] + [self.deviceClient.get(deviceData[0], deviceData[1]) for deviceData in self.monitorTuples]
        self.monitorWriter.writerow(row)



class MonitorTimer(object):
    """
    This class represents a time which can call a \function in a given \interval.
    
    Usage:
    mt = MonitorTimer(interval, self.timerEvent)
    mt.start()
    sleep(5)
    mt.stop()
    """

    def __init__(self, interval, function, *args, **kwargs):
        self._timer = None
        self.interval = interval
        self.function = function
        self.args = args
        self.kwargs = kwargs
        self.is_running = False


    def _run(self):
        self.is_running = False
        self.start()
        self.function(*self.args, **self.kwargs)


    def start(self):
        if not self.is_running:
            self._timer = Timer(self.interval, self._run)
            self._timer.start()
            self.is_running = True


    def stop(self):
        self._timer.cancel()
        self.is_running = False

