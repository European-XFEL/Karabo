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
from karabo.project import Project, ProjectConfiguration

import os.path
from tempfile import NamedTemporaryFile
from zipfile import ZipFile, ZIP_DEFLATED


class DeviceClientProject(Project):

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


    def unzip(self):
        with ZipFile(self.filename, "r") as zf:
            data = zf.read("{}.xml".format(self.PROJECT_KEY))
            projectConfig = XMLParser().read(data)

            self.version = projectConfig[self.PROJECT_KEY, "version"]

            projectConfig = projectConfig[self.PROJECT_KEY]
            for d in projectConfig[self.DEVICES_KEY]:
                serverId = d.get("serverId")
                
                filename = d.get("filename")
                data = zf.read("{}/{}".format(self.DEVICES_KEY, filename))
                assert filename.endswith(".xml")
                filename = filename[:-4]

                for classId, config in XMLParser().read(data).iteritems():
                    device = ProjectDevice(serverId, classId, filename, d.get("ifexists"))
                    device.initConfig = config
                    break # there better be only one!
                self.addDevice(device)
            for deviceId, configList in projectConfig[
                                self.CONFIGURATIONS_KEY].iteritems():
                # Vector of hashes
                for c in configList:
                    filename = c.get("filename")
                    configuration = ProjectConfiguration(self, filename)
                    data = zf.read("{}/{}".format(self.CONFIGURATIONS_KEY,
                                                  filename))
                    configuration.fromXml(data)
                    self.addConfiguration(deviceId, configuration)
            self.resources = {k: v for k, v in
                              projectConfig["resources"].iteritems()}


    def zip(self, filename=None):
        """
        This method saves this project as a zip file.
        """
        projectConfig = Hash()
        exception = None

        if filename is None:
            filename = self.filename

        if os.path.exists(filename):
            file = NamedTemporaryFile(dir=os.path.dirname(filename),
                                      delete=False)
        else:
            file = filename

        with ZipFile(file, mode="w", compression=ZIP_DEFLATED) as zf:
            for device in self.devices:
                zf.writestr("{}/{}".format(self.DEVICES_KEY, device.filename),
                            device.toXml())
            projectConfig[self.DEVICES_KEY] = [Hash("serverId", device.serverId,
                                                    "classId", device.classId,
                                                    "filename", device.filename,
                                                    "ifexists", device.ifexists)
                                            for device in self.devices]

            configs = Hash()
            for deviceId, configList in self.configurations.iteritems():
                configs[deviceId] = [Hash("filename", c.filename)
                                     for c in configList]
                for c in configList:
                    zf.writestr("{}/{}".format(self.CONFIGURATIONS_KEY,
                                               c.filename), c.toXml())
            projectConfig[self.CONFIGURATIONS_KEY] = configs

            resources = Hash()
            if file is not self.filename:
                with ZipFile(self.filename, "r") as zin:
                    for k, v in self.resources.iteritems():
                        for fn in v:
                            f = "resources/{}/{}".format(k, fn)
                            zf.writestr(f, zin.read(f))
                        resources[k] = v
            projectConfig["resources"] = resources

            # Create folder structure and save content
            projectConfig = Hash(self.PROJECT_KEY, projectConfig)
            projectConfig[self.PROJECT_KEY, "version"] = self.version
            zf.writestr("{}.xml".format(Project.PROJECT_KEY),
                        XMLWriter().write(projectConfig))

        if file is not filename:
            file.close()
            os.remove(filename)
            os.rename(file.name, filename)

        if exception is not None:
            raise exception


    def instantiate(self, deviceIds):
        """
        This function instantiates the list of \devices.
        """
        devices = self.getDevices(deviceIds)
        if not devices:
            print "The given devices do not belong to this project and " \
                  "therefore can not be instantiated."
        
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
            print "The given devices do not belong to this project and " \
                  "therefore can not be shutdown."
        
        for d in devices:
            self.deviceClient.shutdownDevice(d.deviceId)


    def shutdownAll(self):
        """
        This function shuts down all project devices.
        """
        for d in self.devices:
            self.deviceClient.shutdownDevice(d.deviceId)


class ProjectDevice(object):

    def __init__(self, serverId, classId, deviceId, ifexists):
        super(ProjectDevice, self).__init__()

        self.serverId = serverId
        self.classId = classId
        self.deviceId = deviceId
        
        self.filename = "{}.xml".format(deviceId)
        self.ifexists = ifexists # restart, ignore
        
        # Needed in case the descriptor is not set yet
        self._initConfig = None


    @property
    def initConfig(self):
        return self._initConfig


    @initConfig.setter
    def initConfig(self, config):
        self._initConfig = config


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

