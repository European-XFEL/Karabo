#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QWidget

from .base import BasePanelWidget


class ConfigurationPanel(BasePanelWidget):
    def __init__(self):
        super(ConfigurationPanel, self).__init__("Configuration Editor")

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        return QWidget(self)
