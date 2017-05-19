#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from contextlib import contextmanager
from weakref import WeakValueDictionary

from PyQt4.QtCore import QAbstractItemModel, QModelIndex, Qt
from PyQt4.QtGui import QItemSelectionModel

from karabo.common.api import walk_traits_object
from karabo.common.project.api import MacroModel
from karabo.common.scenemodel.api import SceneModel
from karabo_gui.events import (broadcast_event, KaraboEventSender,
                               register_for_broadcasts)
from karabo_gui.indicators import get_alarm_icon, get_state_icon_for_status
from karabo_gui.project.controller.build import (
    create_project_controller, destroy_project_controller)
from karabo_gui.project.controller.device import DeviceInstanceController
from karabo_gui.project.utils import show_no_configuration
from karabo_gui.singletons.api import get_topology

TABLE_HEADER_LABELS = ["Projects", "", ""]

PROJECT_COLUMN = 0
STATUS_COLUMN = 1
ALARM_COLUMN = 2


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

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary for self due to the fact that the singleton mediator
        # object and `self` are being destroyed when the GUI exists
        register_for_broadcasts(self)

    def controller_ref(self, model_index):
        """Get the controller object for a ``QModelIndex``. This is essentially
        equivalent to a weakref and might return None.

        NOTE: We're doing a rather complicated dance here with `internalId` to
        avoid PyQt's behaviour of casting pointers into Python objects, because
        those objects might now be invalid.
        """
        key = model_index.internalId()
        return self._model_index_refs.get(key)

    @contextmanager
    def insertion_context(self, parent_controller, first, last):
        """Provide a context for the addition of multiple children under a
        single parent item.

        NOTE: This method is a context manager wraps the insertion with calls
        to ``QAbstractItemModel.beginInsertRows`` and
        ``QAbstractItemModel.endInsertRows`` (See Qt documentation)
        """
        parent_row = self._controller_row(parent_controller)
        parent_index = self.createIndex(parent_row, PROJECT_COLUMN,
                                        parent_controller)
        if parent_index.isValid():
            try:
                self.beginInsertRows(parent_index, first, last)
                yield
            finally:
                self.endInsertRows()

    @contextmanager
    def removal_context(self, controller):
        """Provide a context for the removal of a single item from the model.

        NOTE: This method is a context manager which wraps the removal of an
        item with ``QAbstractItemModel.beginRemoveRows`` and
        ``QAbstractItemModel.endRemoveRows`` (See Qt documentation)
        """
        index_row = self._controller_row(controller)
        index = self.createIndex(index_row, PROJECT_COLUMN, controller)
        if index.isValid():
            parent_index = index.parent()
            if parent_index.isValid():
                try:
                    row = self._controller_row(controller)
                    self.beginRemoveRows(parent_index, row, row)
                    yield
                finally:
                    self.endRemoveRows()

    @property
    def root_controller(self):
        """ Return the project controller object at the root of the hierarchy
        """
        return self._controller

    @property
    def traits_data_model(self):
        """ Return the project object at the root of the hierarchy

        XXX: This should be renamed to ``root_model``
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
            show_no_configuration()

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

        models = []

        def visitor(obj):
            """Find all macros and scenes"""
            if isinstance(obj, (MacroModel, SceneModel)):
                models.append(obj)

        # Request that views for every macro and scene be closed
        walk_traits_object(self._traits_model, visitor)
        broadcast_event(KaraboEventSender.RemoveProjectModelViews,
                        {'models': models})

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

    def _update_alarm_type(self, device_id, alarm_type):
        """Update alarm qt item for the given ``device_id``
        """
        if self._controller is None:
            return

        # Walk tree to find DeviceInstanceController with given ``device_id``
        device_controller = None

        def visitor(obj):
            nonlocal device_controller
            if (isinstance(obj, DeviceInstanceController) and
                    obj.model.instance_id == device_id):
                device_controller = obj

        walk_traits_object(self._controller, visitor)

        if device_controller is not None:
            device_controller.ui_data.alarm_type = alarm_type

    # ----------------------------
    # Qt methods

    def karaboBroadcastEvent(self, event):
        if event.sender is KaraboEventSender.AlarmDeviceUpdate:
            data = event.data
            device_id = data.get('deviceId')
            alarm_type = data.get('alarm_type')
            self._update_alarm_type(device_id, alarm_type)
        return False

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

        column = index.column()
        ui_data = controller.ui_data
        if column == PROJECT_COLUMN:
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
        elif isinstance(controller, DeviceInstanceController):
            if column == ALARM_COLUMN and role == Qt.DecorationRole:
                return get_alarm_icon(ui_data.alarm_type)
            elif column == STATUS_COLUMN and role == Qt.DecorationRole:
                return get_state_icon_for_status(ui_data.status)

    def index(self, row, column, parent=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        if self._controller is None:
            return QModelIndex()

        if not self.hasIndex(row, column, parent):
            return QModelIndex()

        if not parent.isValid():
            return self.createIndex(row, column, self._controller)
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

        return self.createIndex(self._controller_row(parent_controller),
                                PROJECT_COLUMN, parent_controller)

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
        # Only column 0 has children
        if self._controller is None or parent.column() > 0:
            return 0

        if not parent.isValid():
            return 1
        else:
            parent_controller = self.controller_ref(parent)

        if parent_controller is None:
            return 0
        return parent_controller.rows()
