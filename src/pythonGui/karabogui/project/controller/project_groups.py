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
from functools import partial

from qtpy.QtWidgets import QAction, QDialog, QMenu, QMessageBox
from traits.api import Instance, Property, String

from karabo.common.api import Capabilities
from karabo.common.project.api import (
    DeviceServerModel, MacroModel, ProjectModel, read_macro)
from karabo.common.scenemodel.api import SceneModel, read_scene
from karabogui import icons, messagebox
from karabogui.access import (
    AccessRole, access_role_allowed, get_access_level_for_role)
from karabogui.binding.api import ProxyStatus
from karabogui.dialogs.device_capability import DeviceCapabilityDialog
from karabogui.itemtypes import ProjectItemTypes
from karabogui.project.dialog.move_handle import MoveHandleDialog
from karabogui.project.dialog.object_handle import ObjectEditDialog
from karabogui.project.dialog.server_handle import ServerHandleDialog
from karabogui.project.utils import (
    check_device_server_exists, check_macro_exists)
from karabogui.request import get_macro_from_server, get_scene_from_server
from karabogui.singletons.api import (
    get_config, get_network, get_panel_wrangler)
from karabogui.util import (
    VALID_PROJECT_OBJECT_NAME, getOpenFileName, move_to_cursor,
    show_filename_error)
from karabogui.wizards.api import CinemaWizardController

from .bases import BaseProjectGroupController, ProjectControllerUiData


class ProjectSubgroupController(BaseProjectGroupController):
    """ A controller for ProjectModel subgroups (devices, scenes, macros, etc.)
    """
    # Redefine model with the correct type
    model = Instance(ProjectModel)
    # The name of the group shown in the GUI
    group_name = String
    # The name of the trait on ``model`` which is controlled by ``children``
    trait_name = String

    # NOTE: We're overriding the base class here
    ui_data = Property(Instance(ProjectControllerUiData))

    def context_menu(self, project_controller, parent=None):
        menu_fillers = {
            'macros': _fill_macros_menu,
            'scenes': _fill_scenes_menu,
            'servers': _fill_servers_menu,
        }
        filler = menu_fillers.get(self.trait_name)
        menu = QMenu(parent)
        menu.setToolTipsVisible(True)
        filler(menu, project_controller)
        return menu

    def create_ui_data(self):
        """Satisfy the ABC
        """
        return None

    def ui_item_text(self):
        """The name for this controller's item in the GUI view

        NOTE: We are overriding this here to avoid the addition of a '*' when
        the project object is in a modified state.
        """
        return self.display_name

    def _get_display_name(self):
        """Traits property getter for ``display_name``
        """
        return self.group_name

    def _get_ui_data(self):
        return ProjectControllerUiData(icon=icons.folder)

    def _set_ui_data(self, value):
        """Explicitly ignore assignment to the ``ui_data`` trait.
        """
        pass

    def info(self):
        return {'type': ProjectItemTypes.PROJECT_GROUP}


def _fill_macros_menu(menu, project_controller):
    project_allowed = access_role_allowed(AccessRole.PROJECT_EDIT)

    add_action = QAction(icons.add, 'Add macro', menu)
    add_action.triggered.connect(partial(_add_macro, project_controller,
                                         parent=menu.parent()))
    add_action.setEnabled(project_allowed)

    load_action = QAction(icons.load, 'Load macro...', menu)
    load_action.triggered.connect(partial(_load_macro, project_controller,
                                          parent=menu.parent()))
    load_action.setEnabled(project_allowed)

    load_from_device = QAction(icons.download, 'Load from device...', menu)
    load_from_device.triggered.connect(partial(_load_macro_from_device,
                                               project_controller,
                                               parent=menu.parent()))
    load_from_device.setEnabled(project_allowed)

    move_action = QAction(icons.move, 'Arrange Macros', menu)
    move_action.triggered.connect(partial(_move_project_macros,
                                          project_controller,
                                          parent=menu.parent()))
    move_action.setEnabled(project_allowed)

    menu.addAction(add_action)
    menu.addAction(load_action)
    menu.addAction(load_from_device)
    menu.addAction(move_action)
    if not project_allowed:
        _add_disabled_tooltip(menu)


