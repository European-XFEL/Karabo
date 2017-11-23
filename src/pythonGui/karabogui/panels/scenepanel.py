#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QWidget

from .base import BasePanelWidget


class ScenePanel(BasePanelWidget):
    def __init__(self, model, connected_to_server):
        # cache a reference to the scene model
        self.model = model
        super(ScenePanel, self).__init__(model.simple_name, allow_closing=True)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        return QWidget(self)
