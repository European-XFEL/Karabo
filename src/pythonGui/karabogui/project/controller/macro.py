#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
import os.path as op
import re
from functools import partial
from io import StringIO

from qtpy.QtWidgets import QAction, QDialog, QMenu, QMessageBox
from traits.api import Instance, String, on_trait_change

from karabo.common.api import walk_traits_object
from karabo.common.project.api import MacroModel, read_macro, write_macro
from karabogui import icons, messagebox
from karabogui.access import AccessRole, access_role_allowed
from karabogui.binding.api import ProxyStatus
from karabogui.events import (
    KaraboEvent, broadcast_event, register_for_broadcasts,
    unregister_from_broadcasts)
from karabogui.indicators import get_project_device_status_icon
from karabogui.itemtypes import ProjectItemTypes
from karabogui.project.dialog.object_handle import (
    ObjectDuplicateDialog, ObjectEditDialog)
from karabogui.project.topo_listener import SystemTopologyListener
from karabogui.project.utils import restart_macro, run_macro
from karabogui.singletons.api import get_config, get_manager, get_topology
from karabogui.topology.api import is_macro_online
from karabogui.util import getSaveFileName, move_to_cursor

from .bases import (
    BaseProjectController, BaseProjectGroupController, ProjectControllerUiData)


class MacroInstanceController(BaseProjectController):
    """ A controller for running macro instances
    """
    # The instance ID of the running macro
    instance_id = String

    def context_menu(self, project_controller, parent=None):

        service_allowed = access_role_allowed(AccessRole.SERVICE_EDIT)
        menu = QMenu(parent)
        shutdown_action = QAction(icons.kill, 'Shutdown', menu)
        shutdown_action.triggered.connect(partial(self.shutdown,
                                                  parent=parent))
        shutdown_action.setEnabled(service_allowed)
        menu.addAction(shutdown_action)
        return menu

    def create_ui_data(self):
        icon = get_project_device_status_icon(ProxyStatus.ONLINE)
        return ProjectControllerUiData(icon=icon)

    def single_click(self, project_controller, parent=None):
        proxy = get_topology().get_device(self.instance_id)
        broadcast_event(KaraboEvent.ShowConfiguration, {'proxy': proxy})

    def _get_display_name(self):
        """Traits property getter for ``display_name``
        """
        return self.instance_id

    def info(self):
        return {'type': ProjectItemTypes.MACRO_INSTANCE,
                'deviceId': self.instance_id}

    # ----------------------------------------------------------------------
    # action handlers

    def shutdown(self, parent=None):
        get_manager().shutdownDevice(self.instance_id, showConfirm=True,
                                     parent=parent)


