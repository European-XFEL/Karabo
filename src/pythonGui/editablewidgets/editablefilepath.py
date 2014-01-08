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


from widget import EditableWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class EditableFilePath(EditableWidget):
    category = "FilePath"
    alias = "File Path"

    def __init__(self, value=None, **params):
        super(EditableFilePath, self).__init__(**params)

        self.__pbSelectPath = QPushButton("...")
        self.__pbSelectPath.setMaximumSize(32,32)
        
        self.__leFilePath = QLineEdit()
        self.__leFilePath.textChanged.connect(self.onEditingFinished)
        self.__pbSelectPath.clicked.connect(self.onSelectFilePath)
        
        self.valueChanged(self.keys[0], value)


    @property
    def widget(self):
        return self.__leFilePath


    @property
    def value(self):
        return self.__leFilePath.text()


    def valueChanged(self, key, value, timestamp=None, forceRefresh=False):
        if value is None:
            value = str()
        
        self.__leFilePath.blockSignals(True)
        self.__leFilePath.setText(value)
        self.__leFilePath.blockSignals(False)        


### slots ###
    def onEditingFinished(self, value):
        self.valueEditingFinished(self.keys[0], value)


    def onSelectFilePath(self):
        filePath = QFileDialog.getExistingDirectory(None, "Set path", "", QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)
        
        if len(filePath) < 1:
            return
        
        self._setValue(filePath)
        self.onEditingFinished()
