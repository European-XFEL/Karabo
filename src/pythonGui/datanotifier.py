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
        
        self.__components = set()
        self.addComponent(key, component)


    def addComponent(self, key, component):
        # Connect signals
        self.signalUpdateComponent.connect(component.onValueChanged)
        #self.signalAddKey.connect(widget.onAddKey)
        #self.signalRemoveKey.connect(widget.onRemoveKey)
        
        # Add widget to set and emit signalAddKey to widget
        self.__components.add(component)
        #self.signalAddKey.emit(key)


    def removeComponent(self, key, component):
        # Remove widget from set and emit signalRemoveKey to widget
        if component in self.__components:
            self.__components.remove(component)
        #self.signalRemoveKey.emit(key)
        
        # Disconnect signals
        self.signalUpdateComponent.disconnect(component.onValueChanged)
        #self.signalAddKey.disconnect(widget.onAddKey)
        #self.signalRemoveKey.disconnect(widget.onRemoveKey)


    def updateDisplayValue(self, key, value):
        for component in self.__components:
            component.onDisplayValueChanged(key, value)

