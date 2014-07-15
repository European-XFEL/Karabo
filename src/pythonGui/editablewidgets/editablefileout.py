#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 4, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for parameters
   and is created by the factory class EditableWidget.
"""

__all__ = ["EditableFileOut"]

import icons
from util import getSaveFileName
from util import SignalBlocker
from widget import EditableWidget

from PyQt4.QtGui import (QHBoxLayout, QLineEdit, QToolButton, QWidget)


class EditableFileOut(EditableWidget):
    category = "String"
    alias = "File Out"

    def __init__(self, box, parent):
        super(EditableFileOut, self).__init__(box)
        
        self.__compositeWidget = QWidget(parent)
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0,0,0,0)

        self.__lePath = QLineEdit()
        self.__lePath.textChanged.connect(self.onEditingFinished)
        hLayout.addWidget(self.__lePath)
        
        text = "Select output file"
        self.__tbPath = QToolButton()
        self.__tbPath.setStatusTip(text)
        self.__tbPath.setToolTip(text)
        self.__tbPath.setIcon(icons.fileout)
        self.__tbPath.setMaximumSize(25,25)
        self.__tbPath.clicked.connect(self.onFileOutClicked)
        hLayout.addWidget(self.__tbPath)


    @property
    def widget(self):
        return self.__compositeWidget


    @property
    def value(self):
        return self.__lePath.text()


    def valueChanged(self, box, value, timestamp=None, forceRefresh=False):
        if value is None:
            value = ""

        with SignalBlocker(self.__lePath):
            self.__lePath.setText(value)


    def onFileOutClicked(self):
        filename = getSaveFileName("Select output file")
        if not filename:
            return

        self.__lePath.setText(filename)

