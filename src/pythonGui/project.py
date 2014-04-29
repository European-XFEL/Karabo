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


from graphicsview import GraphicsView


class Project(object):


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


    def addDevice(self, deviceConfig):
        self.devices.append(deviceConfig)


    def addScene(self, sceneData):
        self.scenes.append(sceneData)


    def fromHash(self, hash):
        print "fromHash"


    def toHash(self):
        print "toHash"


class Scene(object):

    def __init__(self, projectName, name):
        super(Scene, self).__init__()

        # Project to which this scene belongs
        self.projectName = projectName

        self.name = name
        self.filename = ""
        # GraphicsView
        self.view = None


    def initView(self):
        """
        The graphical representation of this scene is created.
        """
        self.view = GraphicsView()

