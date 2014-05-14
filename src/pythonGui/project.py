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
from karabo.hash import Hash, XMLWriter

from PyQt4.QtCore import pyqtSignal, QDir, QObject
from PyQt4.QtGui import QMessageBox

import os.path
from zipfile import ZipFile


class Project(QObject):
    signalSaveScene = pyqtSignal(object) # scene

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


    def zip(self):
        """
        This function save this project to its zip file.
        """
        # Create folder structure and save content
        projectConfig = Hash(Project.PROJECT_KEY, Hash())
        projectConfig.setAttribute(Project.PROJECT_KEY, "name", self.name)
        projectConfig.setAttribute(Project.PROJECT_KEY, "directory", self.directory)
        
        # Create folder for devices
        devicePath = "{}.{}".format(Project.PROJECT_KEY, Project.DEVICES_KEY)
        projectConfig.set(devicePath, Hash())
        deviceConfig = []
        for device in self.devices:
            config = device.toHash()
            deviceConfig.append(Hash(device.classId, config))
        projectConfig.set(devicePath, deviceConfig)
        
        # Create folder for scenes
        #absoluteLabelPath = os.path.join(absoluteProjectPath, Project.SCENES_LABEL)
        #dir.mkpath(absoluteLabelPath)
        scenePath = "{}.{}".format(Project.PROJECT_KEY, Project.SCENES_KEY)
        sceneConfig = []
        for scene in self.scenes:
            # Save scene to SVG
            self.signalSaveScene.emit(scene)
            sceneConfig.append(Hash("name", scene.name, "filename", scene.filename))
        projectConfig.set(scenePath, sceneConfig)

        # Create folder for macros
        #absoluteLabelPath = os.path.join(absoluteProjectPath, Project.MACROS_LABEL)
        #dir.mkpath(absoluteLabelPath)
        macroPath = "{}.{}".format(Project.PROJECT_KEY, Project.MACROS_KEY)
        projectConfig.set(macroPath, Hash())
        for macro in self.macros:
            # TODO
            pass
        
        # Create folder for configurations
        #absoluteLabelPath = os.path.join(absoluteProjectPath, Project.CONFIGURATIONS_LABEL)
        #dir.mkpath(absoluteLabelPath)
        configPath = "{}.{}".format(Project.PROJECT_KEY, Project.CONFIGURATIONS_KEY)
        projectConfig.set(configPath, Hash())
        for config in self.configurations:
            # TODO
            pass
        
        # Create folder for resources
        #absoluteLabelPath = os.path.join(absoluteProjectPath, Project.RESOURCES_LABEL)
        #dir.mkpath(absoluteLabelPath)
        resourcePath = "{}.{}".format(Project.PROJECT_KEY, Project.RESOURCES_KEY)
        projectConfig.set(resourcePath, Hash())
        for resource in self.resources:
            # TODO
            pass

        # Create folder for monitors
        #absoluteLabelPath = os.path.join(absoluteProjectPath, Project.MONITORS_LABEL)
        #dir.mkpath(absoluteLabelPath)
        monitorPath = "{}.{}".format(Project.PROJECT_KEY, Project.MONITORS_KEY)
        projectConfig.set(monitorPath, Hash())
        for montitor in self.monitors:
            # TODO
            pass
        
        projectData = XMLWriter().write(projectConfig)
        
        absoluteProjectPath = os.path.join(self.directory, self.name)
        zf = ZipFile(absoluteProjectPath, mode="w")#, 
                     #compression=ZIP_DEFLATED)
        try:
            print "adding project.xml"
            zf.writestr("project.xml", projectData)
        finally:
            print "closing"
            zf.close()

    
    def save(self, overwrite=False):
        """
        This function saves this project to its directory.
        """
        absoluteProjectPath = os.path.join(self.directory, self.name)
        dir = QDir()
        if not QDir(absoluteProjectPath).exists():
            dir.mkpath(absoluteProjectPath)
        else:
            if not overwrite:
                reply = QMessageBox.question(None, "New project",
                    "A project folder named \"" + self.name + "\" already exists.<br>"
                    "Do you want to replace it?",
                    QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

                if reply == QMessageBox.No:
                    # TODO: Choose other location
                    return

                self._clearProjectDir(absoluteProjectPath)

        # Create folder structure and save content
        projectConfig = Hash(Project.PROJECT_KEY, Hash())
        projectConfig.setAttribute(Project.PROJECT_KEY, "name", self.name)
        projectConfig.setAttribute(Project.PROJECT_KEY, "directory", self.directory)
        
        # Create folder for devices
        devicePath = "{}.{}".format(Project.PROJECT_KEY, Project.DEVICES_KEY)
        projectConfig.set(devicePath, Hash())
        deviceConfig = []
        for device in self.devices:
            config = device.toHash()
            deviceConfig.append(Hash(device.classId, config))
        projectConfig.set(devicePath, deviceConfig)
        
        # Create folder for scenes
        absoluteLabelPath = os.path.join(absoluteProjectPath, Project.SCENES_LABEL)
        dir.mkpath(absoluteLabelPath)
        scenePath = "{}.{}".format(Project.PROJECT_KEY, Project.SCENES_KEY)
        sceneConfig = []
        for scene in self.scenes:
            # Save scene to SVG
            self.signalSaveScene.emit(scene)
            sceneConfig.append(Hash("name", scene.name, "filename", scene.filename))
        projectConfig.set(scenePath, sceneConfig)

        # Create folder for macros
        absoluteLabelPath = os.path.join(absoluteProjectPath, Project.MACROS_LABEL)
        dir.mkpath(absoluteLabelPath)
        macroPath = "{}.{}".format(Project.PROJECT_KEY, Project.MACROS_KEY)
        projectConfig.set(macroPath, Hash())
        for macro in self.macros:
            # TODO
            pass
        
        # Create folder for configurations
        absoluteLabelPath = os.path.join(absoluteProjectPath, Project.CONFIGURATIONS_LABEL)
        dir.mkpath(absoluteLabelPath)
        configPath = "{}.{}".format(Project.PROJECT_KEY, Project.CONFIGURATIONS_KEY)
        projectConfig.set(configPath, Hash())
        for config in self.configurations:
            # TODO
            pass
        
        # Create folder for resources
        absoluteLabelPath = os.path.join(absoluteProjectPath, Project.RESOURCES_LABEL)
        dir.mkpath(absoluteLabelPath)
        resourcePath = "{}.{}".format(Project.PROJECT_KEY, Project.RESOURCES_KEY)
        projectConfig.set(resourcePath, Hash())
        for resource in self.resources:
            # TODO
            pass

        # Create folder for monitors
        absoluteLabelPath = os.path.join(absoluteProjectPath, Project.MONITORS_LABEL)
        dir.mkpath(absoluteLabelPath)
        monitorPath = "{}.{}".format(Project.PROJECT_KEY, Project.MONITORS_KEY)
        projectConfig.set(monitorPath, Hash())
        for montitor in self.monitors:
            # TODO
            pass

        # Save project.xml
        with open(os.path.join(self.directory, self.name, 'project.xml'), 'w') as file:
            w = XMLWriter()
            w.writeToFile(projectConfig, file)


    def load(self):
        print "load project"


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

    def __init__(self, path, type, descriptor=None):
        super(Device, self).__init__(path, type, descriptor)
        
        self.classId = None
        # Needed in case the descriptor is not set yet
        self.futureConfig = None


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
        self.view = None


    def initView(self):
        """
        The graphical representation of this scene is created.
        """
        self.view = GraphicsView()


class Category(object):
    """
    This class represents a project category and is only used to have an object
    for the view items.
    """
    def __init__(self, displayName):
        super(Category, self).__init__()
        
        self.displayName = displayName

