#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 20, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QLabel

from .base import BasePanelWidget


class PlaceholderPanel(BasePanelWidget):
    def __init__(self, container, title):
        super(PlaceholderPanel, self).__init__(container, title)
        self.doesDockOnClose = False

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        return QLabel(self)
