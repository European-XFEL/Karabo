#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from abc import abstractmethod

from PyQt4.QtCore import Qt
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

    # Qt ItemDataRole constants
    MODEL_REF_ITEM_ROLE = Qt.UserRole + 1

    @abstractmethod
    def context_menu(self, parent):
        """ Return a QMenu to be used as a context model for this item
        """

    @abstractmethod
    def _get_qt_item(self):
        """ Force subclasses to implement the Traits property getter for
        ``qt_item``
        """
