#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 23, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a dialog to set the properties
   for a Text component of the middle panel. """

__all__ = ["RectangleDialog"]


from PyQt4.QtCore import *
from PyQt4.QtGui import *


class RectangleDialog(QDialog):

    def __init__(self, parent, rectItem=None):
        super(RectangleDialog, self).__init__(parent)
        
        self.__pbBackgroundColor = QPushButton()
        self.__pbBackgroundColor.setMaximumSize(32,32)
        
        self.__pbOutlineColor = QPushButton()
        self.__pbOutlineColor.setMaximumSize(32,32)
        
        formLayout = QFormLayout()
        formLayout.addRow("&Background color:", self.__pbBackgroundColor)
        formLayout.addRow("&Outline color:", self.__pbOutlineColor)
        
        self.__buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.__buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        if rectItem:
            self.__textBackgroundColor = rectItem.backgroundColor()
            self.__textOutlineColor = rectItem.outlineColor()
        else:
            self.__textBackgroundColor = QColor(Qt.lightGray)
            self.__textOutlineColor = QColor(Qt.darkGray)
        
        # Set colors to buttons
        self.updateColorButtons()
        
        layout = QVBoxLayout()
        layout.addLayout(formLayout)
        layout.addWidget(self.__buttonBox)
        self.setLayout(layout)

        # Signals & Slots connections
        self.__pbBackgroundColor.clicked.connect(self.onBackgroundColorClicked)
        self.__pbOutlineColor.clicked.connect(self.onOutlineColorClicked)
        
        self.__buttonBox.accepted.connect(self.onOkClicked)
        self.__buttonBox.rejected.connect(self.reject)

        self.setWindowTitle("Edit rectangle")


    def updateColorButtons(self):
        self.__pbBackgroundColor.setStyleSheet(QString("background-color : rgb(%1,%2,%3);").arg(self.__textBackgroundColor.red()) \
                                                                                           .arg(self.__textBackgroundColor.green()) \
                                                                                           .arg(self.__textBackgroundColor.blue()))

        self.__pbOutlineColor.setStyleSheet(QString("background-color : rgb(%1,%2,%3);").arg(self.__textOutlineColor.red()) \
                                                                                        .arg(self.__textOutlineColor.green()) \
                                                                                        .arg(self.__textOutlineColor.blue()))


    def backgroundColor(self):
        return self.__textBackgroundColor


    def outlineColor(self):
        return self.__textOutlineColor


### slots ###
    def onBackgroundColorClicked(self):
        color = QColorDialog.getColor(self.__textBackgroundColor, self)
        if color.isValid():
            self.__textBackgroundColor = color
            self.updateColorButtons()
            self.__buttonBox.button(QDialogButtonBox.Ok).setEnabled(True)


    def onOutlineColorClicked(self):
        color = QColorDialog.getColor(self.__textOutlineColor, self)
        if color.isValid():
            self.__textOutlineColor = color
            self.updateColorButtons()
            self.__buttonBox.button(QDialogButtonBox.Ok).setEnabled(True)


    def onOkClicked(self):
        self.accept()

