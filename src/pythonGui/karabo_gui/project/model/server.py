#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
import weakref

from PyQt4.QtGui import QAction, QMenu, QStandardItem
from traits.api import Instance, List

from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceServerModel
)
from karabo_gui import icons
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
from .bases import BaseProjectTreeItem


class DeviceServerModelItem(BaseProjectTreeItem):
    """ A wrapper for DeviceServerModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceServerModel)
    # The different device configuration models
    children = List(Instance(DeviceConfigurationModel))

    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)
        edit_action = QAction('Edit', menu)
        dupe_action = QAction('Duplicate', menu)
        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_server,
                                                parent_project))
        menu.addAction(edit_action)
        menu.addAction(dupe_action)
        menu.addAction(delete_action)
        return menu

    def create_qt_item(self):
        item = QStandardItem(self.model.server_id)
        item.setData(weakref.ref(self), PROJECT_ITEM_MODEL_REF)
        item.setIcon(icons.deviceGroupInstance)
        item.setEditable(False)
        for child in self.children:
            item.appendRow(child.qt_item)

        return item

    # ----------------------------------------------------------------------
    # action handlers

    def _delete_server(self, project):
        """ Remove the macro associated with this item from its project
        """
        server = self.model
        if server in project.servers:
            project.servers.remove(server)
