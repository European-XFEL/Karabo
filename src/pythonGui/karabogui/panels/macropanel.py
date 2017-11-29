#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created in June 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QWidget

from .base import BasePanelWidget


class MacroPanel(BasePanelWidget):
    def __init__(self, model):
        self.model = model
        super(MacroPanel, self).__init__(model.simple_name, allow_closing=True)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        return QWidget(self)
