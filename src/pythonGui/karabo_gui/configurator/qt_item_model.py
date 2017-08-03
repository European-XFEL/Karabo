#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 3, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import json
from weakref import WeakValueDictionary

from PyQt4.QtCore import (
    pyqtSlot, QAbstractItemModel, QMimeData, QModelIndex, Qt)

from karabo.middlelayer import AccessMode
import karabo_gui.globals as krb_globals
import karabo_gui.icons as icons
from karabo_gui.schema import Dummy, Schema, SlotNode
from karabo_gui.treewidget.parametertreewidget import getDeviceBox
from karabo_gui.widget import DisplayWidget, EditableWidget


def _get_property_names(node_descriptor):
    """Filter by currently accessible properties
    """
    level = krb_globals.GLOBAL_ACCESS_LEVEL
    assert isinstance(node_descriptor, Schema)
    return [key for key, desc in node_descriptor.dict.items()
            if desc.requiredAccessLevel <= level]


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
        oldconf = self._configuration
        if oldconf is not None:
            oldconf.signalUpdateComponent.disconnect(self._config_update)
            if oldconf.type == 'device':
                sig = oldconf.boxvalue.state.signalUpdateComponent
                sig.disconnect(self._state_update)

        try:
            self.beginResetModel()
            self._model_index_refs.clear()
            self._configuration = conf
        finally:
            self.endResetModel()

        if conf is not None:
            conf.signalUpdateComponent.connect(self._config_update)
            if conf.type == 'device':
                sig = conf.boxvalue.state.signalUpdateComponent
                sig.connect(self._state_update)

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

        properties = _get_property_names(parent.descriptor)
        return properties.index(box_key)

    @pyqtSlot(object, object, object)
    def _config_update(self, box, value, timestamp):
        """Notify the view of item updates

        XXX: This is slightly clever (always dangerous!) in that we're only
        connected to the signalUpdateComponent signal of the root Configuration
        object. Therefore, we will only be called here when a configuration is
        applied via the configuration's `fromHash` method.
        """
        last_row = self.rowCount()
        first = self.index(0, 1)
        last = self.index(last_row, 1)
        self.dataChanged.emit(first, last)

    @pyqtSlot(object, object, object)
    def _state_update(self, box, state, timestamp):
        """Respond to device instance state changes

        XXX: This will eventually enable/disable slot buttons
        """

    # ----------------------------
    # Qt methods

    def columnCount(self, parentIndex=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        return 2

    def createIndex(self, row, col, box):
        """Prophylaxis for QModelIndex.internalPointer...

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
        if column == 0:
            if role == Qt.DisplayRole:
                return box.path[-1]
            elif role == Qt.DecorationRole:
                return icons.undefined
        elif column == 1:
            if role == Qt.DisplayRole:
                if isinstance(box.descriptor, Schema):
                    return ''

                value = box.value
                if isinstance(value, (Dummy, bytes, bytearray)):
                    return ''
                return str(value)

    def flags(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return Qt.NoItemFlags

        flags = Qt.ItemIsEnabled | Qt.ItemIsSelectable
        box = self.box_ref(index)
        if box is not None:
            is_node = isinstance(box.descriptor, Schema)
            is_slot = isinstance(box.descriptor, SlotNode)
            if not is_node or is_slot:
                flags |= Qt.ItemIsDragEnabled

        return flags

    def headerData(self, section, orientation, role):
        """Reimplemented function of QAbstractItemModel.
        """
        if role == Qt.DisplayRole:
            if orientation == Qt.Horizontal:
                if section == 0:
                    return "Name"
                elif section == 1:
                    return "Value"

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

        names = _get_property_names(parent_box.descriptor)
        key = names[row]
        box = getattr(parent_box.boxvalue, key)
        return self.createIndex(row, column, box)

    def mimeData(self, indices):
        """Reimplemented function of QAbstractItemModel.
        """
        if len(indices) == 0 or self._configuration.type == 'class':
            return None

        dragged = []
        for index in indices:
            if index.column() != 0:
                continue  # Ignore other columns (all columns are the same box)

            box = self.box_ref(index)
            if box is None:
                continue

            # Get the box. "box" is in the project, "realbox" the
            # one on the device. They are the same if not from a project
            realbox = getDeviceBox(box)
            if realbox.descriptor is not None:
                box = realbox

            # Collect the relevant information
            data = {
                'key': box.key(),
                'label': box.path[-1],
            }

            factory = DisplayWidget.getClass(box)
            if factory is not None:
                data['display_widget_class'] = factory.__name__
            if box.descriptor.accessMode == AccessMode.RECONFIGURABLE:
                factory = EditableWidget.getClass(box)
                if factory is not None:
                    data['edit_widget_class'] = factory.__name__
            # Add it to the list of dragged items
            dragged.append(data)

        if not dragged:
            return None

        mimeData = QMimeData()
        mimeData.setData('source_type', 'ParameterTreeWidget')
        mimeData.setData('tree_items', json.dumps(dragged))
        return mimeData

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

        return len(_get_property_names(descriptor))
