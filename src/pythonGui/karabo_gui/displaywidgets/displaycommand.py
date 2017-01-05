#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 8, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import QSize, pyqtSlot
from PyQt4.QtGui import QToolButton, QWidget, QAction, QStackedLayout, QIcon

from karabo_gui.displaywidgets.icons import Item, SelectionDialog
from karabo_gui.widget import DisplayWidget
from karabo_gui.schema import SlotNode


class DisplayCommand(DisplayWidget):
    category = SlotNode
    alias = "Command"

    def __init__(self, box, parent):
        super(DisplayCommand, self).__init__(None)
        self.widget = QWidget(parent)
        layout = QStackedLayout(self.widget)
        self.button = QToolButton(self.widget)
        self.button.setIconSize(QSize(1000, 1000))
        layout.addWidget(self.button)
        action = QAction("Change Icons...", self.widget)
        self.widget.addAction(action)
        action.triggered.connect(self.showDialog)
        self.current = None
        self.actions = []
        self.addBox(box)

    def showDialog(self):
        dialog = SelectionDialog(self.project, self.actions, None)
        self.actions = dialog.exec()
        self.updateIcons()

    def updateIcons(self):
        for a in self.actions:
            if a.pixmap is not None:
                a.action.setIcon(QIcon(a.pixmap))

    def addBox(self, box):
        action = QAction("NO TEXT", self.button)
        self.button.addAction(action)
        item = Item(None)  # XXX: needs to be checked
        item.box = box
        item.action = action
        self.actions.append(item)
        box.configuration.boxvalue.state.signalUpdateComponent.connect(
            self.update)
        self.update()
        return True

    @property
    def boxes(self):
        return [a.box for a in self.actions]

    def typeChanged(self, box):
        for item in self.actions:
            if item.box is box and item.value is None:
                item.action.triggered.disconnect()
                item.action.triggered.connect(box.execute)
                item.action.setText(box.descriptor.displayedName)
                item.value = box.descriptor.displayedName

    @pyqtSlot()
    def update(self):
        for item in self.actions:
            item.action.setEnabled(item.box.isAllowed() and
                                   item.box.isAccessible())
        for a in self.button.actions():
            if a.isEnabled():
                self.button.setDefaultAction(a)
                break
        else:
            self.button.setDefaultAction(self.button.actions()[0])
