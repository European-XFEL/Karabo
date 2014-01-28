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
    signalUpdateComponent = pyqtSignal(str, object) # internalKey, value, timestamp (TODO)
    signalUpdateDisplayValue = pyqtSignal(str, object)


    def __init__(self, key, component):
        super(DataNotifier, self).__init__()
        
        self.signalUpdateComponent.connect(self.onValueChanged)
        self.addComponent(key, component)


    def onValueChanged(self, key, value):
        self.value = value


    def addComponent(self, key, component):
        self.signalUpdateComponent.connect(component.onValueChanged)
        self.signalUpdateDisplayValue.connect(component.onDisplayValueChanged)
        if hasattr(self, "value"):
            self.signalUpdateComponent.emit(key, self.value)


    def removeComponent(self, key, component):
        pass

    
    def removeComponents(self, key):
        pass


    def updateDisplayValue(self, key, value):
        self.signalUpdateDisplayValue.emit(key, value)
