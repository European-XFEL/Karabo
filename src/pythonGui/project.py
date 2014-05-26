from __future__ import unicode_literals
#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on April 7, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents the project related datastructure.
"""

__all__ = ["Project", "Scene", "ProjectConfiguration", "Category"]


from configuration import Configuration
from graphicsview import GraphicsView
from karabo.hash import Hash, XMLParser, XMLWriter

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
        elif isinstance(object, Scene):
            self.scenes.remove(object)
        # TODO: for others as well
        
        del object


    def unzip(self, filename):
        zf = ZipFile(filename)
        try:
            data = zf.read("{}.xml".format(Project.PROJECT_KEY))
        except KeyError:
            print "ERROR: Did not find %s in zip file" % "{}.xml".format(Project.PROJECT_KEY)
        
        projectConfig = XMLParser().read(data)
        
        self.version = projectConfig.getAttribute(Project.PROJECT_KEY, "version")
        self.name = projectConfig.getAttribute(Project.PROJECT_KEY, "name")
        self.directory = projectConfig.getAttribute(Project.PROJECT_KEY, "directory")
        
        projConfig = projectConfig.get(Project.PROJECT_KEY)
        
        for category in projConfig.keys():
            if category == Project.DEVICES_KEY:
                devices = projConfig.get(category)
                # Vector of hashes
                for d in devices:
                    filename = d.get("filename")
                    data = zf.read(os.path.join(Project.DEVICES_KEY, filename))
                    fi = QFileInfo(filename)
                    if len(fi.suffix()) > 1:
                        filename = fi.baseName()
                    
                    config = XMLParser().read(data)
                    # classId comes from configuration hash
                    classId = config.keys()[0]
                    device = Device(filename, classId, config.get(classId))
                    device.ifexists = d.get("ifexists")
                    self.addDevice(device)
            elif category == Project.SCENES_KEY:
                scenes = projConfig.get(category)
                # Vector of hashes
                for s in scenes:
                    filename = s.get("filename")
                    scene = Scene(self, filename)
                    data = zf.read(os.path.join(Project.SCENES_KEY, filename))
                    scene.fromXml(data)
                    self.addScene(scene)
            elif category == Project.CONFIGURATIONS_KEY:
                configurations = projConfig.get(category)
                for deviceId in configurations.keys():
                    configList = configurations[deviceId]
                    # Vector of hashes
                    for c in configList:
                        filename = c.get("filename")
                        configuration = ProjectConfiguration(self, c.get("filename"))
                        data = zf.read(os.path.join(Project.CONFIGURATIONS_KEY, filename))
                        configuration.fromXml(data)
                        self.addConfiguration(deviceId, configuration)
            elif category == Project.MACROS_KEY:
                pass
            elif category == Project.MONITORS_KEY:
                pass
            elif category == Project.RESOURCES_KEY:
                pass


    def zip(self):
        """
        This function save this project as a zip file.
        """
        absoluteProjectPath = os.path.join(self.directory, "{}.krb".format(self.name))
        zf = ZipFile(absoluteProjectPath, mode="w", compression=ZIP_DEFLATED)
        
        # Create folder structure and save content
        projectConfig = Hash(Project.PROJECT_KEY, Hash())
        projectConfig.setAttribute(Project.PROJECT_KEY, "version", self.version)
        projectConfig.setAttribute(Project.PROJECT_KEY, "name", self.name)
        projectConfig.setAttribute(Project.PROJECT_KEY, "directory", self.directory)
        
        # Handle devices
        devicePath = "{}.{}".format(Project.PROJECT_KEY, Project.DEVICES_KEY)
        deviceVec = []
        for device in self.devices:
            zf.writestr(os.path.join(Project.DEVICES_KEY, device.filename), device.toXml())
            h = Hash("filename", device.filename, "ifexists", device.ifexists)
            deviceVec.append(h)
        projectConfig.set(devicePath, deviceVec)
        
        # Handle scenes
        scenePath = "{}.{}".format(Project.PROJECT_KEY, Project.SCENES_KEY)
        sceneVec = []
        for scene in self.scenes:
            zf.writestr(os.path.join(Project.SCENES_KEY, scene.filename), scene.toXml())
            sceneVec.append(Hash("filename", scene.filename))
        projectConfig.set(scenePath, sceneVec)
        
        # Handle configurations
        configPath = "{}.{}".format(Project.PROJECT_KEY, Project.CONFIGURATIONS_KEY)
        configHash = Hash()
        for deviceId, configList in self.configurations.iteritems():
            configVec = []
            for c in configList:
                zf.writestr(os.path.join(Project.CONFIGURATIONS_KEY, c.filename),
                            c.toXml())
                configVec.append(Hash("filename", c.filename))
            configHash.set(deviceId, configVec)
        projectConfig.set(configPath, configHash)
        
        # Handle macros
        macroPath = "{}.{}".format(Project.PROJECT_KEY, Project.MACROS_KEY)
        projectConfig.set(macroPath, Hash())
        for macro in self.macros:
            # TODO
            pass
        
        # Handle resources
        resourcePath = "{}.{}".format(Project.PROJECT_KEY, Project.RESOURCES_KEY)
        projectConfig.set(resourcePath, Hash())
        for resource in self.resources:
            # TODO
            pass

        # Handle monitors
        monitorPath = "{}.{}".format(Project.PROJECT_KEY, Project.MONITORS_KEY)
        projectConfig.set(monitorPath, Hash())
        for montitor in self.monitors:
            # TODO
            pass

        projectData = XMLWriter().write(projectConfig)
        try:
            zf.writestr("{}.xml".format(Project.PROJECT_KEY), projectData)
        finally:
            zf.close()


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
        
        self.filename = path
        fi = QFileInfo(self.filename)
        if len(fi.suffix()) < 1:
            self.filename = "{}.xml".format(self.filename)
        
        self.classId = classId
        self.ifexists = "ignore" # restart, ignore, keep
        # Needed in case the descriptor is not set yet
        self.futureConfig = config
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
        self.fromHash(self.futureConfig)


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


class Scene(object):

    def __init__(self, project, name):
        super(Scene, self).__init__()
        
        # Reference to the project this scene belongs to
        self.project = project

        self.filename = name
        fi = QFileInfo(self.filename)
        if len(fi.suffix()) < 1:
            self.filename = "{}.svg".format(self.filename)
        
        # GraphicsView
        self.view = GraphicsView()


    def open(self):
        self.view.load()


    def fromXml(self, xmlString):
        """
        This function loads the corresponding SVG file of this scene into the
        view.
        """
        self.view.sceneFromXml(xmlString)


    def toXml(self):
        """
        This function returns the scenes' SVG file as a string.
        """
        return self.view.sceneToXml()


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

