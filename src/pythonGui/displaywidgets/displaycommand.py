#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 8, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class DisplayWidget.
   
   Each plugin needs to implement the following interface:
   
   def getCategoryAliasClassName():
       pass
   
    class Maker:
        def make(self, **params):
            return Attribute*(**params)
"""

__all__ = ["DisplayCommand"]

from const import ns_karabo
from displaywidgets.icons import Item, SelectionDialog
from widget import DisplayWidget

from PyQt4.QtCore import QSize, pyqtSlot
from PyQt4.QtGui import QToolButton, QWidget, QAction, QStackedLayout, QIcon

from xml.etree.ElementTree import Element


class DisplayCommand(DisplayWidget):
    category = "Slot"
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
        item = Item()
        item.box = box
        item.value = None
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
            descr = item.box.descriptor
            if descr is None or descr.allowedStates is None:
                continue
            item.action.setEnabled(item.box.configuration.value.state in
                                   descr.allowedStates)
        for a in self.button.actions():
            if a.isEnabled():
                self.button.setDefaultAction(a)
                break
        else:
            self.button.setDefaultAction(self.button.actions()[0])


    def save(self, e):
        for item in self.actions:
            if item.url is not None:
                ee = Element(ns_karabo + "action")
                ee.set("key", item.box.key())
                ee.set("image", item.url)
                e.append(ee)

    def load(self, e):
        for ee in e:
            key = ee.get("key")
            for item in self.actions:
                if item.box.key() == key:
                    item.__init__(ee, self.project)
                    break
        self.updateIcons()
