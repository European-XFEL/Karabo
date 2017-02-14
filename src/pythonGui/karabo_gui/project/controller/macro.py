#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
from io import StringIO
import weakref

from PyQt4.QtGui import QAction, QDialog, QMenu, QStandardItem
from traits.api import Instance, String, on_trait_change

from karabo.common.project.api import MacroModel, read_macro, write_macro
from karabo_gui import icons
from karabo_gui.const import PROJECT_CONTROLLER_REF
from karabo_gui.events import (
    broadcast_event, register_for_broadcasts, unregister_from_broadcasts,
    KaraboBroadcastEvent, KaraboEventSender)
from karabo_gui.indicators import DeviceStatus, get_project_device_status_icon
from karabo_gui.project.dialog.macro_handle import MacroHandleDialog
from karabo_gui.project.dialog.object_handle import ObjectDuplicateDialog
from karabo_gui.project.topo_listener import SystemTopologyListener
from karabo_gui.project.utils import save_object, run_macro
from karabo_gui.singletons.api import get_manager, get_topology
from karabo_gui.util import getSaveFileName
from .bases import BaseProjectGroupController, BaseProjectController


class MacroInstanceController(BaseProjectController):
    """ A controller for running macro instances
    """
    # The instance ID of the running macro
    instance_id = String

    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)
        shutdown_action = QAction('Shutdown', menu)
        shutdown_action.triggered.connect(self.shutdown)
        menu.addAction(shutdown_action)
        return menu

    def create_qt_item(self):
        item = QStandardItem(self.instance_id)
        item.setData(weakref.ref(self), PROJECT_CONTROLLER_REF)
        icon = get_project_device_status_icon(DeviceStatus.STATUS_ONLINE)
        item.setIcon(icon)
        item.setEditable(False)
        return item

    def single_click(self, parent_project, parent=None):
        instance_configuration = get_topology().get_device(self.instance_id)
        data = {'configuration': instance_configuration}
        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.ShowConfiguration, data))

    # ----------------------------------------------------------------------
    # action handlers

    def shutdown(self):
        get_manager().shutdownDevice(self.instance_id, showConfirm=True)


