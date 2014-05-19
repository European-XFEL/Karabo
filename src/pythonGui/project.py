from __future__ import unicode_literals
#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on April 7, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents the project related datastructure.
"""

__all__ = ["Project", "Scene", "Category"]


from configuration import Configuration
from graphicsview import GraphicsView
from karabo.hash import Hash, XMLParser, XMLWriter

from PyQt4.QtCore import pyqtSignal, QDir, QObject
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
        self.macros = []
        self.configurations = []
        self.resources = []
        self.monitors = []


    def addDevice(self, device):
        self.devices.append(device)


    def addScene(self, scene):
        self.scenes.append(scene)


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
                    classId = d.keys()[0]
                    config = d.get(classId)
                    device = Device(config.get("deviceId"), classId, config)
                    self.addDevice(device)
            elif category == Project.SCENES_KEY:
                scenes = projConfig.get(category)
                # Vector of hashes
                for s in scenes:
                    scene = Scene(self, s.get("name"))
                    data = zf.read(os.path.join(Project.SCENES_LABEL, s.get("filename")))
                    scene.sceneFromXml(data)
                    self.addScene(scene)
            elif category == Project.MACROS_KEY:
                pass
            elif category == Project.MONITORS_KEY:
                pass
            elif category == Project.RESOURCES_KEY:
                pass
            elif category == Project.CONFIGURATIONS_KEY:
                pass


    def zip(self):
        """
        This function save this project as a zip file.
        """
        absoluteProjectPath = os.path.join(self.directory, self.name)
        zf = ZipFile(absoluteProjectPath, mode="w", compression=ZIP_DEFLATED)
        
        # Create folder structure and save content
        projectConfig = Hash(Project.PROJECT_KEY, Hash())
        projectConfig.setAttribute(Project.PROJECT_KEY, "version", self.version)
        projectConfig.setAttribute(Project.PROJECT_KEY, "name", self.name)
        projectConfig.setAttribute(Project.PROJECT_KEY, "directory", self.directory)
        
        # Handle devices
        devicePath = "{}.{}".format(Project.PROJECT_KEY, Project.DEVICES_KEY)
        projectConfig.set(devicePath, Hash())
        deviceConfig = []
        for device in self.devices:
            config = device.toHash()
            deviceConfig.append(Hash(device.classId, config))
        projectConfig.set(devicePath, deviceConfig)
        
        # Handle scenes
        scenePath = "{}.{}".format(Project.PROJECT_KEY, Project.SCENES_KEY)
        sceneConfig = []
        for scene in self.scenes:
            zf.writestr(os.path.join(Project.SCENES_LABEL, scene.filename), scene.sceneToXml())
            sceneConfig.append(Hash("name", scene.name, "filename", scene.filename))
        projectConfig.set(scenePath, sceneConfig)

        # Handle macros
        macroPath = "{}.{}".format(Project.PROJECT_KEY, Project.MACROS_KEY)
        projectConfig.set(macroPath, Hash())
        for macro in self.macros:
            # TODO
            pass
        
        # Handle configurations
        configPath = "{}.{}".format(Project.PROJECT_KEY, Project.CONFIGURATIONS_KEY)
        projectConfig.set(configPath, Hash())
        for config in self.configurations:
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
        
        self.classId = classId
        # Needed in case the descriptor is not set yet
        self.futureConfig = config
        # Merge futureConfig, if descriptor is not None
        self.mergeFutureConfig()


    def mergeFutureConfig(self):
        """
        This function merges the \self.futureConfig into the Configuration.
        This is only possible, if the descriptor has been set before.
        """
        if self.getDescriptor() is None: return

        # Set default values for configuration
        self.setDefault()
        self.merge(self.futureConfig)


class Scene(object):

    def __init__(self, project, name):
        super(Scene, self).__init__()

        # Reference to the project this scene belongs to
        self.project = project

        self.name = name
        self.filename = "{}.svg".format(name)
        
        self.absoluteFilePath = os.path.join(project.directory, project.name,
                                             Project.SCENES_LABEL, self.filename)
        
        # GraphicsView
        self.view = GraphicsView()


    def open(self):
        self.view.load()


    def sceneFromXml(self, xmlString):
        """
        This function loads the corresponding SVG file of this scene into the
        view.
        """
        self.view.sceneFromXml(xmlString)


    def sceneToXml(self):
        """
        This function returns the scenes' SVG file as a string.
        """
        return self.view.sceneToXml()


class Category(object):
    """
    This class represents a project category and is only used to have an object
    for the view items.
    """
    def __init__(self, displayName):
        super(Category, self).__init__()
        
        self.displayName = displayName

