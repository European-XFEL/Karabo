#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 3, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from weakref import WeakValueDictionary

from PyQt4.QtCore import pyqtSignal, QAbstractItemModel, QModelIndex, Qt
from PyQt4.QtGui import QBrush, QColor, QFont
from traits.api import Undefined

from karabo.middlelayer import AccessMode, Assignment
from karabo.common.api import State
from karabogui.binding.api import (
    BaseBinding, BindingRoot, ChoiceOfNodesBinding, DeviceClassProxy,
    DeviceProxy, ImageBinding, ListOfNodesBinding, NodeBinding,
    ProjectDeviceProxy, PropertyProxy, SlotBinding, has_changes,
    KARABO_SCHEMA_ALLOWED_STATES, KARABO_SCHEMA_DISPLAYED_NAME,
    KARABO_SCHEMA_DISPLAY_TYPE, KARABO_WARN_LOW, KARABO_WARN_HIGH,
    KARABO_ALARM_LOW, KARABO_ALARM_HIGH
)
from karabogui.const import (
    OK_COLOR, ERROR_COLOR_ALPHA, PROPERTY_ALARM_COLOR, PROPERTY_WARN_COLOR)
from karabogui.indicators import STATE_COLORS
from karabogui.request import send_property_changes
from .utils import (
    dragged_configurator_items, get_child_names, get_device_state_string,
    get_icon, get_proxy_value
)


def _friendly_repr(proxy, value):
    """Return a user-friendly value, convert base or with units displayed.
    """
    converters = {'hex': hex, 'oct': oct, 'bin': bin}
    base = proxy.binding.attributes.get(KARABO_SCHEMA_DISPLAY_TYPE)
    if base in converters:
        try:
            return converters[base](value)
        except TypeError:
            # value could be an empty string
            return value
    units = proxy.binding.unit_label
    value = str(value)
    return value + (' ' + units if units and value else '')


