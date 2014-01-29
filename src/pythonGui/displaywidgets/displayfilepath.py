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

        self.__leFilePath = QLineEdit()
        self.__leFilePath.setReadOnly(True)

        self.__pbSelectPath = QPushButton("...")
        self.__pbSelectPath.setMaximumSize(32,32)
        self.__pbSelectPath.setReadOnly(True)
        

    @property
    def widget(self):
        return self.__leFilePath


    @property
    def value(self):
        return self.__leFilePath.text()


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
