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
import contextlib

from qtpy.QtCore import QAbstractItemModel, QModelIndex
from traits.api import HasStrictTraits, WeakRef


class _UpdateContext(HasStrictTraits):
    """A context manager that can be handed off to code which doesn't need to
    know that it's dealing with a Qt QAbstractItemModel.
    """
    item_model = WeakRef(QAbstractItemModel)

    @contextlib.contextmanager
    def reset_context(self):
        """Provide a context whenever the system tree was cleared then the
        Qt model needs to be reseted.

        NOTE: This method is a context manager wraps the insertion with calls
        to ``QAbstractItemModel.beginResetModel`` and
        ``QAbstractItemModel.endResetModel`` (See Qt documentation)
        """
        try:
            self.item_model.beginResetModel()
            yield
        finally:
            self.item_model.endResetModel()

    @contextlib.contextmanager
    def insertion_context(self, parent_node, first, last):
        """Provide a context for the addition of multiple children under a
        single parent item.

        NOTE: This method is a context manager wraps the insertion with calls
        to ``QAbstractItemModel.beginInsertRows`` and
        ``QAbstractItemModel.endInsertRows`` (See Qt documentation)
        """
        parent_index = self.item_model.createIndex(parent_node.row(), 0,
                                                   parent_node)

        def gen():
            try:
                self.item_model.beginInsertRows(parent_index, first, last)
                yield
            finally:
                self.item_model.endInsertRows()

        if parent_index.isValid():
            yield from gen()
        else:
            yield

    @contextlib.contextmanager
    def removal_context(self, tree_node):
        """Provide a context for the removal of a single item from the model.

        NOTE: This method is a context manager which wraps the removal of an
        item with ``QAbstractItemModel.beginRemoveRows`` and
        ``QAbstractItemModel.endRemoveRows`` (See Qt documentation)
        """
        node_row = tree_node.row()
        index = self.item_model.createIndex(node_row, 0, tree_node)
        if index.isValid():
            parent_index = index.parent()
        else:
            parent_index = QModelIndex()

        def gen():
            try:
                self.item_model.beginRemoveRows(parent_index, node_row,
                                                node_row)
                yield
            finally:
                self.item_model.endRemoveRows()

        if parent_index.isValid():
            yield from gen()
        else:
            yield

    @contextlib.contextmanager
    def insert_root_context(self, first, last):
        """Provide a context for the addition of a children under the root"""
        try:
            self.item_model.beginInsertRows(QModelIndex(), first, last)
            yield
        finally:
            self.item_model.endInsertRows()

    @contextlib.contextmanager
    def remove_root_context(self, first, last):
        """Provide a context for the removal of a children under the root"""
        try:
            self.item_model.beginRemoveRows(QModelIndex(), first, last)
            yield
        finally:
            self.item_model.endRemoveRows()

    @contextlib.contextmanager
    def removal_children_context(self, tree_node):
        """Provide a context for the removal of a single item from the model.
        """
        node_row = tree_node.row()
        index = self.item_model.createIndex(node_row, 0, tree_node)

        def gen():
            try:
                self.item_model.beginRemoveRows(index, 0,
                                                len(tree_node.children) - 1)
                yield
            finally:
                self.item_model.endRemoveRows()

        if index.isValid() and tree_node.children:
            yield from gen()
        else:
            yield

    @contextlib.contextmanager
    def layout_context(self):
        """Provide a context for layout change announcements
        """
        try:
            self.item_model.layoutAboutToBeChanged.emit(
                [], QAbstractItemModel.VerticalSortHint)
            yield
        finally:
            self.item_model.layoutChanged.emit()
