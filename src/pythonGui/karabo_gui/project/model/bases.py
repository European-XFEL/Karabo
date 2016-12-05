#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from abc import abstractmethod

from PyQt4.QtGui import QStandardItem
from traits.api import (
    ABCHasStrictTraits, Instance, on_trait_change, Property
)

from karabo.common.project.api import BaseProjectObjectModel


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

    @on_trait_change("model.simple_name")
    def simple_name_change(self):
        if not self.is_ui_initialized():
            return
        self.qt_item.setText(self.model.simple_name)
