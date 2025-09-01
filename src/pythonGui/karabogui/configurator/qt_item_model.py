#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 3, 2017
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
#############################################################################
from weakref import WeakValueDictionary

from qtpy.QtCore import QAbstractItemModel, QModelIndex, Qt, Signal
from qtpy.QtGui import QBrush, QColor

import karabogui.access as krb_access
from karabo.common.api import State
from karabo.native import AccessLevel, AccessMode, Assignment, has_changes
from karabogui.binding.api import (
    BindingRoot, DeviceClassProxy, DeviceProxy, ImageBinding, NDArrayBinding,
    NodeBinding, ProjectDeviceProxy, PropertyProxy, SlotBinding, StringBinding,
    WidgetNodeBinding, get_binding_value)
from karabogui.fonts import get_qfont
from karabogui.indicators import (
    LOCKED_COLOR, PROPERTY_ALARM_COLOR_MAP, PROPERTY_READONLY_COLOR,
    STATE_COLORS, get_state_color)
from karabogui.request import send_property_changes

from .utils import (
    dragged_configurator_items, get_child_names, get_device_locked_string,
    get_device_state_string, get_icon, get_proxy_value, get_qcolor_state,
    is_mandatory)

SPECIAL_BINDINGS = (SlotBinding, ImageBinding,
                    NDArrayBinding, WidgetNodeBinding)


