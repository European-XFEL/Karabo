#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 20, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a placeholder panel to start with."""

__all__ = ["PlaceholderPanel"]


import sys

from PyQt4.QtGui import QLabel, QVBoxLayout, QWidget


class PlaceholderPanel(QWidget):
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
    
    
    def __init__(self, widget=None):
        super(PlaceholderPanel, self).__init__()

        vLayout = QVBoxLayout(self)

        if widget is not None:
            vLayout.addWidget(widget)
        else:
            startup = QLabel()
            vLayout.addWidget(startup)

        #self.setupActions()
        self.setLayout(vLayout)


    def setupToolBars(self, standardToolBar, parent):
        pass


    def setupActions(self):
        pass


# slots
    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass
