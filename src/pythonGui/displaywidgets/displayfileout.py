#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 4, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for parameters
   and is created by the factory class DisplayWidget.
"""

__all__ = ["DisplayFileOut"]


from widget import DisplayWidget

from PyQt4.QtGui import (QHBoxLayout, QIcon, QLineEdit, QToolButton, QWidget)


class DisplayFileOut(DisplayWidget):
    category = "String"
    alias = "File Out"

    def __init__(self, **params):
        super(DisplayFileOut, self).__init__(**params)

        self.__compositeWidget = QWidget()
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0,0,0,0)

        self.__lePath = QLineEdit()
        self.__lePath.setReadOnly(True)
        hLayout.addWidget(self.__lePath)

        text = "Select directory"
        self.__tbPath = QToolButton()
        self.__tbPath.setStatusTip(text)
        self.__tbPath.setToolTip(text)
        self.__tbPath.setIcon(QIcon(":fileout"))
        self.__tbPath.setEnabled(False)
        self.__tbPath.setMaximumSize(25,25)
        hLayout.addWidget(self.__tbPath)


    @property
    def widget(self):
        return self.__compositeWidget


    @property
    def value(self):
        return self.__lePath.text()


    def valueChanged(self, key, value, timestamp=None):
        if value is None:
            return

        if value != self.value:
            self.__lePath.blockSignals(True)
            self.__lePath.setText(value)
            self.__lePath.blockSignals(False)

