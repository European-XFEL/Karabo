#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the scripting panel of the bottom
   middle of the MainWindow which is un/dockable.
"""

__all__ = ["ScriptingPanel"]

import atexit

from ipythonwidget import IPythonWidget

from PyQt4.QtGui import QAction, QVBoxLayout, QWidget


class ScriptingPanel(QWidget):
    ##########################################
    # Dockable widget class used in DivWidget
    # Requires following interface:
    #
    #def setupActions(self):
    #    pass
    #def setupToolBars(self, standardToolBar, parent):
    #    pass
    #def onUndock(self):
    #    pass
    #def onDock(self):
    #    pass
    ##########################################


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
        
        # Create IPython widget
        self.console = IPythonWidget(customBanner="Welcome to the embedded ipython console.\n")
        self.console.executeCommand("from karabo import *\n", True)
        self.console.printText("The karabo device client is available.")       

        self.mainLayout.addWidget(self.console)


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass

