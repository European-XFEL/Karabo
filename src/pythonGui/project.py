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


    def addDevice(self, configuration):
        self.devices.append(configuration)


    def addScene(self, scene):
        self.scenes.append(scene)


    def fromHash(self, hash):
        print "fromHash"


    def toHash(self):
        print "toHash"

    
    def save(self):
        print "save project..."


    def load(self):
        print "load project"


class Scene(object):

    def __init__(self, name):
        super(Scene, self).__init__()

        self.name = name
        self.filename = ""
        # GraphicsView
        self.view = None


    def initView(self):
        """
        The graphical representation of this scene is created.
        """
        self.view = GraphicsView()

