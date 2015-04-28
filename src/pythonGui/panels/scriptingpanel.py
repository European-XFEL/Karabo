#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the scripting panel of the bottom
   middle of the MainWindow which is un/dockable.
"""

__all__ = ["ScriptingPanel"]

from docktabwindow import Dockable
from ipythonwidget import IPythonWidget
from network import network

from PyQt4.QtGui import QAction, QVBoxLayout, QWidget


class ScriptingPanel(Dockable, QWidget):
    def __init__(self):
        super(ScriptingPanel, self).__init__()
        
        self.console = None
        
        self._setupActions()
                
        self.mainLayout = QVBoxLayout(self)
        self.mainLayout.setContentsMargins(5,5,5,5)


    def _setupActions(self):
        text = "Start IPython console"
        self.acStartIPython = QAction("IP[y]:", self)
        self.acStartIPython.setToolTip(text)
        self.acStartIPython.setStatusTip(text)
        #self.acStartIPython.setCheckable(True)
        self.acStartIPython.triggered.connect(self.onStartIPython)


    def setupToolBars(self, toolBar, parent):
        toolBar.addAction(self.acStartIPython)


    def onStartIPython(self, isChecked):
        if self.console:
            self.mainLayout.removeWidget(self.console)
        self.console = IPythonWidget(
            banner="Welcome to the embedded ipython console.\n")
        self.mainLayout.addWidget(self.console)
        network.signalServerConnectionChanged.connect(self.console.stop)
        network.signalServerConnectionChanged.connect(self.stopIPython)
        self.console.exit_requested.connect(self.stopIPython)


    def stopIPython(self):
        if self.console:
            self.mainLayout.removeWidget(self.console)
            self.console.setParent(None)
            self.console = None