def _fill_scenes_menu(menu, project_controller):
    project_allowed = access_role_allowed(AccessRole.PROJECT_EDIT)

    add_action = QAction(icons.add, 'Add scene', menu)
    add_action.triggered.connect(partial(_add_scene, project_controller,
                                         parent=menu.parent()))
    add_action.setEnabled(project_allowed)

    load_action = QAction(icons.load, 'Load scene...', menu)
    load_action.triggered.connect(partial(_load_scene, project_controller,
                                          parent=menu.parent()))
    load_action.setEnabled(project_allowed)

    load_from_device = QAction(icons.download, 'Load from device...', menu)
    load_from_device.triggered.connect(partial(_load_scene_from_device,
                                               project_controller,
                                               parent=menu.parent()))
    load_from_device.setEnabled(project_allowed)

    project_model = project_controller.model
    cinema_action = QAction(icons.run, 'Create cinema link', menu)
    cinema_action.triggered.connect(partial(_create_cinema_link,
                                            project_model=project_model,
                                            parent=menu.parent()))

    move_action = QAction(icons.move, 'Arrange Scenes', menu)
    move_action.triggered.connect(partial(_move_project_scenes,
                                          project_controller,
                                          parent=menu.parent()))
    move_action.setEnabled(project_allowed)

    about_action = QAction(icons.about, 'About', menu)
    about_action.triggered.connect(partial(_about_scene,
                                           project_controller,
                                           parent=menu.parent()))
    menu.addAction(add_action)
    menu.addAction(load_action)
    menu.addAction(load_from_device)
    menu.addSeparator()
    menu.addAction(cinema_action)
    menu.addSeparator()
    menu.addAction(move_action)
    menu.addSeparator()
    menu.addAction(about_action)

    if not project_allowed:
        _add_disabled_tooltip(menu)


def _fill_servers_menu(menu, project_controller):
    project_allowed = access_role_allowed(AccessRole.PROJECT_EDIT)

    add_action = QAction(icons.add, 'Add server', menu)
    add_action.triggered.connect(partial(_add_server,
                                         project_controller,
                                         parent=menu.parent()))
    add_action.setEnabled(project_allowed)

    server_action = QAction(icons.kill, "Shutdown all servers", menu)
    server_action.triggered.connect(partial(_shutdown_servers,
                                            project_controller,
                                            parent=menu.parent()))
    instance_allowed = access_role_allowed(AccessRole.INSTANCE_CONTROL)
    server_action.setEnabled(instance_allowed)

    menu.addAction(add_action)
    menu.addAction(server_action)

    if not project_allowed:
        _add_disabled_tooltip(menu)


def _add_disabled_tooltip(menu: QMenu) -> None:
    """Add a tooltip to each disabled item in the menu to explain why it is
    disabled."""
    required_access_level = get_access_level_for_role(
        AccessRole.PROJECT_EDIT)
    tooltip = f"Requires minimum '{required_access_level}' access level"
    for action in menu.actions():
        if not action.isEnabled():
            action.setToolTip(tooltip)


# ----------------------------------------------------------------------
# action handlers

_macro_template = """\
from karabo.middlelayer import Macro, MacroSlot, String


class {0}(Macro):
    name = String(defaultValue="{0}")

    @MacroSlot()
    def execute(self):
        print("Hello {{}}!".format(self.name))
"""


def _add_macro(project_controller, parent=None):
    """ Add a macro to the associated project
    """
    project = project_controller.model
    dialog = ObjectEditDialog(object_type='macro', parent=parent)
    move_to_cursor(dialog)
    if dialog.exec() == QDialog.Accepted:
        classname = dialog.simple_name.title()
        classname = "".join(c for c in classname if c.isalpha())
        macro_name = dialog.simple_name
        if check_macro_exists(macro_name):
            return
        macro = MacroModel(simple_name=macro_name,
                           code=_macro_template.format(classname),
                           project_name=project.simple_name)
        # Set initialized and modified last
        macro.initialized = macro.modified = True
        project.macros.append(macro)


def _load_macro(project_controller, parent=None):
    path = get_config()['data_dir']
    directory = path if path and op.isdir(path) else ""

    fn = getOpenFileName(caption='Load macro', filter='Python Macros (*.py)',
                         directory=directory, parent=parent)
    if not fn:
        return

    simple_name = op.splitext(op.basename(fn))[0]

    if not VALID_PROJECT_OBJECT_NAME.match(simple_name):
        show_filename_error(simple_name, parent=parent)
        return

    # Store old macro dialog path
    get_config()['data_dir'] = op.dirname(fn)

    project = project_controller.model
    # Read MacroModel
    macro = read_macro(fn)
    # Set the scene model title
    macro.simple_name = simple_name
    macro.modified = True
    project.macros.append(macro)


def _load_macro_from_device(project_controller, parent=None):
    """Request a scene directly from a device
    """
    dialog = DeviceCapabilityDialog(capability=Capabilities.PROVIDES_MACROS,
                                    parent=parent)
    if dialog.exec() == QDialog.Accepted:
        device_id = dialog.device_id
        macro_name = dialog.capa_name
        project = project_controller.model
        project_macros = {s.simple_name for s in project.macros}
        if f'{device_id}-{macro_name}' in project_macros:
            msg = ('A macro with that name already exists in the selected '
                   'project.')
            messagebox.show_warning(msg, title='Cannot Load Macro',
                                    parent=parent)
            return

        get_macro_from_server(device_id, macro_name=macro_name,
                              project=project)


