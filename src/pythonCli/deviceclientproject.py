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
from karabo.project import Project, BaseDevice

import os.path


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


class DeviceClientProject(Project):
    Device = ProjectDevice

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
