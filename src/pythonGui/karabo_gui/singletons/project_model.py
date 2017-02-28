#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from weakref import WeakValueDictionary

from PyQt4.QtCore import QAbstractItemModel, QModelIndex, Qt
from PyQt4.QtGui import QItemSelectionModel

from karabo_gui.events import broadcast_event, KaraboEventSender
from karabo_gui.project.api import (
    create_project_controller, destroy_project_controller
)
from karabo_gui.singletons.api import get_topology

TABLE_HEADER_LABELS = ["Projects"]


class ProjectViewItemModel(QAbstractItemModel):
    """ A QAbstractItemModel which mediates between our Traits-based data model
    and Qt
    """

    def __init__(self, parent=None):
        super(ProjectViewItemModel, self).__init__(parent)
        self.q_selection_model = QItemSelectionModel(self, self)

        self._traits_model = None
        self._controller = None
        self._model_index_refs = WeakValueDictionary()

    def controller_ref(self, model_index):
        """Get the controller object for a ``QModelIndex``. This is essentially
        equivalent to a weakref and might return None.

        NOTE: We're doing a rather complicated dance here with `internalId` to
        avoid PyQt's behaviour of casting pointers into Python objects, because
        those objects might now be invalid.
        """
        key = model_index.internalId()
        return self._model_index_refs.get(key)

    def insert_controller(self, controller):
        """Insert a ``QModelIndex`` for the given ``controller``
        """
        row = self._controller_row(controller)
        index = self.createIndex(row, 0, controller)
        if index.isValid():
            try:
                self.beginInsertRows(index, 0, controller.rows()-1)
            finally:
                self.endInsertRows()

    def remove_controller(self, controller):
        """Remove the associated ``QModelIndex`` of the given ``controller``
        from the model
        """
        row = self._controller_row(controller)
        index = self.createIndex(row, 0, controller)
        if index.isValid():
            try:
                self.beginRemoveRows(index, 0, controller.rows()-1)
            finally:
                self.endRemoveRows()

    @property
    def traits_data_model(self):
        """ Return the project object at the root of the hierarchy
        """
        return self._traits_model

    @traits_data_model.setter
    def traits_data_model(self, model):
        """ Set the ProjectModel instance that we're presenting to Qt
        """
        # Clean up any previously created controllers
        if self._controller is not None:
            destroy_project_controller(self._controller)
            get_topology().clear_project_devices()
            self.q_selection_model.clearSelection()

        self._cleanup_project()

        self._traits_model = model
        try:
            self.beginResetModel()
            if model is not None:
                self._controller = create_project_controller(
                    model=model, parent=None, _qt_model=self)
            else:
                self._controller = None
        finally:
            self.endResetModel()

    # ----------------------------
    # private methods

    def _cleanup_project(self):
        """ Clean up the ``self._traits_model`` properly which means trigger
        certain events"""
        if self._traits_model is None:
            return

        for scene in self._traits_model.scenes:
            broadcast_event(KaraboEventSender.RemoveSceneView,
                            {'model': scene})

        for macro in self._traits_model.macros:
            broadcast_event(KaraboEventSender.RemoveMacro, {'model': macro})

    def _controller_row(self, controller):
        """Return the row for the given ``controller``

        Note: The row position needs to be fetched from the parent of the given
        ``controller``
        """
        parent_controller = controller.parent
        if parent_controller is None:
            return 0
        else:
            return parent_controller.children.index(controller)

    # ----------------------------
    # Qt methods

    def createIndex(self, row, column, controller):
        """Prophalaxis for QModelIndex.internalPointer...

        We need a nice way to get back to our controller objects from a
        QModelIndex. So, we store controller instances in model indices, but
        indirectly. This is because QModelIndex is not a strong reference AND
        these objects will tend to outlive the controller objects which they
        reference. So the solution is to use a WeakValueDictionary as
        indirection between Qt and our model layer.

        Awesome. QAbstractItemModel can go DIAF.
        """
        key = id(controller)
        if key not in self._model_index_refs:
            self._model_index_refs[key] = controller
        return super(ProjectViewItemModel, self).createIndex(row, column, key)

    def flags(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return Qt.NoItemFlags

        return super(ProjectViewItemModel, self).flags(index)

    def data(self, index, role=Qt.DisplayRole):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return

        controller = self.controller_ref(index)
        if controller is None:
            return

        ui_data = controller.ui_data
        if role == Qt.DisplayRole:
            return controller.ui_item_text()
        elif role == Qt.DecorationRole:
            return ui_data.icon
        elif role == Qt.ForegroundRole:
            return ui_data.brush
        elif role == Qt.FontRole:
            return ui_data.font
        elif role == Qt.CheckStateRole:
            if ui_data.checkable:
                return ui_data.check_state

    def index(self, row, column, parent=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        if self._controller is None:
            return QModelIndex()

        if not self.hasIndex(row, column, parent):
            return QModelIndex()

        if not parent.isValid():
            return self.createIndex(0, 0, self._controller)
        else:
            parent_controller = self.controller_ref(parent)

        if parent_controller is None:
            return QModelIndex()

        child_controller = parent_controller.child(row)
        if child_controller is not None:
            return self.createIndex(row, column, child_controller)

        return QModelIndex()

    def parent(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if self._controller is None:
            return QModelIndex()

        if not index.isValid():
            return QModelIndex()

        child_controller = self.controller_ref(index)
        if child_controller is None:
            return QModelIndex()

        parent_controller = child_controller.parent
        if parent_controller is None:
            return QModelIndex()

        return self.createIndex(self._controller_row(parent_controller), 0,
                                parent_controller)

    def headerData(self, section, orientation, role):
        """Reimplemented function of QAbstractItemModel.
        """
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return TABLE_HEADER_LABELS[section]

    def columnCount(self, parentIndex=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        return len(TABLE_HEADER_LABELS)

    def rowCount(self, parent=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        if self._controller is None or parent.column() > 0:
            return 0

        if not parent.isValid():
            return 1
        else:
            parent_controller = self.controller_ref(parent)

        if parent_controller is None:
            return 0
        return parent_controller.rows()
