#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 3, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import json
from weakref import WeakValueDictionary

from PyQt4.QtCore import (
    pyqtSlot, QAbstractItemModel, QMimeData, QModelIndex, Qt)
from PyQt4.QtGui import QBrush, QColor

from karabo.middlelayer import AccessMode, VectorHash
from karabo.common.api import State
from karabo_gui.const import OK_COLOR, ERROR_COLOR_ALPHA
import karabo_gui.globals as krb_globals
from karabo_gui.schema import (
    ChoiceOfNodes, Dummy, EditableAttributeInfo, ImageNode, Schema, SlotNode,
    get_editable_attributes
)
from karabo_gui.treewidget.parametertreewidget import getDeviceBox
from karabo_gui.treewidget.utils import get_icon
from karabo_gui.widget import DisplayWidget, EditableWidget


def _get_child_names(descriptor):
    """Return all the names of a descriptor's accessible children.

    In the case of a `Schema`-derived descriptor, this is a list of properties.
    For a leaf-node, this is a list of attribute names.
    """
    level = krb_globals.GLOBAL_ACCESS_LEVEL
    if isinstance(descriptor, Schema):
        return [key for key, desc in descriptor.dict.items()
                if desc.requiredAccessLevel <= level]

    # `get_editable_attributes` returns an empty list if descriptor is None
    return get_editable_attributes(descriptor)


class ConfigurationTreeModel(QAbstractItemModel):
    def __init__(self, parent=None):
        super(ConfigurationTreeModel, self).__init__(parent)
        self._configuration = None
        self._model_index_refs = WeakValueDictionary()

    # ----------------------------
    # Public interface

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

    def index_ref(self, index):
        """Get the object from a ``QModelIndex`` which was created by this
        model. This is essentially equivalent to a weakref and _might_ return
        None.

        NOTE: We're doing a rather complicated dance here with `internalId` to
        avoid PyQt's behaviour of casting pointers into Python objects, because
        those objects might now be invalid.
        """
        key = index.internalId()
        return self._model_index_refs.get(key)

    # ----------------------------
    # Private interface

    def _attribute_data(self, attr_info, role, column, row):
        """data() implementation for property attributes
        """
        box = attr_info.parent
        if box is None:
            return

        name = attr_info.names[row]
        if column == 0:
            if role == Qt.DisplayRole:
                return name
            elif role == Qt.DecorationRole:
                return get_icon(box.descriptor)
        elif column == 1:
            if role == Qt.DisplayRole:
                value = getattr(box.descriptor, name)
                return str(value)

    def _box_data(self, box, role, column):
        """data() implementation for properties
        """
        if column == 0:
            if role == Qt.DisplayRole:
                return box.descriptor.displayedName
            elif role == Qt.DecorationRole:
                return get_icon(box.descriptor)
        elif column == 1:
            if role == Qt.DisplayRole:
                if isinstance(box.descriptor, Schema):
                    return ''

                value = box.value
                if isinstance(value, (Dummy, bytes, bytearray)):
                    return ''
                return str(value)

    def _box_row(self, box):
        """Return the row for the given ``box``
        """
        if (self._configuration is None or
                self._configuration.descriptor is None):
            return 0

        # Make sure we were actually passed a `Box` and not an
        # `EditableAttributeInfo` instance
        assert not isinstance(box, EditableAttributeInfo)

        parent_path, box_key = box.path[:-1], box.path[-1]
        if parent_path:
            parent = self._configuration.getBox(parent_path)
        else:
            parent = self._configuration

        properties = _get_child_names(parent.descriptor)
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
        """
        def recurse(box, parent):
            descriptor = box.descriptor
            if descriptor is None:
                return
            names = _get_child_names(descriptor)
            for row, name in enumerate(names):
                sub_box = getattr(box.boxvalue, name)
                if sub_box is None:
                    continue
                sub_desc = sub_box.descriptor
                sub_index = self.index(row, 0, parent)
                if sub_desc.allowedStates is not None:
                    self.dataChanged.emit(sub_index, sub_index)
                if isinstance(sub_desc, Schema):
                    recurse(sub_box, sub_index)

        recurse(self._configuration, QModelIndex())

    # ----------------------------
    # Qt methods

    def columnCount(self, parentIndex=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        return 2  # Name, Value

    def createIndex(self, row, col, obj):
        """Prophylaxis for QModelIndex.internalPointer...

        QModelIndex stores internalPointer references weakly. This can be
        highly dangerous when a model index outlives the data it's referencing.
        As with ProjectViewItemModel, we maintain a WeakValueDictionary of
        references to avoid getting into sticky situations.
        """
        key = id(obj)
        if key not in self._model_index_refs:
            self._model_index_refs[key] = obj
        return super(ConfigurationTreeModel, self).createIndex(row, col, key)

    def data(self, index, role=Qt.DisplayRole):
        """Reimplemented function of QAbstractItemModel.
        """
        # short circuit for the state color background
        column = index.column()
        if column == 1 and role == Qt.BackgroundRole:
            state = self.configuration.boxvalue.state.value
            if isinstance(state, Dummy):
                return
            in_error = State(state) == State.ERROR
            color = ERROR_COLOR_ALPHA if in_error else OK_COLOR
            return QBrush(QColor(*color))

        # Get the index's stored object
        obj = self.index_ref(index)
        if obj is None:
            return None

        if isinstance(obj, EditableAttributeInfo):
            return self._attribute_data(obj, role, column, index.row())
        else:
            return self._box_data(obj, role, column)

    def flags(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return Qt.NoItemFlags

        flags = Qt.ItemIsEnabled | Qt.ItemIsSelectable

        obj = self.index_ref(index)
        if obj is None or isinstance(obj, EditableAttributeInfo):
            return flags

        # We know that `obj` is a `Box` now
        box = obj

        is_node = isinstance(box.descriptor, Schema)
        is_special = isinstance(box.descriptor, (SlotNode, ImageNode))
        if not is_node or is_special:
            flags |= Qt.ItemIsDragEnabled

        descriptor = box.descriptor
        if descriptor is None:
            return flags

        is_class = box.configuration.type in ('class', 'projectClass')
        is_editable_type = not is_node or isinstance(descriptor, ChoiceOfNodes)
        is_class_editable = (is_class and descriptor.accessMode in
                             (AccessMode.INITONLY, AccessMode.RECONFIGURABLE))
        is_inst_editable = (not is_class and box.isAllowed() and
                            descriptor.accessMode is AccessMode.RECONFIGURABLE)
        if is_editable_type and (is_class_editable or is_inst_editable):
            flags |= Qt.ItemIsEditable
            # XXX: Explicitly avoid the table editor!
            if isinstance(descriptor, VectorHash):
                flags &= ~Qt.ItemIsEditable

        return flags

    def headerData(self, section, orientation, role):
        """Reimplemented function of QAbstractItemModel.
        """
        names = ('Name', 'Value')
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            return names[section]

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
            parent_box = self.index_ref(parent)
            assert not isinstance(parent_box, EditableAttributeInfo)

        if parent_box is None or parent_box.descriptor is None:
            return QModelIndex()

        names = _get_child_names(parent_box.descriptor)
        if isinstance(parent_box.descriptor, Schema):
            # Nodes have properties as children
            obj = getattr(parent_box.boxvalue, names[row])
        else:
            # Leaves have attributes as children
            obj = parent_box.attributeInfo
        return self.createIndex(row, column, obj)

    def mimeData(self, indices):
        """Reimplemented function of QAbstractItemModel.

        NOTE: flags() is controlling which indices show up in this method. We
        don't have to do so many checks here.
        """
        if len(indices) == 0 or self._configuration.type == 'class':
            return None

        dragged = []
        for index in indices:
            if index.column() != 0:
                continue  # Ignore other columns (all columns are the same box)

            box = self.index_ref(index)
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

        child_obj = self.index_ref(index)
        if child_obj is None:
            return QModelIndex()

        # Handle attribute indices correctly
        if isinstance(child_obj, EditableAttributeInfo):
            parent_box = child_obj.parent
            if parent_box is None:
                return QModelIndex()
            parent_path = parent_box.path
        else:
            parent_path = child_obj.path[:-1]

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
            box = self.index_ref(parent)

        # `box` might be an ``EditableAttributeInfo``!
        if box is None or isinstance(box, EditableAttributeInfo):
            return 0

        is_class = self._configuration.type != 'device'
        descriptor = box.descriptor
        if isinstance(descriptor, Schema) or is_class:
            # Schemas and class properties can have children
            return len(_get_child_names(descriptor))

        # otherwise no children
        return 0
