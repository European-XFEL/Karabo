#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 4, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for parameters
   and is created by the factory class EditableWidget.
"""

__all__ = ["EditableFileOut"]


from widget import EditableWidget

from PyQt4.QtGui import (QFileDialog, QHBoxLayout, QIcon, QLineEdit, QToolButton,
                         QWidget)


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
        self.__tbPath.setIcon(QIcon(":fileout"))
        self.__tbPath.setMaximumSize(25,25)
        self.__tbPath.clicked.connect(self.onFileOutClicked)
        hLayout.addWidget(self.__tbPath)


    @property
    def widget(self):
        return self.__compositeWidget


    @property
    def value(self):
        return self.__lePath.text()


    def valueChanged(self, key, value, timestamp=None, forceRefresh=False):
        if value is None:
            value = ""

        self.__lePath.blockSignals(True)
        self.__lePath.setText(value)
        self.__lePath.blockSignals(False)


    def onFileOutClicked(self):
        fileOut = QFileDialog.getSaveFileName(None, "Select output file")
        if len(fileOut) < 1:
            return

        self.__lePath.setText(fileOut)

