#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 18, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QMenu
from traits.api import Instance

from karabo.common.project.api import DeviceConfigurationModel
from .bases import BaseProjectController, ProjectControllerUiData


class DeviceConfigurationController(BaseProjectController):
    """ A controller for DeviceConfigurationModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceConfigurationModel)

    def context_menu(self, project_controller, parent=None):
        return QMenu(parent)

    def create_ui_data(self):
        return ProjectControllerUiData(checkable=True, check_state=Qt.Checked)
