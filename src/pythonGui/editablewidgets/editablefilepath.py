#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class EditableWidget.
   
   Each plugin needs to implement the following interface:
   
   def getCategoryAliasClassName():
       pass
   
    class Maker:
        def make(self, **params):
            return Attribute*(**params)
"""

__all__ = ["EditableFilePath"]


from editablewidget import EditableWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


def getCategoryAliasClassName():
    return ["FilePath","File Path","EditableFilePath"]


class EditableFilePath(EditableWidget):
    
    def __init__(self, **params):
        super(EditableFilePath, self).__init__(**params)

        self.__leFilePath = QLineEdit()

        self.__pbSelectPath = QPushButton("...")
        self.__pbSelectPath.setMaximumSize(32,32)
        
        self.__leFilePath.textChanged.connect(self.onEditingFinished)
        self.__pbSelectPath.clicked.connect(self.onSelectFilePath)
        
        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        # Set key
        self.__key = params.get(QString('key'))
        if self.__key is None:
            self.__key = params.get('key')
        # Set value
        value = params.get(QString('value'))
        if value is None:
            value = params.get('value')
        self.valueChanged(self.__key, value)


    def _getCategory(self):
        category, alias, className = getCategoryAliasClassName()
        return category
    category = property(fget=_getCategory)


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


    def addParameters(self, **params):
        print "addParameters", params


    def _value(self):
        return str(self.__leFilePath.text())
    value = property(fget=_value)


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


    def valueChanged(self, key, value, timestamp=None, forceRefresh=False):
        if (value is None) or (self.__leFilePath.isEnabled() == False):
            return
        
        self.__leFilePath.blockSignals(True)
        self.__leFilePath.setText(value)
        self.__leFilePath.blockSignals(False)        


### slots ###
    def onEditingFinished(self, value):
        self.valueEditingFinished(self.__key, value)


    def onSelectFilePath(self):
        filePath = QFileDialog.getExistingDirectory(None, "Set path", "", QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)
        
        if filePath.isEmpty():
            return
        
        self._setValue(filePath)
        self.onEditingFinished()


    class Maker:
        def make(self, **params):
            return EditableFilePath(**params)

