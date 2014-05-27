from __future__ import unicode_literals
#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on April 7, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents the project related datastructure.
"""

__all__ = ["Project", "ProjectConfiguration", "Category"]


from configuration import Configuration
from graphicsview import GraphicsView
from karabo.hash import Hash, XMLParser, XMLWriter
import manager

from PyQt4.QtCore import pyqtSignal, QDir, QFileInfo, QObject
#from PyQt4.QtGui import QMessageBox

import os.path
from zipfile import ZipFile, ZipInfo, ZIP_DEFLATED


class Project(QObject):

    DEVICES_LABEL = "Devices"
    SCENES_LABEL = "Scenes"
    MACROS_LABEL = "Macros"
    MONITORS_LABEL = "Monitors"
    RESOURCES_LABEL = "Resources"
    CONFIGURATIONS_LABEL = "Configurations"

    PROJECT_KEY = "project"
    DEVICES_KEY = "devices"
    SCENES_KEY = "scenes"
    MACROS_KEY = "macros"
    MONITORS_KEY = "monitors"
    RESOURCES_KEY = "resources"
    CONFIGURATIONS_KEY = "configurations"

    def __init__(self, name="", directory=""):
        super(Project, self).__init__()

        self.version = 1
        self.name = name
        self.directory = directory
        
        # List of Configuration
        self.devices = []
        # List of Scene
        self.scenes = []
        # Map for {deviceId, [ProjectConfiguration]}
        self.configurations = dict()
        self.macros = []
        self.resources = []
        self.monitors = []


    def addDevice(self, device):
        self.devices.append(device)


    def addScene(self, scene):
        self.scenes.append(scene)


    def addConfiguration(self, deviceId, configuration):
        if deviceId in self.configurations:
            self.configurations[deviceId].append(configuration)
        else:
            self.configurations[deviceId] = [configuration]


    def remove(self, object):
        """
        The \object should be removed from this project.
        """
        if isinstance(object, Configuration):
            self.devices.remove(object)
        elif isinstance(object, GraphicsView):
            self.scenes.remove(object)


    def unzip(self, filename):
        with ZipFile(filename) as zf:
            data = zf.read("{}.xml".format(self.PROJECT_KEY))
            projectConfig = XMLParser().read(data)

            self.version = projectConfig[self.PROJECT_KEY, "version"]
            self.name = projectConfig[self.PROJECT_KEY, "name"]
            self.directory = projectConfig[self.PROJECT_KEY, "directory"]

            projectConfig = projectConfig[self.PROJECT_KEY]

            for d in projectConfig[self.DEVICES_KEY]:
                filename = d.get("filename")
                data = zf.read("{}/{}".format(self.DEVICES_KEY, filename))
                assert filename.endswith(".xml")
                filename = filename[:-4]

                for classId, conf in XMLParser().read(data).iteritems():
                    device = Device(filename, classId, conf)
                    device.ifexists = d.get("ifexists")
                    break # there better be only one!
                self.addDevice(device)
            for s in projectConfig[self.SCENES_KEY]:
                scene = GraphicsView(self, s["filename"])
                data = zf.read("{}/{}".format(self.SCENES_LABEL, s["filename"]))
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


    def zip(self):
        """
        This method saves this project as a zip file.
        """
        absoluteProjectPath = os.path.join(self.directory,
                                           "{}.krb".format(self.name))
        projectConfig = Hash()

        with ZipFile(absoluteProjectPath, mode="w",
                     compression=ZIP_DEFLATED) as zf:
            for device in self.devices:
                zf.writestr("{}/{}".format(self.DEVICES_KEY, device.filename),
                            device.toXml())
            projectConfig[self.DEVICES_KEY] = [
                Hash("classId", device.classId, "filename", device.filename,
                     "ifexists", device.ifexists) for device in self.devices]

            for scene in self.scenes:
                zf.writestr("{}/{}".format(self.SCENES_LABEL, scene.filename),
                            scene.toXml())
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

            # Create folder structure and save content
            projectConfig = Hash(self.PROJECT_KEY, projectConfig)
            projectConfig[self.PROJECT_KEY, ...] = dict(
                version=self.version, name=self.name, directory=self.directory)
            zf.writestr("{}.xml".format(Project.PROJECT_KEY),
                        XMLWriter().write(projectConfig))


    def _clearProjectDir(self, absolutePath):
        if len(absolutePath) < 1:
            return

        dirToDelete = QDir(absolutePath)
        # Remove all files from directory
        fileEntries = dirToDelete.entryList(QDir.Files | QDir.CaseSensitive)
        while len(fileEntries) > 0:
            dirToDelete.remove(fileEntries.pop())

        # Remove all sub directories
        dirEntries = dirToDelete.entryList(QDir.AllDirs | QDir.NoDotAndDotDot | QDir.CaseSensitive)
        while len(dirEntries) > 0:
            subDirPath = absolutePath + "/" + dirEntries.pop()
            subDirToDelete = QDir(subDirPath)
            if len(subDirToDelete.entryList()) > 0:
                self._clearProjectDir(subDirPath)
            subDirToDelete.rmpath(subDirPath)


class Device(Configuration):

    def __init__(self, path, classId, config, descriptor=None):
        super(Device, self).__init__(path, "projectClass", descriptor)

        self.filename = "{}.xml".format(path)
        self.classId = classId
        self.ifexists = "ignore" # restart, ignore, keep
        # Needed in case the descriptor is not set yet
        self.futureConfig = config
        # Merge futureConfig, if descriptor is not None
        self.mergeFutureConfig()

        actual = manager.Manager().getDevice(config["deviceId"])
        actual.statusChanged.connect(self.onStatusChanged)
        self.onStatusChanged(actual, actual.status)


    def mergeFutureConfig(self):
        """
        This function merges the \self.futureConfig into the Configuration.
        This is only possible, if the descriptor has been set before.
        """
        if self.descriptor is None: return

        # Set default values for configuration
        self.setDefault()
        self.fromHash(self.futureConfig)


    def onNewDescriptor(self, conf):
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
        return XMLWriter().write(Hash(self.classId, self.toHash()))


    def onStatusChanged(self, conf, status):
        """ this method gets the status of the corresponding real device,
        and finds out the gory details for this project device """
        self.error = conf.error

        if manager.Manager().systemHash is None:
            self.status = "offline"
            return

        if status == "offline":
            try:
                attrs = manager.Manager().systemHash[
                    "server.{}".format(self.futureConfig["serverId"]), ...]
            except KeyError:
                self.status = "noserver"
            else:
                if self.classId not in attrs.get("deviceClasses", []):
                    self.status = "noplugin"
                else:
                    self.status = "offline"
        else:
            if (conf.classId == self.classId and
                    conf.serverId == self.futureConfig.get("serverId")):
                self.status = status
            else:
                self.status = "incompatible"


    def isOnline(self):
        return self.status not in (
            "offline", "noplugin", "noserver", "incompatible")


class ProjectConfiguration(object):

    def __init__(self, project, name, hash=None):
        super(ProjectConfiguration, self).__init__()
        
        # Reference to the project this scene belongs to
        self.project = project

        self.filename = name
        fi = QFileInfo(self.filename)
        if len(fi.suffix()) < 1:
            self.filename = "{}.xml".format(self.filename)
        
        #self.deviceId = deviceId
        #self.classId = classId
        self.hash = hash


    def fromXml(self, xmlString):
        """
        This function loads the corresponding XML file of this configuration.
        """
        self.hash = XMLParser().read(xmlString)


    def toXml(self):
        """
        This function returns the configurations' XML file as a string.
        """
        return XMLWriter().write(self.hash)


class Category(object):
    """
    This class represents a project category and is only used to have an object
    for the view items.
    """
    def __init__(self, displayName):
        super(Category, self).__init__()
        
        self.displayName = displayName

