#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
import weakref

from PyQt4.QtGui import QAction, QDialog, QMenu, QStandardItem
from traits.api import Callable, Dict, Instance, List, on_trait_change

from karabo.common.project.api import DeviceInstanceModel, DeviceServerModel
from karabo_gui import icons
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
from karabo_gui.project.dialog.device_handle import DeviceHandleDialog
from karabo_gui.project.dialog.server_handle import ServerHandleDialog
from .bases import BaseProjectTreeItem
from .device import DeviceInstanceModelItem


class DeviceServerModelItem(BaseProjectTreeItem):
    """ A wrapper for DeviceServerModel objects
    """
    # Redefine model with the correct type
    model = Instance(DeviceServerModel)

    # A factory for shadow items wrapping children
    child_create = Callable

    # A callable which can gracefully destroy a child shadow object
    child_destroy = Callable

    # Different devices for the server
    children = List(Instance(DeviceInstanceModelItem))
    _child_map = Dict  # dictionary for fast lookups during removal

    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)
        edit_action = QAction('Edit', menu)
        edit_action.triggered.connect(self._edit_server)
        dupe_action = QAction('Duplicate', menu)
        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_server,
                                                parent_project))
        add_action = QAction('Add device', menu)
        add_action.triggered.connect(self._add_device)
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
        removals = []
        for model in event.removed:
            item_model = self._child_map[model]
            self.children.remove(item_model)
            self.child_destroy(item_model)
            removals.append(item_model)

        additions = [self.child_create(model=model) for model in event.added]
        self.children.extend(additions)

        # Synchronize the GUI with the Traits model
        self._update_ui_children(additions, removals)

    def _children_items_changed(self, event):
        """ Maintain ``_child_map`` by watching item events on ``children``

        This is a static notification handler which is connected automatically
        by Traits.
        """
        for item_model in event.removed:
            del self._child_map[item_model.model]

        for item_model in event.added:
            self._child_map[item_model.model] = item_model

    def _update_ui_children(self, additions, removals):
        """ Propagate changes from the Traits model to the Qt item model.
        """
        def _find_child_qt_item(item_model):
            for i in range(self.qt_item.rowCount()):
                row_child = self.qt_item.child(i)
                row_model = row_child.data(PROJECT_ITEM_MODEL_REF)()
                if row_model is item_model:
                    return i
            return -1

        # Stop immediately if the UI is not yet initialized
        if not self.is_ui_initialized():
            return

        for item in removals:
            index = _find_child_qt_item(item)
            if index >= 0:
                self.qt_item.removeRow(index)

        for item in additions:
            self.qt_item.appendRow(item.qt_item)

    @on_trait_change("model.server_id")
    def server_id_change(self):
        if not self.is_ui_initialized():
            return
        self.qt_item.setText(self.model.server_id)

    # ----------------------------------------------------------------------
    # action handlers

    def _delete_server(self, project):
        """ Remove the macro associated with this item from its project
        """
        server = self.model
        if server in project.servers:
            project.servers.remove(server)

    def _edit_server(self):
        dialog = ServerHandleDialog(self.model)
        result = dialog.exec()
        if result == QDialog.Accepted:
            self.model.server_id = dialog.server_id
            self.model.host = dialog.host
            self.model.description = dialog.description

    def _add_device(self):
        """ Add a device to this server
        """
        dialog = DeviceHandleDialog()
        result = dialog.exec()
        if result == QDialog.Accepted:
            traits = {
                'instance_id': dialog.instance_id,
                'if_exists': dialog.if_exists,
                'description': dialog.description
            }
            device = DeviceInstanceModel(**traits)
            self.model.devices.append(device)
