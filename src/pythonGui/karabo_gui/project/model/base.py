#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from abc import abstractmethod

from PyQt4.QtGui import QStandardItem
from traits.api import ABCHasStrictTraits, Instance, Property

from karabo.common.project.api import BaseProjectObjectModel


class BaseProjectTreeItem(ABCHasStrictTraits):
    """ A base class for all objects which wrap ProjectModel objects for use
    in the ProjectTreeView.
    """
    # The project model object represented here
    model = Instance(BaseProjectObjectModel)

    # The QStandardItem representing this object
    qt_item = Property(Instance(QStandardItem))

    @abstractmethod
    def context_menu(self, parent_project, parent=None):
        """ Requests a menu to be used as a context menu for this item.

        :param parent_project: The ProjectModel which is the immediate parent
                               of the item which was clicked on.
        :param parent: A QObject which can be passed as a Qt object parent.
        :return: A QMenu containing contextual actions for the item.
        """

    @abstractmethod
    def _get_qt_item(self):
        """ Force subclasses to implement the Traits property getter for
        ``qt_item``
        """
