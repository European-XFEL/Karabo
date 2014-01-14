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
from PyQt4.QtGui import QVBoxLayout, QWidget


class ScriptingPanel(QWidget):


    def __init__(self):
        super(ScriptingPanel, self).__init__()
        
        # Create IPython widget
        self.__console = self._consoleWidget()
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainLayout.addWidget(self.__console)


    def _consoleWidget(self, **kwargs):
        """
        Returns the widget with embedded IPython console.
        """
        kernel_app = self._default_kernel_app()
        manager = self._default_manager(kernel_app)
        widget = self._console_widget(manager)

        # Update namespace                                                           
        kernel_app.shell.user_ns.update(kwargs)

        kernel_app.start()
        return widget


    def _default_kernel_app(self):
        app = IPKernelApp.instance()
        app.initialize(['python', '--pylab=qt'])
        app.kernel.eventloop = self._event_loop
        return app


    def _default_manager(self, kernel):
        connection_file = find_connection_file(kernel.connection_file)
        manager = QtKernelManager(connection_file=connection_file)
        manager.load_connection_file()
        manager.start_channels()
        atexit.register(manager.cleanup_connection_file)
        return manager


    def _console_widget(self, manager):
        try: # Ipython v0.13
            widget = RichIPythonWidget(gui_completion='droplist')
        except TraitError:  # IPython v0.12
            widget = RichIPythonWidget(gui_completion=True)
        widget.kernel_manager = manager
        return widget


    def _event_loop(self, kernel):
        kernel.timer = QTimer()
        kernel.timer.timeout.connect(kernel.do_one_iteration)
        kernel.timer.start(1000*kernel._poll_interval)


    def setupActions(self):
        pass


    def setupToolBar(self, toolBar):
        pass


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass

