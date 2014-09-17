from __future__ import unicode_literals
#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on April 7, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents the project related datastructure.
"""

__all__ = ["GuiProject", "Category"]


from configuration import Configuration
from scene import Scene
from karabo.hash import Hash, XMLParser, XMLWriter
from karabo.project import DeviceGroup, Project, ProjectConfiguration
import manager

from PyQt4.QtCore import pyqtSignal, QObject

import os.path
from tempfile import NamedTemporaryFile
from zipfile import ZipFile, ZIP_DEFLATED


class GuiProject(Project, QObject):

    signalProjectModified = pyqtSignal()

    def __init__(self, filename):
        super(GuiProject, self).__init__(filename)

        # List of Scene
        self.scenes = []
        
        # States whether the project was changed or not
        self.isModified = False


    def setModified(self, isModified):
        if self.isModified == isModified:
            return
        
        self.isModified = isModified
        self.signalProjectModified.emit()


    def setupDeviceToProject(self, device):
        self.setModified(True)
        # Connect device to project to get configuration changes
        device.signalDeviceModified.connect(self.setModified)


    def addDevice(self, device):
        Project.addDevice(self, device)
        self.setupDeviceToProject(device)


    def insertDevice(self, index, device):
        Project.insertDevice(self, index, device)
        self.setupDeviceToProject(device)


    def newDevice(self, serverId, classId, deviceId, ifexists, updateNeeded=False):
        """
        A new device with the given parameters is created, added to the
        project and returned.
        """
        device = Device(serverId, classId, deviceId, ifexists)
        self.addDevice(device)
        if updateNeeded:
            self.signalProjectModified.emit()
        return device


    def newDeviceGroup(self, serverId, classId, deviceId, ifexists, prefix, start, end):
        deviceGroup = DeviceGroup()
        for index in xrange(start, end):
            id = "{}{}{}".format(deviceId, prefix, index)
            device = Device(serverId, classId, id, ifexists)
            deviceGroup.append(device)
        Project.addDeviceGroup(self, deviceGroup)
        self.signalProjectModified.emit()
        
        return deviceGroup


    def addScene(self, scene):
        self.scenes.append(scene)
        self.setModified(True)


    def addConfiguration(self, deviceId, configuration):
        Project.addConfiguration(self, deviceId, configuration)
        self.setModified(True)


    def addResource(self, category, data):
        self.setModified(True)
        return Project.addResource(self, category, data)


    def remove(self, object):
        """
        The \object should be removed from this project.
        
        Returns \index of the object in the list.
        """
        if isinstance(object, Configuration):
            index = self.devices.index(object)
            self.devices.pop(index)
            self.setModified(True)
            return index
        elif isinstance(object, Scene):
            index = self.scenes.index(object)
            self.scenes.pop(index)
            self.setModified(True)
            return index


    def unzip(self):
        with ZipFile(self.filename, "r") as zf:
            data = zf.read("{}.xml".format(self.PROJECT_KEY))
            projectConfig = XMLParser().read(data)

            self.version = projectConfig[self.PROJECT_KEY, "version"]

            projectConfig = projectConfig[self.PROJECT_KEY]
            for d in projectConfig[self.DEVICES_KEY]:
                group = d.get("group")
                if group is not None:
                    deviceGroup = DeviceGroup(d.getAttribute("group", "name"))
                    for item in group:
                        serverId = item.get("serverId")
                        filename = item.get("filename")
                        data = zf.read("{}/{}".format(self.DEVICES_KEY, filename))
                        assert filename.endswith(".xml")
                        filename = filename[:-4]

                        for classId, config in XMLParser().read(data).iteritems():
                            device = Device(serverId, classId, filename,
                                            item.get("ifexists"))

                            device.futureConfig = config
                            deviceGroup.append(device)
                            break # there better be only one!
                    self.addDeviceGroup(deviceGroup)
                else:
                    serverId = d.get("serverId")
                    filename = d.get("filename")
                    data = zf.read("{}/{}".format(self.DEVICES_KEY, filename))
                    assert filename.endswith(".xml")
                    filename = filename[:-4]

                    for classId, config in XMLParser().read(data).iteritems():
                        device = self.newDevice(serverId, classId, filename,
                                                d.get("ifexists"), False)
                        device.futureConfig = config
                        break # there better be only one!
            for s in projectConfig[self.SCENES_KEY]:
                scene = Scene(self, s["filename"])
                data = zf.read("{}/{}".format(self.SCENES_KEY, s["filename"]))
                scene.fromXml(data)
                self.addScene(scene)
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
        
        self.setModified(False)


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
            deviceHash = []
            for deviceObj in self.devices:
                if isinstance(deviceObj, Configuration):
                    zf.writestr("{}/{}".format(self.DEVICES_KEY, deviceObj.filename),
                                deviceObj.toXml())
                    
                    deviceHash.append(Hash("serverId", deviceObj.serverId,
                                           "classId", deviceObj.classId,
                                           "filename", deviceObj.filename,
                                           "ifexists", deviceObj.ifexists))
                else:
                    group = []
                    for device in deviceObj:
                        zf.writestr("{}/{}".format(self.DEVICES_KEY, device.filename),
                                    device.toXml())
                        group.append(Hash("serverId", device.serverId,
                                          "classId", device.classId,
                                          "filename", device.filename,
                                          "ifexists", device.ifexists))
                    deviceGroupHash = Hash("group", group)
                    deviceGroupHash.setAttribute("group", "name", deviceObj.name)
                    deviceHash.append(deviceGroupHash)
            projectConfig[self.DEVICES_KEY] = deviceHash

            for scene in self.scenes:
                name = "{}/{}".format(self.SCENES_KEY, scene.filename)
                try:
                    zf.writestr(name, scene.toXml())
                except Exception as e:
                    if file is not self.filename:
                        with ZipFile(self.filename, 'r') as zin:
                            zf.writestr(name, zin.read(name))
                    if exception is None:
                        exception = e
            projectConfig[self.SCENES_KEY] = [Hash("filename", scene.filename)
                                              for scene in self.scenes]

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
        
        self.setModified(False)


    def _instantiateDevice(self, device):
        if device.isOnline():
            if device.ifexists == "ignore":
                return
            elif device.ifexists == "restart":
                # This might take a while...
                manager.Manager().shutdownDevice(device.id, False)

        if device.descriptor is None:
            config = device.futureConfig
        else:
            config = device.toHash()

        manager.Manager().initDevice(device.serverId, device.classId, device.id,
                                     config)


    def instantiate(self, device):
        """
        This function instantiates the \device.
        """
        self._instantiateDevice(device)


    def instantiateAll(self):
        """
        This function instantiates all project devices.
        """
        for d in self.devices:
            self._instantiateDevice(d)


    def shutdown(self, device):
        """
        This function shuts down \device.
        """
        manager.Manager().shutdownDevice(device.id, True)


    def shutdownAll(self):
        """
        This function shuts down all project devices.
        """
        for d in self.devices:
            manager.Manager().shutdownDevice(d.id, False)


class Device(Configuration):
    """
    This class represents a device configuration associated with a project.
    """
    
    signalDeviceModified = pyqtSignal(bool)

    def __init__(self, serverId, classId, deviceId, ifexists, descriptor=None):
        super(Device, self).__init__(deviceId, "projectClass", descriptor)

        self.serverId = serverId
        self.classId = classId
        
        self.filename = "{}.xml".format(deviceId)
        self.ifexists = ifexists # restart, ignore
        
        # Needed in case the descriptor is not set yet
        self._futureConfig = None
        
        actual = manager.getDevice(deviceId)
        actual.statusChanged.connect(self.onStatusChanged)
        self.onStatusChanged(actual, actual.status, actual.error)
        
        # This flag states whether the descriptor was checked already
        # This should only happen once when the device was selected the first
        # time in the project view
        self.descriptorRequested = False


    @property
    def futureConfig(self):
        return self._futureConfig


    @futureConfig.setter
    def futureConfig(self, config):
        self._futureConfig = config
        # Merge futureConfig, if descriptor is not None
        self.mergeFutureConfig()


    def mergeFutureConfig(self):
        """
        This function merges the \self.futureConfig into the Configuration.
        This is only possible, if the descriptor has been set before.
        """
        if self.descriptor is None: return
        
        # Set default values for configuration
        self.setDefault()
        if self._futureConfig is not None:
            self.fromHash(self._futureConfig)


    def onNewDescriptor(self, conf):
        if self.descriptor is not None:
            self.redummy()
        self.descriptor = conf.descriptor
        self.mergeFutureConfig()
        manager.Manager().onShowConfiguration(self)


    def fromXml(self, xmlString):
        """
        This function loads the corresponding XML file of this configuration.
        """
        self.fromHash(XMLParser().read(xmlString))


    def toXml(self):
        """
        This function returns the configurations' XML file as a string.
        """
        if self.descriptor is not None:
            config = self.toHash()
        else:
            config = self._futureConfig
        return XMLWriter().write(Hash(self.classId, config))


    def onStatusChanged(self, conf, status, error):
        """ this method gets the status of the corresponding real device,
        and finds out the gory details for this project device """
        self.error = error

        if manager.Manager().systemHash is None:
            self.status = "offline"
            return

        if status == "offline":
            try:
                attrs = manager.Manager().systemHash[
                    "server.{}".format(self.serverId), ...]
            except KeyError:
                self.status = "noserver"
            else:
                if self.classId not in attrs.get("deviceClasses", []):
                    self.status = "noplugin"
                else:
                    self.status = "offline"
        else:
            if (conf.classId == self.classId and
                    conf.serverId == self.serverId):
                self.status = status
            else:
                self.status = "incompatible"


    def isOnline(self):
        return self.status not in (
            "offline", "noplugin", "noserver", "incompatible")


class Category(object):
    """
    This class represents a project category and is only used to have an object
    for the view items.
    """
    def __init__(self, displayName):
        super(Category, self).__init__()
        
        self.displayName = displayName

