#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the scripting panel of the bottom
   middle of the MainWindow which is un/dockable.
   
   As a dockable widget class used in DivWidget, it needs the following interfaces
   implemented:
   
    def setupActions(self):
        pass
    def setupToolBar(self, toolBar):
        pass
    def onUndock(self):
        pass
    def onDock(self):
        pass
"""

__all__ = ["ScriptingPanel"]

import atexit

from IPython.zmq.ipkernel import IPKernelApp
from IPython.lib.kernel import find_connection_file
from IPython.frontend.qt.kernelmanager import QtKernelManager
from IPython.frontend.qt.console.rich_ipython_widget import RichIPythonWidget
from IPython.utils.traitlets import TraitError

from PyQt4.QtCore import QTimer
from PyQt4.QtGui import QAction, QVBoxLayout, QWidget


class ScriptingPanel(QWidget):


    def __init__(self):
        super(ScriptingPanel, self).__init__()
        
        self.__console = None
        
        self._setupActions()
                
        self.__mainLayout = QVBoxLayout(self)
        self.__mainLayout.setContentsMargins(5,5,5,5)


    def _setupActions(self):
        text = "Start IPython console"
        self.__acStartIPython = QAction("IP[y]:", self)
        self.__acStartIPython.setToolTip(text)
        self.__acStartIPython.setStatusTip(text)
        #self.__acStartIPython.setCheckable(True)
        self.__acStartIPython.triggered.connect(self.onStartIPython)


    def setupToolBar(self, toolBar):
        toolBar.addAction(self.__acStartIPython)


    def _consoleWidget(self, **kwargs):
        """
        Return the embedded IPython console widget.
        """
        kernelApp = self._defaultKernelApp()
        manager = self._defaultKernelManager(kernelApp)
        
        # Include karabo lib into console
        cmd = "from karabo.deviceClient import *\n"
        manager.shell_channel.execute(cmd, False)
        
        widget = self._iPythonWidget(manager)

        # Update namespace                                                           
        kernelApp.shell.user_ns.update(kwargs)
        kernelApp.start()
        
        return widget


    def _defaultKernelApp(self):
        """
        Create and return a default IPython kernel application.
        """
        app = IPKernelApp.instance()
        app.initialize(['--pylab=inline'])
        app.kernel.eventloop = self._eventLoop
        return app


    def _defaultKernelManager(self, kernel):
        """
        Create and return a kernel manager.
        """
        connection_file = find_connection_file(kernel.connection_file)
        manager = QtKernelManager(connection_file=connection_file)
        manager.load_connection_file()
        manager.start_channels()
        atexit.register(manager.cleanup_connection_file)
        return manager


    def _iPythonWidget(self, manager):
        """
        Create and return a IPython console widget.
        """
        try: # Ipython v0.13
            widget = RichIPythonWidget(gui_completion='droplist')
        except TraitError:  # IPython v0.12
            widget = RichIPythonWidget(gui_completion=True)
        widget.kernel_manager = manager
        return widget


    def _eventLoop(self, kernel):
        kernel.timer = QTimer()
        kernel.timer.timeout.connect(kernel.do_one_iteration)
        kernel.timer.start(1000*kernel._poll_interval)


    def onStartIPython(self):#, isChecked):
        if self.__console: return
        
        # Create IPython widget
        self.__console = self._consoleWidget()
        self.__mainLayout.addWidget(self.__console)


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass

