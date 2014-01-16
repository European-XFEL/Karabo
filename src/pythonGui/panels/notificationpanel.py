#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the notification panel on the
   bottom left of the MainWindow which is un/dockable.
"""

__all__ = ["NotificationPanel"]


from manager import Manager
from logwidget import LogWidget

from PyQt4.QtGui import QVBoxLayout, QWidget


class NotificationPanel(QWidget):
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
        super(NotificationPanel, self).__init__()

        self.__logWidget = LogWidget(self, False)
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainLayout.addWidget(self.__logWidget)
        
        Manager().notifier.signalNotificationAvailable.connect(self.onNotificationAvailable)


    def setupActions(self):
        pass
    
    def setupToolBars(self, toolBar, parent):
        pass


    def onNotificationAvailable(self, timestamp, type, shortMessage, detailedMessage, deviceId):
        # Change notification string to logwidget style string
        data = timestamp + " | " + type + " | " + deviceId + " | " + shortMessage + detailedMessage + "#"
        self.__logWidget.addNotificationMessage(data)


    # virtual function
    def onUndock(self):
        pass

    # virtual function
    def onDock(self):
        pass
