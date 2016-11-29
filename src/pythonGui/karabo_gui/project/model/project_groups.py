#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
import os.path as op
import weakref

from PyQt4.QtGui import QAction, QDialog, QMenu, QStandardItem
from traits.api import Callable, Dict, Instance, List, String

from karabo.common.project.api import (DeviceServerModel, MacroModel,
                                       ProjectModel)
from karabo.common.scenemodel.api import SceneModel
from karabo.middlelayer import read_macro, read_scene
from karabo_gui import icons
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
from karabo_gui.project.dialog.macro_handle import MacroHandleDialog
from karabo_gui.project.dialog.project_handle import NewProjectDialog
from karabo_gui.project.dialog.scene_handle import SceneHandleDialog
from karabo_gui.project.dialog.server_handle import ServerHandleDialog
from karabo_gui.util import getOpenFileName
from .bases import BaseProjectTreeItem


class ProjectSubgroupItem(BaseProjectTreeItem):
    """ A wrapper for ProjectModel subgroups (devices, scenes, macros, etc.)
    """
    # Redefine model with the correct type
    model = Instance(ProjectModel)

    # The name of the group shown in the GUI
    group_name = String

    # The name of the trait on ``model`` which is shadowed by ``children``
    trait_name = String

    # A factory for shadow items wrapping children
    child_create = Callable

    # A callable which can gracefully destroy a child shadow object
    child_destroy = Callable

    # The child tree items
    children = List(Instance(BaseProjectTreeItem))
    _child_map = Dict  # dictionary for fast lookups during removal

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
        item.setData(weakref.ref(self), PROJECT_ITEM_MODEL_REF)
        item.setIcon(icons.folder)
        item.setEditable(False)
        for child in self.children:
            item.appendRow(child.qt_item)
        return item

    def item_handler(self, event):
        """ Called for List-trait events on ``model`` (a ProjectModel)

        This notification handler is connected and disconnected in the
        create_project_model_shadow and destroy_project_model_shadow functions.
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
        macro = MacroModel(simple_name=dialog.simple_name, initialized=True)
        project.macros.append(macro)


def _load_macro(project):
    fn = getOpenFileName(caption='Load macro', filter='Python Macros (*.py)')
    if not fn:
        return
    # Read MacroModel
    macro_model = read_macro(fn)
    # Set the scene model title
    macro_model.simple_name = op.splitext(op.basename(fn))[0]
    project.macros.append(macro_model)


def _add_scene(project):
    """ Add a scene to the associated project
    """
    dialog = SceneHandleDialog()
    if dialog.exec() == QDialog.Accepted:
        # XXX: TODO check for existing
        scene = SceneModel(simple_name=dialog.simple_name, initialized=True)
        project.scenes.append(scene)


def _load_scene(project):
    fn = getOpenFileName(caption='Load scene', filter='SVG Files (*.svg)')
    if not fn:
        return
    # Read SceneModel
    scene_model = read_scene(fn)
    scene_model.simple_name = op.splitext(op.basename(fn))[0]
    project.scenes.append(scene_model)


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
        }
        server = DeviceServerModel(**traits)
        project.servers.append(server)


def _add_project(project):
    """ Add a new subproject to the associated project
    """
    dialog = NewProjectDialog()
    if dialog.exec() == QDialog.Accepted:
        # XXX: TODO check for existing
        model = ProjectModel(simple_name=dialog.simple_name, initialized=True)
        project.subprojects.append(model)
