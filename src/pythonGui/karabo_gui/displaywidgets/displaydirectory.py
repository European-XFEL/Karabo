#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 4, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for parameters
   and is created by the factory class DisplayWidget.
"""



__all__ = ["DisplayDirectory"]


from karabo_gui.util import SignalBlocker
import karabo_gui.icons as icons
from karabo_gui.widget import DisplayWidget

from karabo.middlelayer import String

from PyQt4.QtGui import (QHBoxLayout, QLineEdit, QToolButton, QWidget)


class DisplayDirectory(DisplayWidget):
    category = String
    displayType = "directory"
    alias = "Directory"

    def __init__(self, box, parent):
        super(DisplayDirectory, self).__init__(box)
        
        self.__compositeWidget = QWidget(parent)
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0,0,0,0)

        self.__lePath = QLineEdit()
        self.__lePath.setReadOnly(True)
        hLayout.addWidget(self.__lePath)
        
        text = "Select directory"
        self.__tbPath = QToolButton()
        self.__tbPath.setStatusTip(text)
        self.__tbPath.setToolTip(text)
        self.__tbPath.setIcon(icons.open)
        self.__tbPath.setEnabled(False)
        self.__tbPath.setMaximumSize(25,25)
        hLayout.addWidget(self.__tbPath)


    @property
    def widget(self):
        return self.__compositeWidget


    @property
    def value(self):
        return self.__lePath.text()


    def valueChanged(self, box, value, timestamp=None):
        if value is None:
            return

        if value != self.value:
            with SignalBlocker(self.__lePath):
                self.__lePath.setText(value)

