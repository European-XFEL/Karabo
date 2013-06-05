#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 4, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a placeholder widget to start with."""

__all__ = ["PlaceholderWidget"]


import sys

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class PlaceholderWidget(QWidget):

    def __init__(self, widget=None):
        super(PlaceholderWidget, self).__init__()

        vLayout = QVBoxLayout(self)

        if widget is not None :
            vLayout.addWidget(widget)
        else :
            textEdit = QTextEdit("Placeholder")
            textEdit.setReadOnly(True)
            vLayout.addWidget(textEdit)

        self.setupActions()
        self.setLayout(vLayout)


    def setupToolBar(self, toolBar):
        pass


    def setupActions(self):
        pass


# slots
    def onFileSave(self):
        print "PlaceholderWidget onFileSave()"


    def onFileSaveAs(self):
        print "PlaceholderWidget onFileSaveAs()"


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass

