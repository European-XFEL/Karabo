#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from abc import abstractmethod

from PyQt4.QtGui import QStandardItem
from traits.api import (ABCHasStrictTraits, Callable, Dict, Instance, List,
                        Property)

from karabo.common.project.api import BaseProjectObjectModel
from karabo_gui.const import PROJECT_ITEM_MODEL_REF


class BaseProjectTreeItem(ABCHasStrictTraits):
    """ A base class for all objects which wrap ProjectModel objects for use
    in the ProjectTreeView.
    """
    # The project model object represented here
    model = Instance(BaseProjectObjectModel)

    # The QStandardItem representing this object
    qt_item = Property(Instance(QStandardItem))
    # This is where the ``qt_item`` is actually stored.
    # The Traits property caching mechanism is explicitly being avoided here.
    _qt_item = Instance(QStandardItem, allow_none=True)

    @abstractmethod
    def context_menu(self, parent_project, parent=None):
        """ Requests a menu to be used as a context menu for this item.

        :param parent_project: The ProjectModel which is the immediate parent
                               of the item which was clicked on.
        :param parent: A QObject which can be passed as a Qt object parent.
        :return: A QMenu containing contextual actions for the item.
        """

    @abstractmethod
    def create_qt_item(self):
        """ Requests a QStandardItem which represents this object
        """

    def double_click(self, parent_project, parent=None):
        """ Handles a double click event on this item.

        :param parent_project: The ProjectModel which is the immediate parent
                               of the item which was clicked on.
        :param parent: A QObject which can be passed as a Qt object parent.
        """

    def single_click(self, parent_project, parent=None):
        """ Handles a single click event on this item.

        :param parent_project: The ProjectModel which is the immediate parent
                               of the item which was clicked on.
        :param parent: A QObject which can be passed as a Qt object parent.
        """

    def is_ui_initialized(self):
        """ Returns True if ``create_qt_item()`` has been called.
        """
        return self._qt_item is not None

    def _get_qt_item(self):
        """ Traits Property getter for ``qt_item``. Caches the result of
        calling ``self.create_qt_item()`` for later access.

        NOTE: The @cached_property decorator is explicitly avoided here so that
        we maintain direct access to the cached object!
        """
        if self._qt_item is None:
            self._qt_item = self.create_qt_item()
        return self._qt_item


class BaseProjectGroupItem(BaseProjectTreeItem):
    """ A base class for all project tree objects which have children.
    """
    # A factory for shadow items wrapping children
    child_create = Callable
    # A callable which can gracefully destroy a child shadow object
    child_destroy = Callable
    # Children shadow items
    children = List(Instance(BaseProjectTreeItem))
    _child_map = Dict  # dictionary for fast lookups during removal

    def items_assigned(self, obj, name, old, new):
        """ Handles assignment to a list trait and passes the notification
        along to ``item_handler``
        """
        self.item_handler(new, old)

    def items_mutated(self, event):
        """ Handles mutation(insertion/removal) of a list trait and passes the
        notification along to ``item_handler``
        """
        self.item_handler(event.added, event.removed)

    def item_handler(self, added, removed):
        """ Called for List-trait events on ``model``
        """
        removals = []
        for model in removed:
            item_model = self._child_map[model]
            self.children.remove(item_model)
            self.child_destroy(item_model)
            removals.append(item_model)

        additions = [self.child_create(model=model) for model in added]
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
