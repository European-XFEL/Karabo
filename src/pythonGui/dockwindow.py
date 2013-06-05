#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a normal un/dockable widget."""

__all__ = ["DockWindow"]


from divwidget import DivWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class DockWindow(QWidget):

    def __init__(self, title, parent):
        super(DockWindow, self).__init__()

        self.setParent(parent)
        self.setWindowTitle(title)
        self.vLayout = QVBoxLayout(self)
        self.vLayout.setContentsMargins(0,0,0,0)

#        self.setStyleSheet("QWidget {border-style: solid;"
#                                    "border: 1px solid gray;"
#                                    "border-radius: 3px;"
#                                    "}")

    def setDockableWidget(self, dockWidget, label, icon=None):
        self.divWidget = DivWidget(dockWidget, label, icon)

        self.divWidget.docked.connect(self.onDock)
        self.divWidget.undocked.connect(self.onUndock)

        self.vLayout.addWidget(self.divWidget)

# slots
    def onUndock(self):
        self.vLayout.removeWidget(self.divWidget)
        self.divWidget.setParent(None)
        self.divWidget.move(QCursor.pos())
        self.divWidget.show()
        self.hide()

    def onDock(self):
        self.vLayout.addWidget(self.divWidget);
        self.show()