def _friendly_repr(proxy, value):
    """Return a user-friendly value, convert base or with units displayed.
    """
    converters = {'hex': hex, 'oct': oct, 'bin': bin}
    base = proxy.binding.displayType
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
    signalHasModifications = Signal(bool)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._root_proxy = None
        self._property_proxies = {}
        self._model_index_refs = WeakValueDictionary()
        self._header_labels = ('Property', 'Current value on device', 'Value')

        self.global_access = krb_access.GLOBAL_ACCESS_LEVEL
        self.mode_level = AccessLevel.OPERATOR

    # ----------------------------
    # Public interface

    def setAccessLevel(self, level: AccessLevel):
        """Set the global access level of this ConfigurationTreeModel"""
        self.global_access = level

    def setMode(self, expert: bool):
        """Set the mode of this ConfigurationTreeModel"""
        self.mode_level = (AccessLevel.EXPERT if expert
                           else AccessLevel.OPERATOR)

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
        try:
            self.beginResetModel()
            self._property_proxies.clear()
            self._model_index_refs.clear()
            self._root_proxy = proxy
        finally:
            self.endResetModel()

        if proxy is not None:
            proxy.on_trait_change(self._config_update, 'config_update')
            proxy.on_trait_change(self._config_update, 'schema_update')

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
        self.notify_of_modifications()

    def clear_index_modification(self, index):
        """Clear any stored modifications for the proxy referenced by `index`
        """
        proxy = self.index_ref(index)
        if proxy is None or not isinstance(proxy, PropertyProxy):
            return

        if proxy.binding is not None:
            proxy.revert_edit()
            # XXX: Announce the data update for the edit value, as there is no
            # external triggered update!
            self.dataChanged.emit(index, index)
            self.notify_of_modifications()

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

    def announceDataChanged(self):
        """Public method to announce that data has changed"""
        last_row = self.rowCount() - 1
        first = self.index(0, 1)
        last = self.index(last_row, 1)
        self.dataChanged.emit(first, last, [Qt.BackgroundRole, Qt.DisplayRole])

    def notify_of_modifications(self):
        """Tell interested listeners about any changes"""
        has_modifications = any(p.edit_value is not None
                                for p in self._property_proxies.values())
        self.signalHasModifications.emit(has_modifications)

    # ----------------------------
    # Private interface

    def _config_update(self):
        """Notify the view of item updates.

        Note: The `config_update` updates background color for alarms and
        states as well as the display value
        """
        self.announceDataChanged()
        self.notify_of_modifications()

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

        properties = get_child_names(parent, self.mode_level)
        return properties.index(proxy_key)

    # ----------------------------
    # Qt methods

    def createIndex(self, row, col, obj):
        """Reimplemented function of QAbstractItemModel.
        """
        key = id(obj)
        if key not in self._model_index_refs:
            self._model_index_refs[key] = obj
        return super().createIndex(row, col, key)

    def columnCount(self, parentIndex=None):
        """Reimplemented function of QAbstractItemModel.
        """
        return len(self._header_labels)

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
            if color is not None:
                return QBrush(QColor(*color))

            # Use device state and locking information for color
            is_locked = get_device_locked_string(self.root)
            if is_locked:
                color = QColor(*LOCKED_COLOR)
            else:
                color = get_qcolor_state(state)
            return QBrush(color)

        return self._proxy_data(index, obj, role, column)

    def flags(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return Qt.NoItemFlags

        obj = self.index_ref(index)
        if obj is None:
            return Qt.NoItemFlags

        # Note: `obj` can be a `PropertyProxy` or a `BaseBinding`
        flags = Qt.ItemIsEnabled | Qt.ItemIsSelectable
        # Check for draggable rows
        binding = getattr(obj, 'binding', None)
        is_node = isinstance(binding, NodeBinding)
        is_special = isinstance(binding, SPECIAL_BINDINGS)
        if is_special or (binding is not None and not is_node):
            flags |= Qt.ItemIsDragEnabled

        column = index.column()
        if column in (1, 2):
            flags |= Qt.ItemNeverHasChildren

        # Below are the value flags. Ignore the first and second column
        if column in (0, 1):
            return flags

        # Value-specific flags
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

        names = get_child_names(parent_obj, self.mode_level)
        binding = parent_obj.binding
        if isinstance(binding, BindingRoot):
            path = names[row]
            obj = self.property_proxy(path)
        elif isinstance(binding, NodeBinding):
            # Nodes have properties as children
            path = parent_obj.path + '.' + names[row]
            obj = self.property_proxy(path)

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

        # From here, we know `proxy` is really a `PropertyProxy` instance
        binding = proxy.binding
        if isinstance(binding, (BindingRoot, NodeBinding)):
            # Roots and nodes have children
            return len(get_child_names(proxy, self.mode_level))

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

        proxy = obj
        if proxy.binding is None:
            return False

        binding = proxy.binding
        state = get_device_state_string(self.root)
        online_device = isinstance(self.root, DeviceProxy)
        old_value = None if proxy.edit_value is None else proxy.value
        changes = has_changes(old_value, value)
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
        elif online_device or (allowed and not changes):
            # The value was set to the original value. We remove the blue
            # background here!
            proxy.revert_edit()

        self.notify_of_modifications()

        # A value was successfully set!
        return True

    # ----------------------------
    # data() and flags() helper methods

    def _proxy_color(self, proxy):
        """data(role=Qt.ColorRole) for properties."""
        binding = proxy.binding
        value = get_binding_value(binding)
        if value is None:
            return None  # indicate no color

        if isinstance(binding, StringBinding):
            if binding.displayType == 'State':
                return get_state_color(value) + (128,)

            if binding.displayType == 'AlarmCondition':
                return PROPERTY_ALARM_COLOR_MAP.get(value)
        return None  # indicate no color

    def access_level_tooltip(self, proxy):
        binding = proxy.binding
        writable = (binding.accessMode is not AccessMode.READONLY
                    or isinstance(binding, SlotBinding))
        if not writable:
            return
        level = proxy.binding.requiredAccessLevel
        allowed = self.global_access >= level
        return (f"Key: {proxy.path} - AccessLevel: {level.name} "
                f"- Access Allowed: {allowed}")

    def _proxy_data(self, index, proxy, role, column):
        """data() implementation for properties"""
        binding = proxy.binding
        if column == 0:
            if role == Qt.DisplayRole:
                name = binding.displayedName
                return name or proxy.path.split('.')[-1]
            elif role == Qt.BackgroundRole:
                return None
            elif role == Qt.FontRole:
                is_class = isinstance(self.root, DeviceClassProxy)
                if is_class and is_mandatory(binding):
                    font = get_qfont()
                    font.setBold(True)
                    return font
            elif role == Qt.ForegroundRole:
                is_class = isinstance(self.root, DeviceClassProxy)
                if (is_class and (binding.accessMode is AccessMode.READONLY or
                                  binding.assignment is Assignment.INTERNAL)):
                    return QColor(*PROPERTY_READONLY_COLOR)
            elif role == Qt.DecorationRole:
                return get_icon(binding)
            elif role == Qt.UserRole:
                return proxy.path
            elif role == Qt.ToolTipRole:
                return self.access_level_tooltip(proxy)
        elif column == 1 and role == Qt.DisplayRole:
            value = get_proxy_value(index, proxy)
            return value if isinstance(value, str) else _friendly_repr(
                proxy, value)
        elif column == 2:
            if role in (Qt.DisplayRole, Qt.EditRole):
                value = get_proxy_value(index, proxy, is_edit_col=True)
                if role == Qt.EditRole or isinstance(value, str):
                    return value
                elif role == Qt.DisplayRole:
                    return _friendly_repr(proxy, value)
            elif role == Qt.BackgroundRole:
                if proxy.edit_value is not None:
                    color = QColor(*STATE_COLORS[State.CHANGING])
                    color.setAlpha(128)
                    return QBrush(color)
            elif role == Qt.ToolTipRole:
                return self.access_level_tooltip(proxy)

    def _proxy_flags(self, proxy):
        """flags() implementation for properties"""
        flags = 0
        is_project = isinstance(self.root, ProjectDeviceProxy)
        if isinstance(self.root, DeviceClassProxy) and not is_project:
            return flags

        binding = proxy.binding
        if isinstance(binding, NodeBinding):
            return flags

        if is_project:
            writable = binding.accessMode in (AccessMode.INITONLY,
                                              AccessMode.RECONFIGURABLE)
            if (writable and binding.assignment is not Assignment.INTERNAL
                    and self.global_access >= binding.requiredAccessLevel):
                flags |= Qt.ItemIsEditable
        else:
            writable = binding.accessMode is AccessMode.RECONFIGURABLE
            if (writable and binding.is_allowed(
                    get_device_state_string(self.root)) and (
                        self.global_access >= binding.requiredAccessLevel)):
                flags |= Qt.ItemIsEditable

        return flags
