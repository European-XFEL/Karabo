#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
from io import StringIO

from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QAction, QDialog, QMenu, QMessageBox
from traits.api import Instance, String, on_trait_change

from karabo.common.api import DeviceStatus
from karabo.common.project.api import MacroModel, read_macro, write_macro
from karabogui import icons
from karabogui.events import (
    broadcast_event, register_for_broadcasts, unregister_from_broadcasts,
    KaraboEventSender)
from karabogui.indicators import get_project_device_status_icon
from karabogui.project.dialog.object_handle import (
    ObjectDuplicateDialog, ObjectEditDialog)
from karabogui.project.topo_listener import SystemTopologyListener
from karabogui.project.utils import run_macro
from karabogui.singletons.api import get_manager, get_topology
from karabogui.util import getSaveFileName
from .bases import (BaseProjectGroupController, BaseProjectController,
                    ProjectControllerUiData)


class MacroInstanceController(BaseProjectController):
    """ A controller for running macro instances
    """
    # The instance ID of the running macro
    instance_id = String

    def context_menu(self, project_controller, parent=None):
        menu = QMenu(parent)
        shutdown_action = QAction('Shutdown', menu)
        shutdown_action.triggered.connect(self.shutdown)
        menu.addAction(shutdown_action)
        return menu

    def create_ui_data(self):
        icon = get_project_device_status_icon(DeviceStatus.ONLINE)
        return ProjectControllerUiData(icon=icon)

    def single_click(self, project_controller, parent=None):
        proxy = get_topology().get_device(self.instance_id)
        broadcast_event(KaraboEventSender.ShowConfiguration, {'proxy': proxy})

    def _get_display_name(self):
        """Traits property getter for ``display_name``
        """
        return self.instance_id

    # ----------------------------------------------------------------------
    # action handlers

    @pyqtSlot()
    def shutdown(self):
        get_manager().shutdownDevice(self.instance_id, showConfirm=True)


class MacroController(BaseProjectGroupController):
    """ A controller for MacroModel objects
    """
    # Redefine model with the correct type
    model = Instance(MacroModel)
    # An object which listens to system topology updates
    topo_listener = Instance(SystemTopologyListener)

    def context_menu(self, project_controller, parent=None):
        menu = QMenu(parent)
        edit_action = QAction('Edit', menu)
        edit_action.triggered.connect(self._edit_macro)
        dupe_action = QAction('Duplicate', menu)
        dupe_action.triggered.connect(partial(self._duplicate_macro,
                                              project_controller))
        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_macro,
                                                project_controller))
        save_as_action = QAction('Save to file', menu)
        save_as_action.triggered.connect(self._save_macro_to_file)
        run_action = QAction('Run', menu)
        run_action.triggered.connect(partial(run_macro, self.model))
        menu.addAction(edit_action)
        menu.addAction(dupe_action)
        menu.addAction(delete_action)
        menu.addSeparator()
        menu.addAction(save_as_action)
        menu.addSeparator()
        menu.addAction(run_action)
        return menu

    def create_ui_data(self):
        return ProjectControllerUiData(icon=icons.file)

    def double_click(self, project_controller, parent=None):
        broadcast_event(KaraboEventSender.ShowMacroView,
                        {'model': self.model})

    def item_handler(self, added, removed):
        """ Called when instances are added/removed
        """
        for inst_id in removed:
            controller = self._child_map[inst_id]
            # NOTE: Always use a context manager when modifying self.children!
            with self._qt_model.removal_context(controller):
                self.children.remove(controller)
            self.child_destroy(controller)

        additions = [self.child_create(model=self.model, parent=self,
                                       _qt_model=self._qt_model,
                                       instance_id=inst_id)
                     for inst_id in added]
        # Synchronize the GUI with the Traits model
        self._update_ui_children(additions)

    def system_topology_callback(self, devices, servers):
        """ This callback is called by the ``SystemTopologyListener`` object
        in the ``topo_listener`` trait.
        """
        data = {'model': self.model}
        running_instances = self.model.instances
        for dev_id, class_id, status in devices:
            if dev_id.startswith(self.model.instance_id):
                if (DeviceStatus(status) is DeviceStatus.OFFLINE and
                        dev_id in running_instances):
                    running_instances.remove(dev_id)
                elif dev_id not in running_instances:
                    running_instances.append(dev_id)
                    data['instance'] = dev_id
                    # Create KaraboBroadcastEvent
                    broadcast_event(KaraboEventSender.ConnectMacroInstance,
                                    data)

    # ----------------------------------------------------------------------
    # Traits handlers

    @on_trait_change("model:initialized")
    def _update_ui_instances(self):
        """ Whenever the object is modified it should be visible to the user
        """
        self.model.instances = _get_macro_instances(self.model.instance_id)

    # ----------------------------------------------------------------------
    # traits notification handlers

    def _children_items_changed(self, event):
        """ Maintain ``_child_map`` by watching item events on ``children``

        This is a static notification handler which is connected automatically
        by Traits.
        """
        for controller in event.removed:
            self._child_map.pop(controller.instance_id, None)

        for controller in event.added:
            self._child_map[controller.instance_id] = controller

    def _topo_listener_changed(self, name, old, new):
        """Handle broadcast event registration/unregistration here.
        """
        if old is not None:
            unregister_from_broadcasts(old)
        if new is not None:
            register_for_broadcasts(new)

    # ----------------------------------------------------------------------
    # action handlers

    def _delete_macro(self, project_controller):
        """ Remove the macro associated with this item from its project
        """
        macro = self.model

        ask = ('Are you sure you want to delete \"<b>{}</b>\".<br /> '
               'Continue action?'.format(macro.simple_name))
        msg_box = QMessageBox(QMessageBox.Question, 'Delete macro',
                              ask, QMessageBox.Yes | QMessageBox.No)
        msg_box.setModal(False)
        msg_box.setDefaultButton(QMessageBox.No)
        if msg_box.exec() == QMessageBox.Yes:
            project = project_controller.model
            if macro in project.macros:
                project.macros.remove(macro)

            broadcast_event(KaraboEventSender.RemoveProjectModelViews,
                            {'models': [macro]})

    @pyqtSlot()
    def _edit_macro(self):
        dialog = ObjectEditDialog(object_type='macro', model=self.model)
        result = dialog.exec()
        if result == QDialog.Accepted:
            self.model.simple_name = dialog.simple_name

    def _duplicate_macro(self, project_controller):
        macro = self.model
        project = project_controller.model
        dialog = ObjectDuplicateDialog(macro.simple_name)
        if dialog.exec() == QDialog.Accepted:
            code = write_macro(macro)
            for simple_name in dialog.duplicate_names:
                dupe_macro = read_macro(StringIO(code))
                dupe_macro.simple_name = simple_name
                project.macros.append(dupe_macro)

    @pyqtSlot()
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
