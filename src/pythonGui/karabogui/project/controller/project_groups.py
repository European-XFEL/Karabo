#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
import os.path as op

from PyQt4.QtGui import QAction, QDialog, QMenu
from traits.api import Instance, Property, String

from karabo.common.api import Capabilities
from karabo.common.project.api import (
    DeviceServerModel, MacroModel, ProjectModel, read_macro)
from karabo.common.scenemodel.api import SceneModel, read_scene
from karabogui import icons, messagebox
from karabogui.dialogs.device_capability import DeviceCapabilityDialog
from karabogui.enums import KaraboSettings
from karabogui.project.dialog.object_handle import ObjectEditDialog
from karabogui.project.dialog.server_handle import ServerHandleDialog
from karabogui.request import call_device_slot
from karabogui.util import (
    getOpenFileName, get_setting, set_setting, handle_macro_from_server,
    handle_scene_from_server)
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


def _fill_macros_menu(menu, project_controller):
    add_action = QAction('Add macro', menu)
    add_action.triggered.connect(partial(_add_macro, project_controller))
    load_action = QAction('Load macro...', menu)
    load_action.triggered.connect(partial(_load_macro, project_controller))
    load_from_device = QAction('Load from device...', menu)
    load_from_device.triggered.connect(partial(_load_macro_from_device,
                                               project_controller))
    menu.addAction(add_action)
    menu.addAction(load_action)
    menu.addAction(load_from_device)


def _fill_scenes_menu(menu, project_controller):
    add_action = QAction('Add scene', menu)
    add_action.triggered.connect(partial(_add_scene, project_controller))
    load_action = QAction('Load scene...', menu)
    load_action.triggered.connect(partial(_load_scene, project_controller))
    load_from_device = QAction('Load from device...', menu)
    load_from_device.triggered.connect(partial(_load_scene_from_device,
                                               project_controller))
    menu.addAction(add_action)
    menu.addAction(load_action)
    menu.addAction(load_from_device)


def _fill_servers_menu(menu, project_controller):
    add_action = QAction('Add server', menu)
    add_action.triggered.connect(partial(_add_server, project_controller))
    menu.addAction(add_action)


# ----------------------------------------------------------------------
# action handlers

_macro_template = """\
from karabo.middlelayer import Macro, Slot, String

class {0}(Macro):
    name = String(defaultValue="{0}")

    @Slot()
    def execute(self):
        print("Hello {{}}!".format(self.name))
"""


def _add_macro(project_controller):
    """ Add a macro to the associated project
    """
    project = project_controller.model
    dialog = ObjectEditDialog(object_type='macro')
    if dialog.exec() == QDialog.Accepted:
        classname = dialog.simple_name.title()
        classname = "".join(c for c in classname if c.isalpha())
        # XXX: TODO check for existing
        macro = MacroModel(simple_name=dialog.simple_name,
                           code=_macro_template.format(classname))
        # Set initialized and modified last
        macro.initialized = macro.modified = True
        project.macros.append(macro)


def _load_macro(project_controller):
    path = get_setting(KaraboSettings.MACRO_DIR)
    directory = path if path and op.isdir(path) else ""

    fn = getOpenFileName(caption='Load macro', filter='Python Macros (*.py)',
                         directory=directory)
    if not fn:
        return

    # Store old macro dialog path
    set_setting(KaraboSettings.MACRO_DIR, op.dirname(fn))

    project = project_controller.model
    # Read MacroModel
    macro = read_macro(fn)
    # Set the scene model title
    macro.simple_name = op.splitext(op.basename(fn))[0]
    macro.modified = True
    project.macros.append(macro)


def _load_macro_from_device(project_controller):
    """Request a scene directly from a device
    """
    dialog = DeviceCapabilityDialog(capability=Capabilities.PROVIDES_MACROS)
    if dialog.exec() == QDialog.Accepted:
        device_id = dialog.device_id
        macro_name = dialog.capa_name
        project = project_controller.model
        project_macros = {s.simple_name for s in project.macros}
        if '{}-{}'.format(device_id, macro_name) in project_macros:
            msg = ('A macro with that name already exists in the selected '
                   'project.')
            messagebox.show_warning(msg, title='Cannot Load Macro')
            return

        handler = partial(handle_macro_from_server, device_id, macro_name,
                          project)
        call_device_slot(handler, device_id, 'requestMacro', name=macro_name)


def _add_scene(project_controller):
    """ Add a scene to the associated project
    """
    project = project_controller.model
    dialog = ObjectEditDialog(object_type='scene')
    if dialog.exec() == QDialog.Accepted:
        # XXX: TODO check for existing
        scene = SceneModel(simple_name=dialog.simple_name)
        # Set initialized and modified last
        scene.initialized = scene.modified = True
        project.scenes.append(scene)


def _load_scene(project_controller):
    """ Load a scene from local disk
    """
    path = get_setting(KaraboSettings.SCENE_DIR)
    directory = path if path and op.isdir(path) else ""

    fn = getOpenFileName(caption='Load scene', filter='SVG Files (*.svg)',
                         directory=directory)
    if not fn:
        return

    # Store old scene dialog path
    set_setting(KaraboSettings.SCENE_DIR, op.dirname(fn))

    project = project_controller.model
    # Read SceneModel
    scene = read_scene(fn)
    scene.simple_name = op.splitext(op.basename(fn))[0]
    scene.reset_uuid()
    scene.modified = True
    project.scenes.append(scene)


def _load_scene_from_device(project_controller):
    """Request a scene directly from a device
    """
    dialog = DeviceCapabilityDialog(capability=Capabilities.PROVIDES_SCENES)
    if dialog.exec() == QDialog.Accepted:
        device_id = dialog.device_id
        scene_name = dialog.capa_name
        project = project_controller.model
        project_scenes = {s.simple_name for s in project.scenes}

        if '{}|{}'.format(device_id, scene_name) in project_scenes:
            msg = ('A scene with that name already exists in the selected '
                   'project.')
            messagebox.show_warning(msg, title='Cannot Load Scene')
            return

        handler = partial(handle_scene_from_server, device_id, scene_name,
                          project, None)
        call_device_slot(handler, device_id, 'requestScene', name=scene_name)


def _add_server(project_controller):
    """ Add a server to the associated project
    """
    project = project_controller.model
    dialog = ServerHandleDialog()
    if dialog.exec() == QDialog.Accepted:
        # XXX: TODO check for existing
        traits = {
            'server_id': dialog.server_id,
            'host': dialog.host,
            'description': dialog.description
        }
        server = DeviceServerModel(**traits)
        # Set initialized and modified last
        server.initialized = server.modified = True
        project.servers.append(server)