class ConfigurationTreeModel(QAbstractItemModel):
    signalHasModifications = pyqtSignal(bool)

    def __init__(self, parent=None):
        super(ConfigurationTreeModel, self).__init__(parent)
        self._root_proxy = None
        self._property_proxies = {}
        self._model_index_refs = WeakValueDictionary()
        self._attr_backreferences = WeakValueDictionary()
        self._header_labels = ('Property', 'Current value on device', 'Value')

    # ----------------------------
    # Public interface

    @property
    def root(self):
        """Return the `BaseDeviceProxy` instance that we're presenting to Qt
        """
        return self._root_proxy

    @root.setter
    def root(self, proxy):
        """Set the `BaseDeviceProxy` instance that we're presenting to Qt
        """
        oldproxy = self._root_proxy
        if oldproxy is not None:
            oldproxy.on_trait_change(self._config_update, 'config_update',
                                     remove=True)
            oldproxy.on_trait_change(self._config_update, 'schema_update',
                                     remove=True)
            if isinstance(oldproxy, DeviceProxy):
                oldproxy.on_trait_change(self._state_update,
                                         'state_binding.value', remove=True)

        try:
            self.beginResetModel()
            self._property_proxies.clear()
            self._model_index_refs.clear()
            self._attr_backreferences.clear()
            self._root_proxy = proxy
        finally:
            self.endResetModel()

        if proxy is not None:
            proxy.on_trait_change(self._config_update, 'config_update')
            proxy.on_trait_change(self._config_update, 'schema_update')
            if isinstance(proxy, DeviceProxy):
                proxy.on_trait_change(self._state_update,
                                      'state_binding.value')

    def apply_changes(self):
        """Send all modified properties to the remote device.
        """
        proxies = [p for p in self._property_proxies.values()
                   if p.edit_value is not None]
        if proxies:
            send_property_changes(proxies)

    def decline_changes(self):
        """Revert all modifications made to properties.
        """
        for proxy in self._property_proxies.values():
            if proxy.edit_value is not None:
                proxy.revert_edit()
        self._notify_of_modifications()

    def clear_index_modification(self, index):
        """Clear any stored modifications for the proxy referenced by `index`
        """
        proxy = self.index_ref(index)
        if proxy is None or not isinstance(proxy, PropertyProxy):
            return

        if proxy.binding is not None:
            proxy.revert_edit()
            self.layoutChanged.emit()
            self._notify_of_modifications()

    def flush_index_modification(self, index):
        """Send any stored modification for the proxy referenced by `index`
        """
        proxy = self.index_ref(index)
        if proxy is None or not isinstance(proxy, PropertyProxy):
            return

        if proxy.binding is not None:
            if isinstance(self.root, DeviceProxy):
                send_property_changes([proxy])
            else:
                self.setData(index, proxy.edit_value, Qt.EditRole)

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

    def property_proxy(self, path):
        """Get an existing PropertyProxy or create one if needed.
        """
        proxy = self._property_proxies.get(path)
        if proxy is None:
            proxy = PropertyProxy(path=path, root_proxy=self.root)
            self._property_proxies[path] = proxy
        return proxy

    # ----------------------------
    # Private interface

    def _add_attr_backref(self, binding, proxy):
        """Add a mapping from a BaseBinding to its parent PropertyProxy.

        This is necessary for supporting parent() on QModelIndex instances
        which point to attributes.
        """
        if binding not in self._attr_backreferences:
            self._attr_backreferences[binding] = proxy

    def _attr_backref(self, binding):
        """Return the PropertyProxy for a given BaseBinding"""
        return self._attr_backreferences.get(binding)

    def _config_update(self):
        """Notify the view of item updates
        """
        last_row = self.rowCount()
        first = self.index(0, 1)
        last = self.index(last_row, 1)
        self.dataChanged.emit(first, last)
        self.layoutChanged.emit()
        self._notify_of_modifications()

    def _notify_of_modifications(self):
        """Tell interested listeners about any changes
        """
        has_modifications = any(p.edit_value is not None
                                for p in self._property_proxies.values())
        self.signalHasModifications.emit(has_modifications)

    def _proxy_row(self, proxy):
        """Return the row for the given ``proxy``
        """
        if self.root is None or self.root.binding is None:
            return 0

        # Make sure we were actually passed a `PropertyProxy` and not
        # something else
        assert isinstance(proxy, PropertyProxy)

        parts = proxy.path.rsplit('.', 1)
        if len(parts) > 1:
            parent = self.property_proxy(parts[0])
            proxy_key = parts[1]
        else:
            parent = self.root
            proxy_key = proxy.path

        properties = get_child_names(parent)
        return properties.index(proxy_key)

    def _state_update(self, obj, name, value):
        """Respond to device instance state changes
        """
        # The trait handler is for 'state_binding.value'
        if name != 'value':
            return

        def recurse(binding, parent):
            if binding is None:
                return
            for row, name in enumerate(binding.value):
                sub_binding = getattr(binding.value, name)
                sub_index = self.index(row, 0, parent)
                attributes = sub_binding.attributes
                if attributes.get(KARABO_SCHEMA_ALLOWED_STATES, []):
                    self.dataChanged.emit(sub_index, sub_index)
                if isinstance(sub_binding, NodeBinding):
                    recurse(sub_binding, sub_index)

        recurse(self.root, QModelIndex())

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
        if not index.isValid():
            return None

        # short circuit for the text alignment
        column = index.column()
        if column == 1 and role == Qt.TextAlignmentRole:
            return Qt.AlignCenter

        # Get the index's stored object
        obj = self.index_ref(index)
        if obj is None:
            return None

        # background color is sorta special
        if column == 1 and role == Qt.BackgroundRole:
            if not isinstance(self.root, DeviceProxy):
                return None

            state = get_device_state_string(self.root)
            if state == '':  # `state` can be ''!
                return None

            # Properties have a color depending on alarm/warn
            color = (self._proxy_color(obj)
                     if isinstance(obj, PropertyProxy) else None)
            if color is None:
                # Use device state for color
                in_error = State(state) == State.ERROR
                color = ERROR_COLOR_ALPHA if in_error else OK_COLOR
            return QBrush(QColor(*color))

        if isinstance(obj, BaseBinding):
            return self._attribute_data(obj, role, column, index.row())
        else:
            return self._proxy_data(index, obj, role, column)

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

        # NOTE: `obj` can be a `PropertyProxy` or a `BaseBinding`

        # Check for draggable rows
        binding = getattr(obj, 'binding', None)
        is_node = isinstance(binding, (ChoiceOfNodesBinding, NodeBinding))
        is_special = isinstance(binding, (SlotBinding, ImageBinding))
        if is_special or (binding is not None and not is_node):
            flags |= Qt.ItemIsDragEnabled

        # Below are the value flags. Ignore the first and second column
        if index.column() in (0, 1):
            return flags

        # Value-specific flags
        if isinstance(obj, BaseBinding):
            flags |= self._attribute_flags(obj)
        else:
            flags |= self._proxy_flags(obj)

        return flags

    def headerData(self, section, orientation, role):
        """Reimplemented function of QAbstractItemModel.
        """
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            return self._header_labels[section]

    def index(self, row, column, parent=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        if self.root is None:
            return QModelIndex()

        if not self.hasIndex(row, column, parent):
            return QModelIndex()

        if not parent.isValid():
            parent_obj = self.root
        else:
            parent_obj = self.index_ref(parent)

        proxy_types = (DeviceClassProxy, DeviceProxy, PropertyProxy)
        if isinstance(parent_obj, proxy_types):
            binding = parent_obj.binding
            if isinstance(binding, ChoiceOfNodesBinding):
                names = [binding.choice]
            else:
                names = get_child_names(parent_obj)

            if isinstance(binding, BindingRoot):
                path = names[row]
                if parent_obj is not self.root:
                    # This is the child of a ChoiceOfNodes
                    path = parent_obj.path + '.' + path
                obj = self.property_proxy(path)
            elif isinstance(binding, (ChoiceOfNodesBinding, NodeBinding)):
                # Nodes have properties as children
                path = parent_obj.path + '.' + names[row]
                obj = self.property_proxy(path)
            else:  # Normal binding type
                # Leaves have attributes as children
                self._add_attr_backref(binding, parent_obj)
                obj = binding
        else:
            # Only properties can have children
            return QModelIndex()

        return self.createIndex(row, column, obj)

    def mimeData(self, indices):
        """Reimplemented function of QAbstractItemModel.

        NOTE: flags() is controlling which indices show up in this method. We
        don't have to do so many checks here.
        """
        if len(indices) == 0 or isinstance(self.root, DeviceClassProxy):
            return None

        # Only gather valid proxies for indices in the first column
        # (Qt passes indices for each column in a row)
        proxies = [self.index_ref(idx) for idx in indices
                   if idx.column() == 0 and self.index_ref(idx) is not None]
        return dragged_configurator_items(proxies)

    def parent(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if self.root is None:
            return QModelIndex()

        if not index.isValid():
            return QModelIndex()

        child_obj = self.index_ref(index)
        if child_obj is None:
            return QModelIndex()

        # Handle special indices correctly
        if isinstance(child_obj, BaseBinding):
            proxy = self._attr_backref(child_obj)
            if proxy is None:
                return QModelIndex()
        else:
            parts = child_obj.path.rsplit('.', 1)
            if len(parts) == 1:
                return QModelIndex()
            proxy = self.property_proxy(parts[0])

        return self.createIndex(self._proxy_row(proxy), 0, proxy)

    def rowCount(self, parent=QModelIndex()):
        """Reimplemented function of QAbstractItemModel.
        """
        if not parent.isValid():
            proxy = self.root
        else:
            proxy = self.index_ref(parent)

        if proxy is None:
            return 0

        # `proxy` might be a special marker
        if isinstance(proxy, BaseBinding):
            return 0

        # From here, we know `proxy` is really a `PropertyProxy` instance
        binding = proxy.binding
        if isinstance(binding, ChoiceOfNodesBinding):
            # ChoiceOfNodes only ever appears to have one child
            return 1
        elif isinstance(binding, ListOfNodesBinding):
            # XXX: ListOfNodes should be handled eventually
            return 0
        elif isinstance(binding, (BindingRoot, NodeBinding)):
            # Roots and nodes have children
            return len(get_child_names(proxy))
        elif isinstance(self.root, ProjectDeviceProxy):
            # project device properties can have children (attributes)
            return len(get_child_names(proxy))

        # otherwise no children
        return 0

    def setData(self, index, value, role):
        """Reimplemented function of QAbstractItemModel.
        """
        if role != Qt.EditRole or value is None:
            return False

        # Get the index's stored object
        obj = self.index_ref(index)
        if obj is None:
            return False

        if isinstance(obj, BaseBinding):
            proxy = self._attr_backref(obj)
            if proxy is None:
                return False
            name = proxy.editable_attributes[index.row()]
            obj.attributes[name] = value
            # project needs to be informed
            self.root.binding.config_update = True

        else:  # Normal property value setting
            proxy = obj
            if proxy.binding is None:
                return False

            binding = proxy.binding
            state = get_device_state_string(self.root)
            online_device = isinstance(self.root, DeviceProxy)
            old_value = None if proxy.edit_value is None else proxy.value
            if isinstance(binding, ChoiceOfNodesBinding):
                old_value = binding.choice  # ChoiceOfNodes is "special"
            changes = has_changes(binding, old_value, value)
            allowed = binding.is_allowed(state) or not online_device
            if allowed and changes:
                if online_device:
                    proxy.edit_value = value
                else:  # ProjectDeviceProxy or DeviceClassProxy
                    # Put the value directly in the binding
                    proxy.value = value
                    # Then revert edit_value to avoid the CHANGING background
                    proxy.revert_edit()
                    self.root.binding.config_update = True
            elif online_device:
                proxy.revert_edit()

            self.layoutChanged.emit()
            self._notify_of_modifications()

        # A value was successfully set!
        return True

    # ----------------------------
    # data() and flags() helper methods

    def _attribute_data(self, binding, role, column, row):
        """data() implementation for property attributes"""
        proxy = self._attr_backref(binding)
        if proxy is None:
            return None

        name = proxy.editable_attributes[row]
        value = binding.attributes.get(name)
        if column == 0:
            if role == Qt.DisplayRole:
                return name
            elif role == Qt.DecorationRole:
                return get_icon(binding)
        elif column in (1, 2):
            if role == Qt.DisplayRole:
                return str(value)
            elif role == Qt.EditRole:
                return value

    def _proxy_color(self, proxy):
        """data(role=Qt.ColorRole) for properties."""
        binding = proxy.binding
        attributes = binding.attributes
        value = binding.value
        alarm_low = attributes.get(KARABO_ALARM_LOW)
        alarm_high = attributes.get(KARABO_ALARM_HIGH)
        warn_low = attributes.get(KARABO_WARN_LOW)
        warn_high = attributes.get(KARABO_WARN_HIGH)
        if ((alarm_low is not None and value < alarm_low) or
                (alarm_high is not None and value > alarm_high)):
            return PROPERTY_ALARM_COLOR
        elif ((warn_low is not None and value < warn_low) or
                (warn_high is not None and value > warn_high)):
            return PROPERTY_WARN_COLOR
        return None  # indicate no color

    def _proxy_data(self, index, proxy, role, column):
        """data() implementation for properties"""
        binding = proxy.binding
        if column == 0:
            if role == Qt.DisplayRole:
                name = binding.attributes.get(KARABO_SCHEMA_DISPLAYED_NAME)
                return name or proxy.path.split('.')[-1]
            elif role == Qt.FontRole:
                is_class = isinstance(self.root, DeviceClassProxy)
                if is_class and binding.assignment is Assignment.MANDATORY:
                    font = QFont()
                    font.setBold(True)
                    return font
            elif role == Qt.DecorationRole:
                return get_icon(binding)
        elif column == 1 and role == Qt.DisplayRole:
            value = get_proxy_value(index, proxy)
            if value is Undefined:
                return None
            return _friendly_repr(proxy, value)
        elif column == 2:
            if role == Qt.BackgroundRole:
                if proxy.edit_value is not None:
                    color = QColor(*STATE_COLORS[State.CHANGING])
                    color.setAlpha(128)
                    return QBrush(color)
            elif role in (Qt.DisplayRole, Qt.EditRole):
                value = get_proxy_value(index, proxy, is_edit_col=True)
                if value is Undefined:
                    return None
                if role == Qt.DisplayRole:
                    return _friendly_repr(proxy, value)
                elif role == Qt.EditRole:
                    return value

    def _attribute_flags(self, binding):
        """flags() implementation for property attributes"""
        return Qt.ItemIsEditable

    def _proxy_flags(self, proxy):
        """flags() implementation for properties"""
        flags = 0
        state = get_device_state_string(self.root)
        binding = proxy.binding
        is_class = isinstance(self.root, DeviceClassProxy)
        uneditable_node_types = (ChoiceOfNodesBinding, ListOfNodesBinding,
                                 NodeBinding)
        is_uneditable_node = isinstance(binding, uneditable_node_types)
        is_editable_type = (not is_uneditable_node or (is_class and
                            isinstance(binding, ChoiceOfNodesBinding)))
        is_class_editable = (is_class and binding.access_mode in
                             (AccessMode.INITONLY, AccessMode.RECONFIGURABLE))
        is_inst_editable = (not is_class and binding.is_allowed(state) and
                            binding.access_mode is AccessMode.RECONFIGURABLE)
        if is_editable_type and (is_class_editable or is_inst_editable):
            flags |= Qt.ItemIsEditable
        return flags
