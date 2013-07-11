#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on FJuly 11, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the project panel on the
   middle left of the MainWindow which is un/dockable.
   
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

__all__ = ["ProjectPanel"]


from PyQt4.QtCore import *
from PyQt4.QtGui import *

class ProjectPanel(QWidget):
    ##########################################
    # Dockable widget class used in DivWidget
    # Requires following interface:
    # 
    #def setupActions(self):
    #    pass
    #def setupToolBar(self, toolBar):
    #    pass
    #def onUndock(self):
    #    pass
    #def onDock(self):
    #    pass
    ##########################################
    
    def __init__(self):
        super(ProjectPanel, self).__init__()


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


