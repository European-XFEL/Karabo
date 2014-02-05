#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 4, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for parameters
   and is created by the factory class EditableWidget.
"""

__all__ = ["EditableFileIn"]


from widget import EditableWidget

from PyQt4.QtGui import (QFileDialog, QHBoxLayout, QIcon, QLineEdit, QToolButton,
                         QWidget)


class EditableFileIn(EditableWidget):
    category = "String"
    alias = "File In"

    def __init__(self, value=None, **params):
        super(EditableFileIn, self).__init__(**params)

        self.__compositeWidget = QWidget()
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0,0,0,0)

        self.__lePath = QLineEdit()
        self.__lePath.textChanged.connect(self.onEditingFinished)
        hLayout.addWidget(self.__lePath)

        text = "Select input file"
        self.__tbPath = QToolButton()
        self.__tbPath.setStatusTip(text)
        self.__tbPath.setToolTip(text)
        self.__tbPath.setIcon(QIcon(":filein"))
        self.__tbPath.setMaximumSize(25,25)
        self.__tbPath.clicked.connect(self.onFileInClicked)
        hLayout.addWidget(self.__tbPath)

        self.valueChanged(self.keys[0], value)


    @property
    def widget(self):
        return self.__compositeWidget


    @property
    def value(self):
        return self.__lePath.text()


    def valueChanged(self, key, value, timestamp=None, forceRefresh=False):
        if value is None:
            value = str()

        self.__lePath.blockSignals(True)
        self.__lePath.setText(value)
        self.__lePath.blockSignals(False)


### slots ###
    def onEditingFinished(self, value):
        self.valueEditingFinished(self.keys[0], value)


    def onFileInClicked(self):
        fileIn = QFileDialog.getOpenFileName(None, "Select input file")
        if len(fileIn) < 1:
            return

        self.__lePath.setText(fileIn)

