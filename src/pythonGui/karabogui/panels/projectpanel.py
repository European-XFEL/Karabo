#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QWidget

from .base import BasePanelWidget


class ProjectPanel(BasePanelWidget):
    """ A dockable panel which contains a view of the project
    """
    def __init__(self):
        super(ProjectPanel, self).__init__("Projects")

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        return QWidget(self)
