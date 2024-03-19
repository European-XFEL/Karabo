#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 20, 2014
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
from qtpy.QtWidgets import QLabel

from .base import BasePanelWidget


class PlaceholderPanel(BasePanelWidget):
    def __init__(self):
        super().__init__("Start Page")
        # Logbook is not needed for this panel.
        self.application_toolbar.setVisible(False)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        return QLabel(self)

    def attach_to_container(self, container):
        """We override this method to make sure the standard toolbar stays
        hidden for this panel.
        """
        super().attach_to_container(container)
        self.standard_toolbar.setVisible(False)
