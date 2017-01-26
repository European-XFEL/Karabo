#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 18, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
import weakref

from PyQt4.QtGui import QAction, QMenu, QStandardItem
from traits.api import Instance, on_trait_change

from karabo.common.project.api import DeviceConfigurationModel
from karabo_gui.const import PROJECT_CONTROLLER_REF
from karabo_gui.project.utils import save_object
from .bases import BaseProjectController


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

    def create_qt_item(self):
        item = QStandardItem()
        item.setData(weakref.ref(self), PROJECT_CONTROLLER_REF)
        item.setEditable(False)
        item.setCheckable(True)
        # Currently disable changing CheckState
        item.setEnabled(False)
        self.set_qt_item_text(item, self.model.simple_name)
        return item

    @on_trait_change("model.modified")
    def update_ui_label(self):
        """ Whenever the configuration is modified it should be visible to the
        user
        """
        if not self.is_ui_initialized():
            return
        self.set_qt_item_text(self.qt_item, self.model.simple_name)
