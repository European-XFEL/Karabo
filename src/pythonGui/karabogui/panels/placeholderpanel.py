#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 20, 2014
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
from qtpy.QtWidgets import QLabel

from .base import BasePanelWidget


class PlaceholderPanel(BasePanelWidget):
    def __init__(self):
        super(PlaceholderPanel, self).__init__("Start Page")

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        return QLabel(self)

    def attach_to_container(self, container):
        """We override this method to make sure the standard toolbar stays
        hidden for this panel.
        """
        super(PlaceholderPanel, self).attach_to_container(container)
        self.standard_toolbar.setVisible(False)
