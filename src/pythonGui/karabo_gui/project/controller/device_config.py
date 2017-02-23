#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 18, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QAction, QMenu
from traits.api import Instance

from karabo.common.project.api import DeviceConfigurationModel
from karabo_gui.project.utils import save_object
from .bases import BaseProjectController, ProjectControllerUiData


class DeviceConfigurationController(BaseProjectController):
    """ A controller for DeviceConfigurationModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceConfigurationModel)

    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)
        save_action = QAction('Save', menu)
        save_action.triggered.connect(partial(save_object, self.model))
        menu.addAction(save_action)
        return menu

    def create_ui_data(self):
        return ProjectControllerUiData(checkable=True, check_state=Qt.Checked)
