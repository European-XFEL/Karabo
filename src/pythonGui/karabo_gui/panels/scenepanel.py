#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import pyqtSignal, Qt
from PyQt4.QtGui import (QPalette, QScrollArea)

from karabo_gui.docktabwindow import Dockable


class ScenePanel(Dockable, QScrollArea):
    signalClosed = pyqtSignal()

    def __init__(self, scene):
        super(ScenePanel, self).__init__()

        # Reference to underlying scene view
        self.scene = scene
        self.setWidget(self.scene)

        self.setBackgroundRole(QPalette.Dark)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        self.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)
