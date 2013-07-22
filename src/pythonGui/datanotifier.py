#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 22, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which is used in the Manager class send an update
   signal to the relevant, registered widgets.
"""

__all__ = ["DataNotifier"]


from PyQt4.QtCore import *

class DataNotifier(QObject):
    # signal
    signalUpdateComponent = pyqtSignal(str, object) # internalKey, value, timestamp (TODO)
    #signalAddKey = pyqtSignal(str) # internalKey
    #signalRemoveKey = pyqtSignal(str) # internalKey


    def __init__(self, key, component):
        super(DataNotifier, self).__init__()
        
        self.__components = [] # list of components
        self.addComponent(key, component)


    def addComponent(self, key, component):
        # Connect signals
        self.signalUpdateComponent.connect(component.onValueChanged)
        
        if len(self.__components) > 0:
            value = self.__components[0].value
            if value:
                self.signalUpdateComponent.emit(key, value)
        
        # Add widget to list
        self.__components.append(component)


    def removeComponent(self, key, component):
        # Remove widget from set
        if component in self.__components:
            self.__components.remove(component)
        
        # Disconnect signals
        self.signalUpdateComponent.disconnect(component.onValueChanged)


    def updateDisplayValue(self, key, value):
        #print "updateDisplayValue", key, value
        for component in self.__components:
            component.onDisplayValueChanged(key, value)