class MacroController(BaseProjectGroupController):
    """ A controller for MacroModel objects
    """
    # Redefine model with the correct type
    model = Instance(MacroModel)
    # An object which listens to system topology updates
    topo_listener = Instance(SystemTopologyListener)

    def context_menu(self, project_controller, parent=None):
        project_allowed = access_role_allowed(AccessRole.PROJECT_EDIT)
        service_allowed = access_role_allowed(AccessRole.SERVICE_EDIT)

        menu = QMenu(parent)
        edit_action = QAction(icons.edit, 'Edit', menu)
        edit_action.triggered.connect(partial(self._edit_macro, parent=parent))
        edit_action.setEnabled(project_allowed)

        dupe_action = QAction(icons.editCopy, 'Duplicate', menu)
        dupe_action.triggered.connect(partial(self._duplicate_macro,
                                              project_controller,
                                              parent=parent))
        dupe_action.setEnabled(project_allowed)

        delete_action = QAction(icons.kill, 'Delete', menu)
        delete_action.triggered.connect(partial(self._delete_macro,
                                                project_controller,
                                                parent=parent))
        delete_action.setEnabled(project_allowed)

        save_as_action = QAction(icons.saveAs, 'Save to file', menu)
        save_as_action.triggered.connect(partial(self._save_macro_to_file,
                                                 parent=parent))
        run_action = QAction(icons.run, 'Run', menu)
        run_action.triggered.connect(partial(self.start_macro, parent=parent))
        run_action.setEnabled(service_allowed)

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

    def info(self):
        return {'type': ProjectItemTypes.MACRO,
                'deviceId': self.model.instance_id,
                'code': self.model.code}

    def double_click(self, project_controller, parent=None):
        broadcast_event(KaraboEvent.ShowMacroView,
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
        model = self.model.clone_traits(
            traits=["instance_id", "project_name", "uuid"], copy="deep")
        additions = [self.child_create(model=model, parent=self,
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
                if (status is ProxyStatus.OFFLINE and
                        dev_id in running_instances):
                    running_instances.remove(dev_id)
                elif dev_id not in running_instances:
                    running_instances.append(dev_id)
                    data['instance'] = dev_id
                    # Create KaraboBroadcastEvent
                    broadcast_event(KaraboEvent.ConnectMacroInstance,
                                    data)

    # ----------------------------------------------------------------------
    # Traits handlers

    @on_trait_change("model:initialized")
    def _update_ui_instances(self):
        """ Whenever the object is modified it should be visible to the user
        """
        self.model.instances = _get_macro_instances(self.model.instance_id)
        project = self.parent
        if project is not None:
            self.model.project_name = project.model.simple_name

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
            unregister_from_broadcasts(
                {KaraboEvent.SystemTopologyUpdate: old._event_topology})
        if new is not None:
            register_for_broadcasts(
                {KaraboEvent.SystemTopologyUpdate: new._event_topology})

    # ----------------------------------------------------------------------
    # action handlers

    def _delete_macro(self, project_controller, parent=None):
        """ Remove the macro associated with this item from its project
        """
        macro = self.model

        ask = ('Are you sure you want to delete \"<b>{}</b>\".<br /> '
               'Continue action?'.format(macro.simple_name))
        msg_box = QMessageBox(QMessageBox.Question, 'Delete macro',
                              ask, QMessageBox.Yes | QMessageBox.No,
                              parent=parent)
        msg_box.setModal(False)
        msg_box.setDefaultButton(QMessageBox.No)
        move_to_cursor(msg_box)
        if msg_box.exec() == QMessageBox.Yes:
            project = project_controller.model
            if macro in project.macros:
                project.macros.remove(macro)

            broadcast_event(KaraboEvent.RemoveProjectModelViews,
                            {'models': [macro]})

    def _edit_macro(self, parent=None):
        dialog = ObjectEditDialog(object_type='macro', model=self.model,
                                  parent=parent)
        move_to_cursor(dialog)
        result = dialog.exec()
        if result == QDialog.Accepted:
            self.model.simple_name = dialog.simple_name

    def _duplicate_macro(self, project_controller, parent=None):
        macro = self.model
        project = project_controller.model
        dialog = ObjectDuplicateDialog(macro.simple_name, parent=parent)
        move_to_cursor(dialog)
        if dialog.exec() == QDialog.Accepted:
            code = write_macro(macro)
            for simple_name in dialog.duplicate_names:
                dupe_macro = read_macro(StringIO(code))
                dupe_macro.simple_name = simple_name
                project.macros.append(dupe_macro)

    def _save_macro_to_file(self, parent=None):
        config = get_config()
        path = config['data_dir']
        directory = path if path and op.isdir(path) else ""

        macro = self.model
        filename = macro.simple_name
        filename = re.sub(r'[\W]', '-', filename)
        fn = getSaveFileName(caption='Save macro to file',
                             filter='Python Macro (*.py)',
                             suffix='py',
                             selectFile=filename,
                             directory=directory,
                             parent=parent)
        if not fn:
            return

        config['data_dir'] = op.dirname(fn)

        if not fn.endswith('.py'):
            fn = f'{fn}.py'

        with open(fn, 'w') as fout:
            fout.write(write_macro(macro))

    def start_macro(self, parent=None):
        """Run not running macro and restart running macro

        NOTE: Macro can be restarted only after user confirmation
        """
        instance_id = self.model.instance_id
        if is_macro_online(instance_id):
            restart_macro(instance_id, self.run_macro)
        else:
            self.run_macro()

    def run_macro(self, parent=None):
        """Action handler to instantiate the macro

        NOTE: This method is also used to instantiate all macros in a project!
        """
        try:
            compile(self.model.code, self.model.simple_name, "exec")
        except SyntaxError as e:
            # Show erroneous macro before showing the error dialog
            broadcast_event(KaraboEvent.ShowMacroView,
                            {'model': self.model})
            formatted_msg = "{}\n{}{}^\nin {} line {}".format(
                e.msg, e.text, " " * e.offset, e.filename, e.lineno)
            messagebox.show_warning(formatted_msg, title=type(e).__name__,
                                    parent=parent)
            return

        run_macro(self.model, parent)

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


def get_project_macros(project_controller):
    """Given a ``project_controller`` return all the online and offline macros
    """
    online = []
    offline = []

    def visitor(obj):
        if isinstance(obj, MacroController):
            if obj.children:
                online.append(obj)
            else:
                offline.append(obj)

    walk_traits_object(project_controller, visitor)

    return online, offline
