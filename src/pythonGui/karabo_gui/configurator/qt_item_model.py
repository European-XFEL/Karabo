#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 3, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from weakref import WeakValueDictionary

from PyQt4.QtCore import pyqtSlot, QAbstractItemModel, QModelIndex, Qt
from PyQt4.QtGui import QBrush, QColor, QFont

from karabo.middlelayer import AccessMode, Assignment
from karabo.common.api import State
from karabo_gui.const import OK_COLOR, ERROR_COLOR_ALPHA
import karabo_gui.globals as krb_globals
from karabo_gui.schema import (
    Box, ChoiceOfNodes, Dummy, EditableAttributeInfo, ImageNode, Schema,
    SlotNode, VectorHash, VectorHashCellInfo, VectorHashRowInfo,
    get_editable_attributes
)
from karabo_gui.treewidget.utils import get_icon
from karabo_gui.util import dragged_configurator_items
from .utils import get_attribute_data, get_box_value, get_vector_col_value


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
        self._header_labels = ['Property', 'Current value on device', 'Value']

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

    def _box_row(self, box):
        """Return the row for the given ``box``
        """
        if (self._configuration is None or
                self._configuration.descriptor is None):
            return 0

        # Make sure we were actually passed a `Box` and not something else
        assert isinstance(box, Box)

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
        self.layoutChanged.emit()

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

    def columnCount(self, parentIndex=None):
        """Reimplemented function of QAbstractItemModel.
        """
        return len(self._header_labels)

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
            if self.configuration.type != 'device' or isinstance(state, Dummy):
                return None
            in_error = State(state) == State.ERROR
            color = ERROR_COLOR_ALPHA if in_error else OK_COLOR
            return QBrush(QColor(*color))

        # Get the index's stored object
        obj = self.index_ref(index)
        if obj is None:
            return None

        if isinstance(obj, EditableAttributeInfo):
            return self._attribute_data(obj, role, column, index.row())
        elif isinstance(obj, VectorHashCellInfo):
            return self._vector_col_data(obj, role, column)
        elif isinstance(obj, VectorHashRowInfo):
            return self._vector_row_data(obj, role, column, index.row())
        else:
            return self._box_data(obj, role, column)

    def flags(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return Qt.NoItemFlags

        # All items have these properties
        flags = Qt.ItemIsEnabled | Qt.ItemIsSelectable

        obj = self.index_ref(index)
        if obj is None:
            return flags

        # NOTE: `obj` can be a `Box` or an `EditableAttributeInfo`

        # Check for draggable rows
        descriptor = getattr(obj, 'descriptor', None)
        is_node = isinstance(descriptor, Schema)
        is_special = isinstance(descriptor, (SlotNode, ImageNode))
        if is_special or (descriptor is not None and not is_node):
            flags |= Qt.ItemIsDragEnabled

        # Below are the value flags. Ignore the first and second column
        if index.column() in (0, 1):
            return flags

        # Value-specific flags
        if isinstance(obj, EditableAttributeInfo):
            flags |= self._attribute_flags(obj)
        elif isinstance(obj, VectorHashCellInfo):
            flags |= self._vector_col_flags(obj)
        elif isinstance(obj, VectorHashRowInfo):
            flags |= self._vector_row_flags(obj)
        else:
            flags |= self._box_flags(obj)

        return flags

    def headerData(self, section, orientation, role):
        """Reimplemented function of QAbstractItemModel.
        """
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            return self._header_labels[section]

    def index(self, row, column, parent=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        if self._configuration is None:
            return QModelIndex()

        if not self.hasIndex(row, column, parent):
            return QModelIndex()

        if not parent.isValid():
            parent_obj = self._configuration
        else:
            parent_obj = self.index_ref(parent)

        if isinstance(parent_obj, Box):
            descriptor = parent_obj.descriptor
            names = _get_child_names(descriptor)
            if isinstance(descriptor, Schema):
                # Nodes have properties as children
                obj = getattr(parent_obj.boxvalue, names[row])
            elif isinstance(descriptor, VectorHash):
                obj = descriptor.rowsInfo[row]
            else:
                # Leaves have attributes as children
                obj = descriptor.attributeInfo
        elif isinstance(parent_obj, VectorHashRowInfo):
            obj = parent_obj.columns[row]
        else:
            return QModelIndex()

        return self.createIndex(row, column, obj)

    def mimeData(self, indices):
        """Reimplemented function of QAbstractItemModel.

        NOTE: flags() is controlling which indices show up in this method. We
        don't have to do so many checks here.
        """
        if len(indices) == 0 or self._configuration.type == 'class':
            return None

        # Only gather valid boxes for indices in the first column
        # (Qt passes indices for each column in a row)
        boxes = [self.index_ref(idx) for idx in indices
                 if idx.column() == 0 and self.index_ref(idx) is not None]
        return dragged_configurator_items(boxes)

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

        # Handle special indices correctly
        if isinstance(child_obj, VectorHashCellInfo):
            parent_row = child_obj.parent()
            if parent_row is None:
                return QModelIndex()
            parent_box = parent_row.parent()
            if parent_box is None:
                return QModelIndex()
            row = parent_box.descriptor.rowsInfo.index(parent_row)
            return self.createIndex(row, 0, parent_row)

        elif isinstance(child_obj, (EditableAttributeInfo, VectorHashRowInfo)):
            parent_box = child_obj.parent()
            if parent_box is None:
                return QModelIndex()
            parent_box = self._configuration.getBox(parent_box.path)
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

        if box is None:
            return 0

        # `box` might be a special marker
        if isinstance(box, (EditableAttributeInfo, VectorHashCellInfo)):
            return 0
        elif isinstance(box, VectorHashRowInfo):
            return len(box.columns)

        # From here, we know `box` is really a `Box` instance
        descriptor = box.descriptor
        if isinstance(descriptor, Schema):
            # Schemas have children
            return len(_get_child_names(descriptor))
        elif isinstance(descriptor, VectorHash):
            # VectorHash have as many children as they have rows
            value = box.value
            return 0 if isinstance(value, Dummy) else len(value)
        elif self._configuration.type != 'device':
            # class properties can have children (attributes)
            return len(_get_child_names(descriptor))

        # otherwise no children
        return 0

    def setData(self, index, value, role):
        """Reimplemented function of QAbstractItemModel.
        """
        if role != Qt.EditRole:
            return False

        # Get the index's stored object
        obj = self.index_ref(index)
        if obj is None:
            return False

        if isinstance(obj, EditableAttributeInfo):
            box = obj.parent()
            if box is None or box.descriptor is None:
                return False
            name = obj.names[index.row()]
            descriptor = box.descriptor
            setattr(descriptor, name, value)

            # Configuration changed - so project needs to be informed
            if box.configuration.type == 'projectClass':
                box.configuration.signalBoxChanged.emit()
        else:  # Normal property value setting
            box = obj
            box.signalUserChanged.emit(box, value, None)
            if box.configuration.type == "macro":
                box.set(value)
            elif box.descriptor is not None:
                if box.configuration.type == "device":
                    box.configuration.setUserValue(box, value)
                    box.configuration.sendUserValue(box)
                else:
                    box.set(value)
                    box.configuration.signalBoxChanged.emit()

        # A value was successfully set!
        return True

    # ----------------------------
    # data() and flags() helper methods

    def _attribute_data(self, attr_info, role, column, row):
        """data() implementation for property attributes"""
        name, descriptor, value = get_attribute_data(attr_info, row)
        if column == 0:
            if role == Qt.DisplayRole:
                return name
            elif role == Qt.DecorationRole:
                return get_icon(descriptor)
        elif column in (1, 2) and role == Qt.DisplayRole:
            return str(value)

    def _box_data(self, box, role, column):
        """data() implementation for properties"""
        if column == 0:
            if role == Qt.DisplayRole:
                return box.descriptor.displayedName
            elif role == Qt.FontRole:
                if box.descriptor.assignment == Assignment.MANDATORY.value:
                    font = QFont()
                    font.setBold(True)
                    return font
            elif role == Qt.DecorationRole:
                return get_icon(box.descriptor)
        elif column == 1 and role == Qt.DisplayRole:
            return str(get_box_value(box))
        elif column == 2 and role == Qt.DisplayRole:
            return str(get_box_value(box, is_edit_col=True))

    def _vector_col_data(self, cell_info, role, column):
        """data() implementation for VectorHash columns"""
        if column == 0:
            if role == Qt.DisplayRole:
                return cell_info.name
            elif role == Qt.DecorationRole:
                return get_icon(None)
        elif column == 1 and role == Qt.DisplayRole:
            return str(get_vector_col_value(cell_info))
        elif column == 2 and role == Qt.DisplayRole:
            return str(get_vector_col_value(cell_info, is_edit_col=True))

    def _vector_row_data(self, row_info, role, column, row):
        """data() implementation for VectorHash rows"""
        if column == 0:
            if role == Qt.DisplayRole:
                return str(row + 1)
            elif role == Qt.DecorationRole:
                return get_icon(None)
        elif column in (1, 2) and role == Qt.DisplayRole:
            return ''

    def _attribute_flags(self, attr_info):
        """flags() implementation for property attributes"""
        return Qt.ItemIsEditable

    def _box_flags(self, box):
        """flags() implementation for properties"""
        flags = 0
        descriptor = box.descriptor
        is_class = box.configuration.type in ('class', 'projectClass')
        is_node = isinstance(descriptor, Schema)
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

    def _vector_col_flags(self, cell_info):
        """flags() implementation for VectorHash columns"""
        return 0  # Qt.ItemIsEditable

    def _vector_row_flags(self, row_info):
        """flags() implementation for VectorHash rows"""
        return 0
