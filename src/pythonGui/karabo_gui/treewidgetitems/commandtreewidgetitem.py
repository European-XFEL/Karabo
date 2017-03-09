#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 15, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QPushButton

import karabo_gui.icons as icons
from karabo_gui.components import DisplayComponent

from .basetreewidgetitem import BaseTreeWidgetItem


class CommandTreeWidgetItem(BaseTreeWidgetItem):
    def __init__(self, command, box, parent, parentItem=None):
        super(CommandTreeWidgetItem, self).__init__(box, parent, parentItem)
        self.setIcon(0, icons.slot)

        # Create empty label for 2nd column (current value on device)
        self.displayComponent = DisplayComponent("DisplayLabel", self.box,
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
        self.box.configuration.boxvalue.state.signalUpdateComponent.connect(
            self.onStateChanged)
        self.onStateChanged()

    def onStateChanged(self):
        enable = self.box.isAllowed() and self.box.isAccessible()
        self.__pbCommand.setEnabled(enable)

    def _getText(self):
        return self.__pbCommand.text()

    def _setText(self, text):
        self.__pbCommand.setText(text)
    displayText = property(fget=_getText, fset=_setText)

    def _getCommand(self):
        return self.__command
    command = property(fget=_getCommand)

    def setReadOnly(self, readOnly):
        if readOnly is True:
            self.__pbCommand.setEnabled(False)
        else:
            self.onStateChanged()
        BaseTreeWidgetItem.setReadOnly(self, readOnly)

    def onCommandClicked(self):
        self.box.execute()
