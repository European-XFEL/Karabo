#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
import os.path as op
import weakref

from PyQt4.QtGui import QAction, QDialog, QMenu, QStandardItem
from traits.api import Instance, String

from karabo.common.project.api import (
    DeviceServerModel, MacroModel, ProjectModel, read_macro)
from karabo.common.scenemodel.api import SceneModel, read_scene
from karabo_gui import icons
from karabo_gui.const import PROJECT_CONTROLLER_REF
from karabo_gui.project.dialog.macro_handle import MacroHandleDialog
from karabo_gui.project.dialog.project_handle import NewProjectDialog
from karabo_gui.project.dialog.scene_handle import SceneHandleDialog
from karabo_gui.project.dialog.server_handle import ServerHandleDialog
from karabo_gui.util import getOpenFileName
from .bases import BaseProjectGroupController


class ProjectSubgroupController(BaseProjectGroupController):
    """ A controller for ProjectModel subgroups (devices, scenes, macros, etc.)
    """
    # Redefine model with the correct type
    model = Instance(ProjectModel)
    # The name of the group shown in the GUI
    group_name = String
    # The name of the trait on ``model`` which is controlled by ``children``
    trait_name = String

    def context_menu(self, parent_project, parent=None):
        menu_fillers = {
            'macros': _fill_macros_menu,
            'scenes': _fill_scenes_menu,
            'servers': _fill_servers_menu,
            'subprojects': _fill_subprojects_menu,
        }
        filler = menu_fillers.get(self.trait_name)
        menu = QMenu(parent)
        filler(menu, parent_project)
        return menu

    def create_qt_item(self):
        item = QStandardItem(self.group_name)
        item.setData(weakref.ref(self), PROJECT_CONTROLLER_REF)
        item.setIcon(icons.folder)
        item.setEditable(False)
        for child in self.children:
            item.appendRow(child.qt_item)
        return item


def _fill_macros_menu(menu, parent_project):
    add_action = QAction('Add macro', menu)
    add_action.triggered.connect(partial(_add_macro, parent_project))
    load_action = QAction('Load macro...', menu)
    load_action.triggered.connect(partial(_load_macro, parent_project))
    menu.addAction(add_action)
    menu.addAction(load_action)


def _fill_scenes_menu(menu, parent_project):
    add_action = QAction('Add scene', menu)
    add_action.triggered.connect(partial(_add_scene, parent_project))
    load_action = QAction('Load scene...', menu)
    load_action.triggered.connect(partial(_load_scene, parent_project))
    menu.addAction(add_action)
    menu.addAction(load_action)


def _fill_servers_menu(menu, parent_project):
    add_action = QAction('Add server', menu)
    add_action.triggered.connect(partial(_add_server, parent_project))
    menu.addAction(add_action)


def _fill_subprojects_menu(menu, parent_project):
    add_action = QAction('Add project', menu)
    add_action.triggered.connect(partial(_add_project, parent_project))
    menu.addAction(add_action)


# ----------------------------------------------------------------------
# action handlers


def _add_macro(project):
    """ Add a macro to the associated project
    """
    dialog = MacroHandleDialog()
    if dialog.exec() == QDialog.Accepted:
        # XXX: TODO check for existing
        macro = MacroModel(simple_name=dialog.simple_name, initialized=True,
                           modified=True)
        project.macros.append(macro)


def _load_macro(project):
    fn = getOpenFileName(caption='Load macro', filter='Python Macros (*.py)')
    if not fn:
        return
    # Read MacroModel
    macro = read_macro(fn)
    # Set the scene model title
    macro.simple_name = op.splitext(op.basename(fn))[0]
    macro.modified = True
    project.macros.append(macro)


def _add_scene(project):
    """ Add a scene to the associated project
    """
    dialog = SceneHandleDialog()
    if dialog.exec() == QDialog.Accepted:
        # XXX: TODO check for existing
        scene = SceneModel(simple_name=dialog.simple_name, initialized=True,
                           modified=True)
        project.scenes.append(scene)


def _load_scene(project):
    fn = getOpenFileName(caption='Load scene', filter='SVG Files (*.svg)')
    if not fn:
        return
    # Read SceneModel
    scene = read_scene(fn)
    scene.simple_name = op.splitext(op.basename(fn))[0]
    scene.modified = True
    project.scenes.append(scene)


def _add_server(project):
    """ Add a server to the associated project
    """
    dialog = ServerHandleDialog()
    if dialog.exec() == QDialog.Accepted:
        # XXX: TODO check for existing
        traits = {
            'server_id': dialog.server_id,
            'host': dialog.host,
            'description': dialog.description,
            'initialized': True,
            'modified': True,
        }
        server = DeviceServerModel(**traits)
        project.servers.append(server)


def _add_project(project):
    """ Add a new subproject to the associated project
    """
    dialog = NewProjectDialog()
    if dialog.exec() == QDialog.Accepted:
        # XXX: TODO check for existing
        model = ProjectModel(simple_name=dialog.simple_name, initialized=True,
                             modified=True)
        project.subprojects.append(model)
