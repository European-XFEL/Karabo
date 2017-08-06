#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 3, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from itertools import islice
from weakref import WeakValueDictionary

from PyQt4.QtCore import QAbstractItemModel, QModelIndex, Qt

import karabo_gui.icons as icons
from karabo_gui.schema import Schema


class ConfigurationTreeModel(QAbstractItemModel):
    def __init__(self, parent=None):
        super(ConfigurationTreeModel, self).__init__(parent)
        self._configuration = None
        self._model_index_refs = WeakValueDictionary()

    # ----------------------------
    # Public interface

    def box_ref(self, model_index):
        """Get the Box object for a ``QModelIndex``. This is essentially
        equivalent to a weakref and might return None.

        NOTE: We're doing a rather complicated dance here with `internalId` to
        avoid PyQt's behaviour of casting pointers into Python objects, because
        those objects might now be invalid.
        """
        key = model_index.internalId()
        return self._model_index_refs.get(key)

    @property
    def configuration(self):
        """Return the `Configuration` instance that we're presenting to Qt
        """
        return self._configuration

    @configuration.setter
    def configuration(self, conf):
        """Set the `Configuration` instance that we're presenting to Qt
        """
        self._model_index_refs.clear()

        try:
            self.beginResetModel()
            self._configuration = conf
        finally:
            self.endResetModel()

    # ----------------------------
    # Private interface

    def _box_row(self, box):
        """Return the row for the given ``box``
        """
        if (self._configuration is None or
                self._configuration.descriptor is None):
            return 0

        parent_path, box_key = box.path[:-1], box.path[-1]
        if parent_path:
            parent = self._configuration.getBox(parent_path)
        else:
            parent = self._configuration

        properties = list(parent.descriptor.dict.keys())
        return properties.index(box_key)

    # ----------------------------
    # Qt methods

    def columnCount(self, parentIndex=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        return 1

    def createIndex(self, row, col, box):
        """Prophalaxis for QModelIndex.internalPointer...

        QModelIndex stores internalPointer references weakly. This can be
        highly dangerous when a model index outlives the data it's referencing.
        As with ProjectViewItemModel, we maintain a WeakValueDictionary of
        references to avoid getting into sticky situations.
        """
        key = id(box)
        if key not in self._model_index_refs:
            self._model_index_refs[key] = box
        return super(ConfigurationTreeModel, self).createIndex(row, col, key)

    def data(self, index, role=Qt.DisplayRole):
        """Reimplemented function of QAbstractItemModel.
        """
        box = self.box_ref(index)
        if box is None:
            return None

        column = index.column()
        if column == 0 and role == Qt.DisplayRole:
            return box.path[-1]
        elif column == 0 and role == Qt.DecorationRole:
            return icons.undefined

    def flags(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return Qt.NoItemFlags

        return super(ConfigurationTreeModel, self).flags(index)

    def headerData(self, section, orientation, role):
        """Reimplemented function of QAbstractItemModel.
        """
        if role == Qt.DisplayRole:
            if orientation == Qt.Horizontal and section == 0:
                return "Name"

    def index(self, row, column, parent=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        if self._configuration is None:
            return QModelIndex()

        if not self.hasIndex(row, column, parent):
            return QModelIndex()

        if not parent.isValid():
            parent_box = self._configuration
        else:
            parent_box = self.box_ref(parent)

        if parent_box is None or parent_box.descriptor is None:
            return QModelIndex()

        # descriptor.dict is an OrderedDict of (path, descriptor) items
        key = next(islice(parent_box.descriptor.dict, row, None))
        box = getattr(parent_box.boxvalue, key)
        return self.createIndex(row, column, box)

    def mimeData(self, indices):
        """Reimplemented function of QAbstractItemModel.
        """
        return None

    def parent(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if self._configuration is None:
            return QModelIndex()

        if not index.isValid():
            return QModelIndex()

        child_box = self.box_ref(index)
        if child_box is None:
            return QModelIndex()

        parent_path = child_box.path[:-1]
        if not parent_path:
            return QModelIndex()

        parent_box = self._configuration.getBox(parent_path)
        return self.createIndex(self._box_row(parent_box), 0, parent_box)

    def rowCount(self, parent=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        if not parent.isValid():
            box = self._configuration
        else:
            box = self.box_ref(parent)

        if box is None or box.descriptor is None:
            return 0

        descriptor = box.descriptor
        if not isinstance(descriptor, Schema):
            return 0

        return len(descriptor.dict)
