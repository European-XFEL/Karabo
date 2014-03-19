#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 8, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class DisplayWidget.
   
   Each plugin needs to implement the following interface:
   
   def getCategoryAliasClassName():
       pass
   
    class Maker:
        def make(self, **params):
            return Attribute*(**params)
"""

__all__ = ["DisplayCommand"]


from manager import Manager
from widget import DisplayWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class DisplayCommand(DisplayWidget):
    category = "Slot"
    alias = "Command"
  
    def __init__(self, command, commandEnabled, commandText, allowedStates, **params):
        super(DisplayCommand, self).__init__(**params)

        self.command = command

        self.__allowedStates = allowedStates
        self.__pbCommand = QPushButton(commandText)
        self.__pbCommand.setEnabled(commandEnabled)
        self.__pbCommand.clicked.connect(self.onCommandClicked)
        
        # TODO: better solution
        Manager().signalDeviceStateChanged.connect(self.onDeviceStateChanged)


    @property
    def widget(self):
        return self.__pbCommand
    
    
    @property
    def allowedStates(self):
        return [s for s in self.__allowedStates]

    value = None

    def valueChanged(self, key, value, timestamp=None):
        pass


### slots ###
    def onDeviceStateChanged(self, deviceId, state):
        isFound = False
        for key in self.keys:
            devId = key.split(".", 1)[0]
            if devId == deviceId:
                isFound = True
        
        if not isFound:
            return
        
        if len(self.__allowedStates) < 1:
            return
        
        if state in self.__allowedStates:
            self.__pbCommand.setEnabled(True)
        else:
            self.__pbCommand.setEnabled(False)


    def onCommandClicked(self):
        args = [] # TODO slot arguments
        for key in self.keys:
            Manager().executeCommand(dict(path=key, command=self.command,
                                          args=args))
