#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QWidget

from .base import BasePanelWidget


class ScriptingPanel(BasePanelWidget):
    def __init__(self):
        super(ScriptingPanel, self).__init__("Console", allow_closing=True)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        return QWidget(self)
