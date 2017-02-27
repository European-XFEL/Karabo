#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from abc import abstractmethod

from PyQt4.QtCore import QAbstractItemModel, Qt
from PyQt4.QtGui import QBrush, QFont, QIcon
from traits.api import (
    ABCHasStrictTraits, HasStrictTraits, Bool, Callable, Dict, Instance, Int,
    List, Property, String, WeakRef, on_trait_change
)

from karabo.common.project.api import BaseProjectObjectModel
from karabo_gui.project.utils import show_no_configuration


class ProjectControllerUiData(HasStrictTraits):
    """ A data class which contains all necessary data for the Qt item to
    display
    """
    icon = Instance(QIcon, args=())
    font = Instance(QFont, args=())
    brush = Instance(QBrush, args=())
    checkable = Bool(False)
    check_state = Int(Qt.Unchecked)


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
    def context_menu(self, parent_project, parent=None):
        """ Requests a menu to be used as a context menu for this item.

        :param parent_project: The ProjectModel which is the immediate parent
                               of the item which was clicked on.
        :param parent: A QObject which can be passed as a Qt object parent.
        :return: A QMenu containing contextual actions for the item.
        """

    @abstractmethod
    def create_ui_data(self):
        """Must be implemented by derived classes to return a
        `ProjectControllerUiData` instance representing this controller. The
        result will be stored in the ``ui_data`` trait.
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
            return '*{}'.format(self.display_name)
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
            self._qt_model.layoutChanged.emit()

    @on_trait_change('ui_data.icon,ui_data.brush,ui_data.check_state')
    def _request_repaint(self):
        if self._qt_model is not None:
            self._qt_model.layoutChanged.emit()

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
    _child_map = Dict  # dictionary for fast lookups during removal

    def child(self, index):
        """Return child at given ``index``
        """
        return self.children[index]

    def rows(self):
        """Return number of rows
        """
        return len(self.children)

    def single_click(self, parent_project, parent=None):
        show_no_configuration()

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
            self._qt_model.remove_controller(controller)
            self.children.remove(controller)
            self.child_destroy(controller)

        additions = [self.child_create(model=model, parent=self,
                                       _qt_model=self._qt_model)
                     for model in added]
        self.children.extend(additions)

        # Synchronize the GUI with the Traits model
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
        for item in additions:
            self._qt_model.insert_controller(item)
