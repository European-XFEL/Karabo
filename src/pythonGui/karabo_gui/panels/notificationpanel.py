#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtGui import QVBoxLayout, QWidget

from karabo_gui.docktabwindow import Dockable
from karabo_gui.logwidget import LogWidget
from karabo_gui.singletons.api import get_manager


class NotificationPanel(Dockable, QWidget):
    def __init__(self):
        super(NotificationPanel, self).__init__()

        self.__logWidget = LogWidget(self, False)

        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5, 5, 5, 5)
        mainLayout.addWidget(self.__logWidget)

        get_manager().signalNotificationAvailable.connect(
            self.__logWidget.onNotificationAvailable)
