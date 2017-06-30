#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 15, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QPushButton

from karabo_gui.components import DisplayComponent
import karabo_gui.icons as icons
from .base_item import BaseTreeWidgetItem


class CommandTreeWidgetItem(BaseTreeWidgetItem):
    def __init__(self, command, box, parent, parentItem=None):
        super(CommandTreeWidgetItem, self).__init__(box, parent, parentItem)
        self.setIcon(0, icons.slot)

        # Create empty label for 2nd column (current value on device)
        self.displayComponent = DisplayComponent("DisplayLabel", box,
                                                 self.treeWidget())
        self.treeWidget().setItemWidget(self, 1, self.displayComponent.widget)
        self.treeWidget().resizeColumnToContents(1)

        # Name of command
        self.__command = command
        self.__pbCommand = QPushButton()
        self.__pbCommand.setMinimumHeight(32)
        self.__pbCommand.setEnabled(False)
        self.treeWidget().setItemWidget(self, 0, self.__pbCommand)
        self.__pbCommand.clicked.connect(self.onCommandClicked)

        box.configuration.boxvalue.state.signalUpdateComponent.connect(
            self.updateState)
        self.updateState()

    @property
    def displayText(self):
        return self.__pbCommand.text()

    @displayText.setter
    def displayText(self, text):
        self.__pbCommand.setText(text)

    @property
    def command(self):
        return self.__command

    def destroy(self):
        """Give item subclasses a chance to clean up signal connections"""
        self.box.configuration.boxvalue.state.signalUpdateComponent.disconnect(
            self.updateState)

    def setReadOnly(self, readOnly):
        if readOnly is True:
            self.__pbCommand.setEnabled(False)
        else:
            self.updateState()
        super(CommandTreeWidgetItem, self).setReadOnly(readOnly)

    def onCommandClicked(self):
        self.box.execute()

    def updateState(self):
        enable = self.box.isAllowed() and self.box.isAccessible()
        self.__pbCommand.setEnabled(enable)
