#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 15, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which inherits from a BaseTreeWidgetItem and represents
   a command item and its parameters."""

__all__ = ["CommandTreeWidgetItem"]


from basetreewidgetitem import BaseTreeWidgetItem
from displaycomponent import DisplayComponent
from manager import Manager

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class CommandTreeWidgetItem(BaseTreeWidgetItem):
    
    def __init__(self, command, path, parent, parentItem=None):
        super(CommandTreeWidgetItem, self).__init__(path, parent, parentItem)
        
        self.setIcon(0, QIcon(":slot"))
        
        # Create empty label for 2nd column (current value on device)
        self.displayComponent = DisplayComponent("Value Field", key=self.internalKey)
        self.treeWidget().setItemWidget(self, 1, self.displayComponent.widget)
        self.treeWidget().resizeColumnToContents(1)

        # Name of command
        self.__command = command
        
        self.__isCommandEnabled = False
        self.__pbCommand = QPushButton()
        self.__pbCommand.setMinimumHeight(32)
        self.__pbCommand.setEnabled(self.__isCommandEnabled)
        self.treeWidget().setItemWidget(self, 0, self.__pbCommand)
        self.__pbCommand.clicked.connect(self.onCommandClicked)


### getter & setter functions ###
    def _getEnabled(self):
        return self.__isCommandEnabled
    def _setEnabled(self, enabled):
        self.__pbCommand.setEnabled(enabled)
        self.__isCommandEnabled = enabled
    enabled = property(fget=_getEnabled, fset=_setEnabled)


    def _getText(self):
        return self.__pbCommand.text()
    def _setText(self, text):
        self.__pbCommand.setText(text)
    displayText = property(fget=_getText, fset=_setText)


    def _getCommand(self):
        return self.__command
    command = property(fget=_getCommand)


### public functions ###
    def setReadOnly(self, readOnly):
        if readOnly is True:
            self.__pbCommand.setEnabled(False)
        else:
            self.__pbCommand.setEnabled(self.__isCommandEnabled)
        BaseTreeWidgetItem.setReadOnly(self, readOnly)


### slots ###
    def onCommandClicked(self):
        args = [] # TODO slot arguments
        Manager().executeCommand(dict(path=self.internalKey, command=self.__command, args=args))
