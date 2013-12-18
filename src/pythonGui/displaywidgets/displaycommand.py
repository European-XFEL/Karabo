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
from displaywidget import DisplayWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


def getCategoryAliasClassName():
    return ["Slot","Command","DisplayCommand"]


class DisplayCommand(DisplayWidget):
  
    def __init__(self, **params):
        super(DisplayCommand, self).__init__(**params)

        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        self.__allowedStates = params.get('allowedStates')
        commandText = params.get('commandText')
        commandEnabled = params.get('commandEnabled')
        self.__command = params.get('command')
            
        self.__pbCommand = QPushButton(commandText)
        self.__pbCommand.setEnabled(commandEnabled)
        self.__pbCommand.clicked.connect(self.onCommandClicked)
        self.__key = params.get('key')
        
        # TODO: better solution
        Manager().notifier.signalDeviceStateChanged.connect(self.onDeviceStateChanged)


    def _getCategory(self):
        category, alias, className = getCategoryAliasClassName()
        return category
    category = property(fget=_getCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__pbCommand
    widget = property(fget=_getWidget)
    
    
    def _getAllowedStates(self):
        als = []
        for state in self.__allowedStates:
            als.append(str(state))
        return als
    allowedStates = property(fget=_getAllowedStates)
    
    def _getCommand(self):
        return self.__command
    command = property(fget=_getCommand)


    # Returns a tuple of min and max number of associated keys with this component
    def _getMinMaxAssociatedKeys(self):
        return self.__minMaxAssociatedKeys
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        return [self.__key]
    keys = property(fget=_getKeys)


    def _value(self):
        return None
    value = property(fget=_value)


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


    def valueChanged(self, key, value, timestamp=None):
       
        pass


### slots ###
    def onDeviceStateChanged(self, internalDeviceId, state):
        isFound = False
        for key in self.keys:
            deviceId = key.split(".configuration")[0]
            if deviceId == internalDeviceId:
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
            Manager().executeCommand(dict(path=key, command=self.__command, args=args))


    class Maker:
        def make(self, **params):
            return DisplayCommand(**params)