class MacroController(BaseProjectGroupController):
    """ A controller for MacroModel objects
    """
    # Redefine model with the correct type
    model = Instance(MacroModel)
    # An object which listens to system topology updates
    topo_listener = Instance(SystemTopologyListener)

    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)
        edit_action = QAction('Edit', menu)
        edit_action.triggered.connect(self._edit_macro)
        dupe_action = QAction('Duplicate', menu)
        dupe_action.triggered.connect(partial(self._duplicate_macro,
                                              parent_project))
        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_macro,
                                                parent_project))
        save_action = QAction('Save', menu)
        save_action.triggered.connect(partial(save_object, self.model))
        save_as_action = QAction('Save As...', menu)
        save_as_action.triggered.connect(self._save_macro_to_file)
        run_action = QAction('Run', menu)
        run_action.triggered.connect(partial(run_macro, self.model))
        menu.addAction(edit_action)
        menu.addAction(dupe_action)
        menu.addAction(delete_action)
        menu.addSeparator()
        menu.addAction(save_action)
        menu.addAction(save_as_action)
        menu.addSeparator()
        menu.addAction(run_action)
        return menu

    def create_qt_item(self):
        item = QStandardItem(self.model.simple_name)
        item.setData(weakref.ref(self), PROJECT_CONTROLLER_REF)
        item.setIcon(icons.file)
        item.setEditable(False)
        self.model.instances = _get_macro_instances(self.model.instance_id)
        for child in self.children:
            item.appendRow(child.qt_item)
        self.set_qt_item_text(item, self.model.simple_name)
        return item

    def double_click(self, parent_project, parent=None):
        data = {'model': self.model}
        broadcast_event(KaraboBroadcastEvent(KaraboEventSender.OpenMacro,
                                             data))

    def item_handler(self, added, removed):
        """ Called when instances are added/removed
        """
        removals = []
        for inst_id in removed:
            item_model = self._child_map[inst_id]
            self.children.remove(item_model)
            self.child_destroy(item_model)
            removals.append(item_model)

        additions = [self.child_create(model=self.model, instance_id=inst_id)
                     for inst_id in added]
        self.children.extend(additions)

        # Synchronize the GUI with the Traits model
        self._update_ui_children(additions, removals)

    def system_topology_callback(self, devices, servers):
        """ This callback is called by the ``SystemTopologyListener`` object
        in the ``topo_listener`` trait.
        """
        data = {'model': self.model}
        running_instances = self.model.instances
        for dev_id, class_id, status in devices:
            if dev_id.startswith(self.model.instance_id):
                if status == 'offline' and dev_id in running_instances:
                    running_instances.remove(dev_id)
                elif dev_id not in running_instances:
                    running_instances.append(dev_id)
                    data['instance'] = dev_id
                    # Create KaraboBroadcastEvent
                    broadcast_event(KaraboBroadcastEvent(
                        KaraboEventSender.ConnectMacroInstance, data))

    # ----------------------------------------------------------------------
    # traits notification handlers
    @on_trait_change("model.modified, model.simple_name")
    def modified_change(self):
        """ Whenever the project is modified it should be visible.

        The macro name is modified in the project panel.
        """
        if not self.is_ui_initialized():
            return
        self.set_qt_item_text(self.qt_item, self.model.simple_name)

    @on_trait_change("model.simple_name")
    def on_model_name_change(self):
        """ New macro name should appear in the middle panel """
        if not self.is_ui_initialized():
            return
        data = {'model': self.model}
        broadcast_event(KaraboBroadcastEvent(KaraboEventSender.RenameMacro,
                                             data))

    def _children_items_changed(self, event):
        """ Maintain ``_child_map`` by watching item events on ``children``

        This is a static notification handler which is connected automatically
        by Traits.
        """
        for item_model in event.removed:
            del self._child_map[item_model.instance_id]

        for item_model in event.added:
            self._child_map[item_model.instance_id] = item_model

    def _topo_listener_changed(self, name, old, new):
        """Handle broadcast event registration/unregistration here.
        """
        if old is not None:
            unregister_from_broadcasts(old)
        if new is not None:
            register_for_broadcasts(new)

    # ----------------------------------------------------------------------
    # action handlers

    def _delete_macro(self, project):
        """ Remove the macro associated with this item from its project
        """
        macro = self.model
        if macro in project.macros:
            project.macros.remove(macro)

        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.RemoveMacro, {'model': macro}))

    def _edit_macro(self):
        dialog = MacroHandleDialog(self.model)
        result = dialog.exec()
        if result == QDialog.Accepted:
            self.model.simple_name = dialog.simple_name

    def _duplicate_macro(self, project):
        macro = self.model
        dialog = ObjectDuplicateDialog(macro.simple_name)
        if dialog.exec() == QDialog.Accepted:
            code = write_macro(macro)
            for simple_name in dialog.duplicate_names:
                dupe_macro = read_macro(StringIO(code))
                dupe_macro.simple_name = simple_name
                project.macros.append(dupe_macro)

    def _save_macro_to_file(self):
        macro = self.model
        fn = getSaveFileName(caption='Save macro to file',
                             filter='Python Macro (*.py)',
                             suffix='py',
                             selectFile=macro.simple_name)
        if not fn:
            return

        if not fn.endswith('.py'):
            fn = '{}.py'.format(fn)

        with open(fn, 'w') as fout:
            fout.write(write_macro(macro))


# ----------------------------------------------------------------------

def _get_macro_instances(macro_id):
    instances = set()

    def visitor(node):
        attrs = node.attributes
        if (attrs.get('type') == 'macro' and
                node.node_id.startswith(macro_id)):
            instances.add(node.node_id)

    get_topology().visit_system_tree(visitor)
    return list(instances)
