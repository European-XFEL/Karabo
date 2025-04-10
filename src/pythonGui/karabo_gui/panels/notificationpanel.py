#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the notification panel on the
   bottom left of the MainWindow which is un/dockable.
"""

__all__ = ["NotificationPanel"]


from karabo_gui.docktabwindow import Dockable
from karabo_gui.logwidget import LogWidget
from karabo_gui.topology import Manager

from PyQt4.QtGui import QVBoxLayout, QWidget


class NotificationPanel(Dockable, QWidget):
    def __init__(self):
        super(NotificationPanel, self).__init__()

        self.__logWidget = LogWidget(self, False)
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainLayout.addWidget(self.__logWidget)
        
        Manager().signalNotificationAvailable.connect(
            self.__logWidget.onNotificationAvailable)