def _move_project_macros(project_controller, parent=None):
    wrangler = get_panel_wrangler()
    project = project_controller.model
    showing = [wrangler.is_showing_project_item(model)
               for model in project.macros]
    if any(showing):
        messagebox.show_error(
            "Please close all macros in the group element first before "
            "reordering.", parent=parent)
        return

    macros = list(project.macros)
    dialog = MoveHandleDialog(macros, parent=parent)
    if dialog.exec() == QDialog.Accepted:
        del project.macros[:]
        project.macros.extend(dialog.items)


def _add_scene(project_controller, parent=None):
    """ Add a scene to the associated project
    """
    project = project_controller.model
    dialog = ObjectEditDialog(object_type='scene', parent=parent)
    move_to_cursor(dialog)
    if dialog.exec() == QDialog.Accepted:
        scene = SceneModel(simple_name=dialog.simple_name)
        # Set initialized and modified last
        scene.initialized = scene.modified = True
        project.scenes.append(scene)


def _about_scene(project_controller, parent=None):
    """ Retrieve the UUIDs of the scenes in the project controller for display
    """
    project = project_controller.model
    html = "<ul>" + "".join(["<li>" + child.uuid + "</li>"
                             for child in project.scenes]) + "</ul>"
    messagebox.show_information(html, title="List of scene UUIDs",
                                parent=parent)


def _load_scene(project_controller, parent=None):
    """ Load a scene from local disk
    """
    path = get_config()['data_dir']
    directory = path if path and op.isdir(path) else ""

    fn = getOpenFileName(caption='Load scene', filter='SVG Files (*.svg)',
                         directory=directory, parent=parent)
    if not fn:
        return

    simple_name = op.splitext(op.basename(fn))[0]

    if not VALID_PROJECT_OBJECT_NAME.match(simple_name):
        show_filename_error(simple_name, parent=parent)
        return

    # Store old scene dialog path
    get_config()['data_dir'] = op.dirname(fn)

    project = project_controller.model
    # Read SceneModel
    scene = read_scene(fn)
    scene.simple_name = simple_name
    scene.reset_uuid()
    scene.modified = True
    project.scenes.append(scene)


def _load_scene_from_device(project_controller, parent=None):
    """Request a scene directly from a device
    """
    dialog = DeviceCapabilityDialog(capability=Capabilities.PROVIDES_SCENES,
                                    parent=parent)
    if dialog.exec() == QDialog.Accepted:
        device_id = dialog.device_id
        scene_name = dialog.capa_name
        project = project_controller.model
        project_scenes = {s.simple_name for s in project.scenes}

        if f'{device_id}|{scene_name}' in project_scenes:
            msg = ('A scene with that name already exists in the selected '
                   'project.')
            messagebox.show_warning(msg, title='Cannot Load Scene',
                                    parent=parent)
            return

        get_scene_from_server(device_id, scene_name=scene_name,
                              project=project)


def _move_project_scenes(project_controller, parent=None):
    wrangler = get_panel_wrangler()
    project = project_controller.model
    showing = [wrangler.is_showing_project_item(model)
               for model in project.scenes]
    if any(showing):
        messagebox.show_error(
            "Please close all scenes in the group element first before "
            "reordering.", parent=parent)
        return
    scenes = list(project.scenes)
    dialog = MoveHandleDialog(scenes, parent=parent)
    if dialog.exec() == QDialog.Accepted:
        del project.scenes[:]
        project.scenes.extend(dialog.items)


def _create_cinema_link(project_model=None, parent=None):
    wizard = CinemaWizardController(project_model=project_model,
                                    parent=parent)
    wizard.run()


def _add_server(project_controller, parent=None):
    """ Add a server to the associated project
    """
    project = project_controller.model
    dialog = ServerHandleDialog(parent=parent)
    move_to_cursor(dialog)
    if dialog.exec() == QDialog.Accepted:
        serverId = dialog.server_id
        if check_device_server_exists(serverId):
            return
        traits = {
            'server_id': serverId,
        }
        server = DeviceServerModel(**traits)
        # Set initialized and modified last
        server.initialized = server.modified = True
        project.servers.append(server)


def _shutdown_servers(project_controller, parent=None):
    """Shutdown all servers in a project group"""
    project = project_controller.model
    ask = (f"Are you sure you want to shutdown all servers of project "
           f"'{project.simple_name}'?")
    msg_box = QMessageBox(QMessageBox.Question, "Shutdown servers",
                          ask, QMessageBox.Yes | QMessageBox.No,
                          parent=parent)
    msg_box.setModal(False)
    msg_box.setDefaultButton(QMessageBox.No)
    move_to_cursor(msg_box)
    if msg_box.exec() != QMessageBox.Yes:
        return

    for server in project.servers:
        if server.status is not ProxyStatus.ONLINE:
            continue
        get_network().onKillServer(server.server_id)
