#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtGui import QAction, QVBoxLayout, QWidget

from karabo_gui.ipythonwidget import IPythonWidget
from karabo_gui.singletons.api import get_network
from karabo_gui.toolbar import ToolBar
from .base import BasePanelWidget


class ScriptingPanel(BasePanelWidget):
    def __init__(self):
        super(ScriptingPanel, self).__init__("Console")

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget()

        self.console = None
        self.mainLayout = QVBoxLayout(widget)
        self.mainLayout.setContentsMargins(5, 5, 5, 5)

        return widget

    def toolbars(self):
        """This should create and return one or more `ToolBar` instances needed
        by this panel.
        """
        toolbar = ToolBar(parent=self)

        text = "Start IPython console"
        self.acStartIPython = QAction("IP[y]:", toolbar)
        self.acStartIPython.setToolTip(text)
        self.acStartIPython.setStatusTip(text)
        self.acStartIPython.triggered.connect(self.onStartIPython)
        toolbar.addAction(self.acStartIPython)

        return [toolbar]

    def onStartIPython(self, isChecked):
        if self.console:
            self.mainLayout.removeWidget(self.console)
        self.console = IPythonWidget(
            banner="Welcome to the embedded ipython console.\n")
        self.mainLayout.addWidget(self.console)
        network = get_network()
        network.signalServerConnectionChanged.connect(self.console.stop)
        network.signalServerConnectionChanged.connect(self.stopIPython)
        self.console.exit_requested.connect(self.stopIPython)

    def stopIPython(self):
        if self.console:
            self.mainLayout.removeWidget(self.console)
            self.console.setParent(None)
            self.console = None
