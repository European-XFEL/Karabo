from __future__ import unicode_literals
#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on April 7, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents the project related datastructure.
"""

__all__ = ["Project", "Scene"]


import os
from graphicsview import GraphicsView
from karabo.hash import Hash, XMLParser, XMLWriter

from PyQt4.QtCore import pyqtSignal, QDir, QObject


class Project(QObject):
    signalSaveScene = pyqtSignal(object, str) # scene, filename

    DEVICES_LABEL = "Devices"
    SCENES_LABEL = "Scenes"
    MACROS_LABEL = "Macros"
    MONITORS_LABEL = "Monitors"
    RESOURCES_LABEL = "Resources"
    CONFIGURATIONS_LABEL = "Configurations"

    def __init__(self, name, directory):
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


    def addDevice(self, configuration):
        self.devices.append(configuration)


    def addScene(self, scene):
        self.scenes.append(scene)


    def fromHash(self, hash):
        print "fromHash"


    def toHash(self):
        print "toHash"

    
    def save(self, overwrite=False):
        absoluteProjectPath = os.path.join(self.directory, self.name)
        print "save project...", absoluteProjectPath
        dir = QDir()
        if not QDir(absoluteProjectPath).exists():
            dir.mkpath(absoluteProjectPath)
        else:
            if not overwrite:
                reply = QMessageBox.question(None, "New project",
                    "A project folder named \"" + projectName + "\" already exists.<br>"
                    "Do you want to replace it?",
                    QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

                if reply == QMessageBox.No:
                    return

                self._clearProjectDir(absoluteProjectPath)

        # Create folder structure and save content
        projectConfig = Hash(self.name, Hash())
        
        devicePath = "{}.{}".format(self.name, Project.DEVICES_LABEL)
        projectConfig.set(devicePath, Hash())
        for device in self.devices:
            projectConfig.merge(device.toHash())
        
        scenePath = "{}.{}".format(self.name, Project.SCENES_LABEL)
        sceneConfig = []
        for scene in self.scenes:
            filePath = os.path.join(absoluteProjectPath, Project.SCENES_LABEL,
                                    scene.filename)
            # Save scene to SVG
            self.signalSaveScene.emit(scene, filePath)
            sceneConfig.append(Hash("filename", scene.filename))
        projectConfig.set(scenePath, sceneConfig)
            
        macroPath = "{}.{}".format(self.name, Project.MACROS_LABEL)
        projectConfig.set(macroPath, Hash())
        for macro in self.macros:
            # TODO
            pass
        
        configPath = "{}.{}".format(self.name, Project.CONFIGURATIONS_LABEL)
        projectConfig.set(configPath, Hash())
        for config in self.configurations:
            # TODO
            pass
        
        resourcePath = "{}.{}".format(self.name, Project.RESOURCES_LABEL)
        projectConfig.set(resourcePath, Hash())
        for resource in self.resources:
            # TODO
            pass
        
        monitorPath = "{}.{}".format(self.name, Project.MONITORS_LABEL)
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


class Scene(object):

    def __init__(self, name):
        super(Scene, self).__init__()

        self.name = name
        self.filename = name
        # GraphicsView
        self.view = None


    def initView(self):
        """
        The graphical representation of this scene is created.
        """
        self.view = GraphicsView()

