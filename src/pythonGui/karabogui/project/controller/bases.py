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
from abc import abstractmethod

from qtpy.QtCore import QAbstractItemModel, Qt
from qtpy.QtGui import QBrush, QFont, QIcon
from traits.api import (
    ABCHasStrictTraits, Bool, Callable, Dict, Enum, HasStrictTraits, Instance,
    Int, List, Property, String, WeakRef, on_trait_change)

from karabo.common.api import InstanceStatus
from karabo.common.project.api import BaseProjectObjectModel
from karabogui.binding.api import ProxyStatus


class ProjectControllerUiData(HasStrictTraits):
    """ A data class which contains all necessary data for the Qt item to
    display
    """
    icon = Instance(QIcon, args=())
    font = Instance(QFont, args=())
    brush = Instance(QBrush, args=())
    checkable = Bool(False)
    check_state = Int(Qt.Checked)
    status = Enum(*ProxyStatus)
    instance_status = Enum(*InstanceStatus)


class BaseProjectController(ABCHasStrictTraits):
    """ A base class for all objects which control ProjectModel objects for use
    in the ProjectTreeView.
    """
    # The project model object controlled here
    model = Instance(BaseProjectObjectModel)

    # A back-reference to our parent controller
    parent = WeakRef('BaseProjectController', allow_none=True)

    # Name to be displayed in the item view
    display_name = Property(String)

    # Easy access display data for model item
    ui_data = Instance(ProjectControllerUiData)

    # Reference the QAbstractItemModel
    _qt_model = WeakRef(QAbstractItemModel)

    @abstractmethod
    def context_menu(self, project_controller, parent=None):
        """ Requests a menu to be used as a context menu for this item.

        :param project_controller: The ProjectController which is the immediate
                                   parent of the item which was clicked on.
        :param parent: A QObject which can be passed as a Qt object parent.
        :return: A QMenu containing contextual actions for the item.
        """

    @abstractmethod
    def create_ui_data(self):
        """Must be implemented by derived classes to return a
        `ProjectControllerUiData` instance representing this controller. The
        result will be stored in the ``ui_data`` trait.
        """

    def double_click(self, project_controller, parent=None):
        """ Handles a double click event on this item.

        :param project_controller: The ProjectController which is the immediate
                                   parent of the item which was clicked on.
        :param parent: A QObject which can be passed as a Qt object parent.
        """

    def single_click(self, project_controller, parent=None):
        """ Handles a single click event on this item.

        :param project_controller: The ProjectController which is the immediate
                                   parent of the item which was clicked on.
        :param parent: A QObject which can be passed as a Qt object parent.
        """

    def delete_press(self, project_controller, parent=None):
        """ Handles a delete key press event on this item.

        :param project_controller: The ProjectController which is the immediate
                                   parent of the item which was pressed on.
        :param parent: A QObject which can be passed as a Qt object parent.
        """

    def child(self, index):
        """Returns a child of this controller.

        :param index: An index into the list of this controller's children
        :returns: A BaseProjectController instance or None
        """
        return None

    def rows(self):
        """Returns the number of rows 'under' this controller in the project
        tree view.
        """
        return -1

    def ui_item_text(self):
        """The name for this controller's item in the GUI view
        """
        if self.model.modified:
            return f'*{self.display_name}'
        else:
            return self.display_name

    @on_trait_change('model.modified')
    def _update_label_style(self):
        brush = QBrush(self.ui_data.brush)
        if self.model.modified:
            brush.setColor(Qt.blue)
        else:
            brush.setColor(Qt.black)
        self.ui_data.brush = brush

    @on_trait_change('model.simple_name')
    def _update_label(self):
        """ Whenever ``simple_name`` is modified the UI should repaint
        """
        if self._qt_model is not None:
            self._qt_model.controller_data_update(self, roles=[Qt.DisplayRole])

    @on_trait_change('ui_data.icon,ui_data.brush,ui_data.check_state')
    def _request_repaint(self):
        if self._qt_model is not None:
            self._qt_model.controller_data_update(
                self, roles=[Qt.DecorationRole, Qt.FontRole, Qt.ForegroundRole,
                             Qt.CheckStateRole])

    @on_trait_change('ui_data.instance_status')
    def _update_instance_info(self):
        if self._qt_model is not None:
            self._qt_model.controller_data_update(
                self, column=1, roles=[Qt.DecorationRole])

    def _get_display_name(self):
        """Traits property getter for ``display_name``
        """
        return self.model.simple_name

    def _ui_data_default(self):
        """Traits default initializer for `ui_data`
        """
        return self.create_ui_data()


class BaseProjectGroupController(BaseProjectController):
    """ A base class for all project controller objects which have children.
    """
    # A factory for children controllers
    child_create = Callable
    # A callable which can gracefully destroy a child controller
    child_destroy = Callable
    # Child controllers
    children = List(Instance(BaseProjectController))
    # Dictionary for fast lookups during removal
    _child_map = Dict  # {model: controller}

    def child(self, index):
        """Return child at given ``index``
        """
        return self.children[index]

    def rows(self):
        """Return number of rows
        """
        return len(self.children)

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
        for model in removed:
            controller = self._child_map[model]
            # NOTE: Always use a context manager when modifying self.children!
            with self._qt_model.removal_context(controller):
                self.children.remove(controller)
            self.child_destroy(controller)

        # Synchronize the GUI with the Traits model
        additions = [self.child_create(model=model, parent=self,
                                       _qt_model=self._qt_model)
                     for model in added]
        self._update_ui_children(additions)

    def _children_items_changed(self, event):
        """ Maintain ``_child_map`` by watching item events on ``children``

        This is a static notification handler which is connected automatically
        by Traits.
        """
        for controller in event.removed:
            del self._child_map[controller.model]

        for controller in event.added:
            self._child_map[controller.model] = controller

    def _update_ui_children(self, additions):
        """ Propagate changes from the Traits model to the Qt item model.
        """
        if not additions:
            return

        # First and last indices, INCLUSIVE (thus the '- 1')
        first = len(self.children)
        last = first + len(additions) - 1
        # NOTE: Always use a context manager when modifying self.children!
        with self._qt_model.insertion_context(self, first, last):
            self.children.extend(additions)
