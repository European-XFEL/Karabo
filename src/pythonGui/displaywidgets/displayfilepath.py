#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March2, 2012
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

__all__ = ["DisplayFilePath"]


from widget import DisplayWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class DisplayFilePath(DisplayWidget):
    category = "FilePath"
    alias = "File Path"

    def __init__(self, **params):
        super(DisplayFilePath, self).__init__(**params)

        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>

        self.__leFilePath = QLineEdit()
        self.__leFilePath.setReadOnly(True)

        self.__pbSelectPath = QPushButton("...")
        self.__pbSelectPath.setMaximumSize(32,32)
        self.__pbSelectPath.setReadOnly(True)
        
        self.__key = params.get('key')


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__leFilePath
    widget = property(fget=_getWidget)


    # Returns a tuple of min and max number of associated keys with this component
    def _getMinMaxAssociatedKeys(self):
        return self.__minMaxAssociatedKeys
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        return [self.__key]
    keys = property(fget=_getKeys)


    def _value(self):
        return str(self.__leFilePath.text())
    value = property(fget=_value)


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


    def valueChanged(self, key, value, timestamp=None):
        if value is None:
            return
        
        if value != self.value:
            self.__leFilePath.blockSignals(True)
            self.__leFilePath.setText(value)
            self.__leFilePath.blockSignals(False)


### slots ###
    def onSelectFilePath(self):
        filePath = QFileDialog.getExistingDirectory(None, "Set path", "", QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)
        if len(filePath) < 1:
            return
        self._setValue(filePath)
        self.onEditingFinished()
