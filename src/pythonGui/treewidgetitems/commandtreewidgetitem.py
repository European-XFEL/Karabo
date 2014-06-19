#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 15, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which inherits from a BaseTreeWidgetItem and represents
   a command item and its parameters."""

__all__ = ["CommandTreeWidgetItem"]


from basetreewidgetitem import BaseTreeWidgetItem
from components import DisplayComponent
import icons
import manager
from network import Network

from PyQt4.QtGui import QPushButton

class CommandTreeWidgetItem(BaseTreeWidgetItem):
    
    def __init__(self, command, box, parent, parentItem=None):
        super(CommandTreeWidgetItem, self).__init__(box, parent, parentItem)
        self.setIcon(0, icons.slot)

        # Create empty label for 2nd column (current value on device)
        self.displayComponent = DisplayComponent("Value Field", self.box, self.treeWidget())
        self.treeWidget().setItemWidget(self, 1, self.displayComponent.widget)
        self.treeWidget().resizeColumnToContents(1)

        # Name of command
        self.__command = command
        self.classAlias = "Command"
        
        self.__pbCommand = QPushButton()
        self.__pbCommand.setMinimumHeight(32)
        self.__pbCommand.setEnabled(False)
        self.treeWidget().setItemWidget(self, 0, self.__pbCommand)
        self.__pbCommand.clicked.connect(self.onCommandClicked)
        self.box.configuration.value.state. \
            signalUpdateComponent.connect(self.onStateChanged)


    def onStateChanged(self):
        self.__pbCommand.setEnabled(self.box.isAllowed())


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
            self.onStateChanged()
        BaseTreeWidgetItem.setReadOnly(self, readOnly)


    def onCommandClicked(self):
        Network().onExecute(self.box)
