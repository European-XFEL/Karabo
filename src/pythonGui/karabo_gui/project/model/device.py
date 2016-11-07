#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
import weakref

from PyQt4.QtGui import QAction, QMenu, QStandardItem
from traits.api import Instance

from karabo.common.project.api import DeviceInstanceModel, find_parent_object
from karabo_gui import icons
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
from .bases import BaseProjectTreeItem


class DeviceInstanceModelItem(BaseProjectTreeItem):
    """ A wrapper for DeviceInstanceModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceInstanceModel)

    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)

        edit_action = QAction('Edit', menu)
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

    # ----------------------------------------------------------------------
    # action handlers

    def _delete_device(self, project):
        """ Remove the device associated with this item from its device server
        """
        device = self.model
        server_model = find_parent_object(self.model, project)
        if device in server_model.devices:
            server_model.devices.remove(device)
