#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 9, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


__all__ = ["IPythonWidget"]

# Import the console machinery from ipython
from IPython.qt.console.rich_ipython_widget import RichIPythonWidget
from IPython.qt.inprocess import QtInProcessKernelManager
from IPython.lib import guisupport


class IPythonWidget(RichIPythonWidget):
        
    """
    A convenience class for a live IPython console widget.
    """
    def __init__(self,customBanner=None, *args, **kwargs):
        if customBanner is not None:
            self.banner = customBanner
        
        super(IPythonWidget, self).__init__(*args,**kwargs)
        
        self.kernel_manager = kernel_manager = QtInProcessKernelManager()
        kernel_manager.start_kernel()
        kernel_manager.kernel.gui = 'qt4'
        
        self.kernel_client = kernel_client = self._kernel_manager.client()
        kernel_client.start_channels()

        self.exit_requested.connect(self.stop)


    def stop(self):
        self.kernel_client.stop_channels()
        self.kernel_manager.shutdown_kernel()
        guisupport.get_app_qt4().exit()


    def pushVariables(self,variableDict):
        """ Given a dictionary containing name / value pairs, push those variables to the IPython console widget """
        self.kernel_manager.kernel.shell.push(variableDict)


    def clearConsole(self):
        """
        The terminal gets cleared.
        """
        self._control.clear()


    def printText(self, text):
        """
        The given \text is printed.
        """
        self._append_plain_text(text)


    def executeCommand(self, command, inFrame=False):
        """
        The given \command is executed in the frame of the console or not.
        """
        self._execute(command, inFrame)

