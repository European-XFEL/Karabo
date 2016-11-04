#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
import weakref

from PyQt4.QtGui import QAction, QMenu, QStandardItem
from traits.api import Instance, List

from karabo.common.project.api import DeviceInstanceModel, DeviceServerModel
from karabo_gui import icons
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
from .bases import BaseProjectTreeItem
from .device import DeviceInstanceModelItem


class DeviceServerModelItem(BaseProjectTreeItem):
    """ A wrapper for DeviceServerModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceServerModel)
    # Different devices for the server
    children = List(Instance(DeviceInstanceModelItem))

    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)
        edit_action = QAction('Edit', menu)
        dupe_action = QAction('Duplicate', menu)
        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_server,
                                                parent_project))
        add_action = QAction('Add device', menu)
        remove_all_action = QAction('Delete all devices', menu)
        menu.addAction(edit_action)
        menu.addAction(dupe_action)
        menu.addAction(delete_action)
        menu.addSeparator()
        menu.addAction(add_action)
        menu.addAction(remove_all_action)
        return menu

    def create_qt_item(self):
        item = QStandardItem(self.model.server_id)
        item.setData(weakref.ref(self), PROJECT_ITEM_MODEL_REF)
        item.setIcon(icons.yes)
        item.setEditable(False)
        for child in self.children:
            item.appendRow(child.qt_item)

        return item

    def item_handler(self, event):
        """ Called for List-trait events on ``model`` (a DeviceServerModel)

        This notification handler is connected and disconnected in the
        create_device_server_model_shadow and
        destroy_device_server_model_shadow functions.
        """
        def _find_qt_item_index(device_instance_model):
            for i in range(self.qt_item.rowCount()):
                row_child = self.qt_item.child(i)
                row_model = row_child.data(PROJECT_ITEM_MODEL_REF)()
                if row_model.model is device_instance_model:
                    return i
            return -1

        for device_instance_model in event.removed:
            index = _find_qt_item_index(device_instance_model)
            if index >= 0:
                self.qt_item.removeRow(index)

        for item_model in event.added:
            # XXX: TODO
            pass

    # ----------------------------------------------------------------------
    # action handlers

    def _delete_server(self, project):
        """ Remove the macro associated with this item from its project
        """
        server = self.model
        if server in project.servers:
            project.servers.remove(server)
