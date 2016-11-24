#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
import weakref

from PyQt4.QtGui import QAction, QDialog, QMenu, QStandardItem
from traits.api import Instance, on_trait_change

from karabo.common.project.api import (DeviceInstanceModel, DeviceServerModel,
                                       find_parent_object)
from karabo_gui import icons
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
from karabo_gui.project.dialog.device_handle import DeviceHandleDialog
from .bases import BaseProjectTreeItem


class DeviceInstanceModelItem(BaseProjectTreeItem):
    """ A wrapper for DeviceInstanceModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceInstanceModel)

    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)

        edit_action = QAction('Edit', menu)
        edit_action.triggered.connect(self._edit_device)
        dupe_action = QAction('Duplicate', menu)
        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_device,
                                                parent_project))
        menu.addAction(edit_action)
        menu.addAction(dupe_action)
        menu.addAction(delete_action)
        return menu

    def create_qt_item(self):
        item = QStandardItem(self.model.instance_id)
        item.setData(weakref.ref(self), PROJECT_ITEM_MODEL_REF)
        item.setIcon(icons.deviceClass)
        item.setEditable(False)
        return item

    @on_trait_change("model.instance_id")
    def instance_id_change(self):
        if not self.is_ui_initialized():
            return
        self.qt_item.setText(self.model.instance_id)

    # ----------------------------------------------------------------------
    # action handlers

    def _delete_device(self, project):
        """ Remove the device associated with this item from its device server
        """
        device = self.model
        server_model = find_parent_object(device, project,
                                          DeviceServerModel)
        if device in server_model.devices:
            server_model.devices.remove(device)

    def _edit_device(self):
        dialog = DeviceHandleDialog(self.model)
        result = dialog.exec()
        if result == QDialog.Accepted:
            self.model.instance_id = dialog.instance_id
            self.model.if_exists = dialog.if_exists
            self.model.description = dialog.description
