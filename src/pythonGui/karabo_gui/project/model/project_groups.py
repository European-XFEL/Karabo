#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import weakref

from PyQt4.QtGui import QAction, QMenu, QStandardItem
from traits.api import Callable, Dict, Instance, List, String

from karabo.common.project.api import ProjectModel
from karabo_gui import icons
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
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
        filler(menu)
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

        additions = [self.child_factory(model=model) for model in event.added]
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


def _fill_macros_menu(menu):
    add_action = QAction('Add macro', menu)
    load_action = QAction('Load macro...', menu)
    menu.addAction(add_action)
    menu.addAction(load_action)


def _fill_scenes_menu(menu):
    add_action = QAction('Add scene', menu)
    open_action = QAction('Load scene...', menu)
    menu.addAction(add_action)
    menu.addAction(open_action)


def _fill_servers_menu(menu):
    add_action = QAction('Add server', menu)
    remove_all_action = QAction('Delete all', menu)
    remove_selected_action = QAction('Delete selected', menu)
    menu.addAction(add_action)
    menu.addAction(remove_all_action)
    menu.addAction(remove_selected_action)


def _fill_subprojects_menu(menu):
    add_action = QAction('Add project', menu)
    menu.addAction(add_action)
