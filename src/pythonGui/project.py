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
        projectConfig = Hash()
        for device in self.devices:
            projectConfig.merge(device.toHash())
        
        for scene in self.scenes:
            filePath = os.path.join(absoluteProjectPath, "Scenes", scene.filename)
            # Save scene to SVG
            self.signalSaveScene.emit(scene, filePath)
        
        for macro in self.macros:
            # TODO
            pass
        
        for config in self.configurations:
            # TODO
            pass
        
        for resource in self.resources:
            # TODO
            pass
        
        for montitor in self.monitors:
            # TODO
            pass
        
        print "@@@@@@@@@@@@@@"
        print projectConfig
        print "@@@@@@@@@@@@@@"
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

