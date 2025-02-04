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
from collections import OrderedDict
from contextlib import contextmanager
from functools import partial

from qtpy.QtCore import (
    QItemSelection, QItemSelectionModel, QModelIndex, QPoint, QRect, Qt,
    Signal, Slot)
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import (
    QAbstractItemDelegate, QAbstractItemView, QAction, QMenu, QTreeView)

from karabo.common.api import (
    KARABO_SCHEMA_ACCESS_MODE, KARABO_SCHEMA_ALIAS,
    KARABO_SCHEMA_ARCHIVE_POLICY, KARABO_SCHEMA_ASSIGNMENT,
    KARABO_SCHEMA_DEFAULT_VALUE, KARABO_SCHEMA_DESCRIPTION,
    KARABO_SCHEMA_MAX_EXC, KARABO_SCHEMA_MAX_INC, KARABO_SCHEMA_MAX_SIZE,
    KARABO_SCHEMA_METRIC_PREFIX_SYMBOL, KARABO_SCHEMA_MIN_EXC,
    KARABO_SCHEMA_MIN_INC, KARABO_SCHEMA_MIN_SIZE,
    KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, KARABO_SCHEMA_TAGS,
    KARABO_SCHEMA_UNIT_SYMBOL)
from karabo.native import AccessLevel, AccessMode, Assignment
from karabogui import icons
from karabogui.binding.api import (
    BaseBinding, DeviceProxy, PropertyProxy, VectorHashBinding)
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.generic_scenes import get_property_proxy_model
from karabogui.indicators import ALARM_HIGH, ALARM_LOW, WARN_HIGH, WARN_LOW
from karabogui.widgets.popup import PopupWidget

from .edit_delegate import EditDelegate
from .filter_model import ConfiguratorFilterModel
from .qt_item_model import ConfigurationTreeModel
from .slot_delegate import SlotButtonDelegate
from .utils import get_proxy_value
from .value_delegate import ValueDelegate


class ConfigurationTreeView(QTreeView):
    """A tree view for `Configuration` instances"""
    itemSelectionChanged = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setDragEnabled(True)
        self.setTabKeyNavigation(True)
        self.header().setSectionsMovable(False)
        self.header().setMinimumSectionSize(100)
        self.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)
        triggers = (QAbstractItemView.CurrentChanged |
                    QAbstractItemView.SelectedClicked)
        self.setEditTriggers(triggers)

        self.edit_delegate = EditDelegate(parent=self)

        model = ConfigurationTreeModel(parent=self)
        self._source_model = model
        self.setModel(model)
        model.dataChanged.connect(self._update_popup_contents)

        # On demand we can have a filter model
        self._filter_model = None

        # Add a delegate for rows with slot buttons
        self.slot_delegate = SlotButtonDelegate(parent=self)
        self.setItemDelegateForColumn(0, self.slot_delegate)
        # ... and a delegate for the value column
        self.setItemDelegateForColumn(1, ValueDelegate(parent=self))
        # ... and a delegate for the editable value column
        self.setItemDelegateForColumn(2, self.edit_delegate)

        # Widget for more information of an index
        self.popup_widget = None
        self._popup_showing_index = None

        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self._show_context_menu)
        self.setUniformRowHeights(True)

    # ------------------------------------
    # Public methods

    def swap_models(self):
        """Swap the models of the tree view between filter and source"""
        self.close_popup_widget()
        current_index = self.currentIndex()
        if self._filter_model is None:
            model = ConfiguratorFilterModel(self.sourceModel())
            self.setModel(model)
            self._filter_model = model
            index = model.mapFromSource(current_index)
        else:
            index = self.model().mapToSource(current_index)
            self.setModel(self.sourceModel())
            self._filter_model.deleteLater()
            self._filter_model = None
        # Select row of index and set current index
        self.setRowSelection(index)

    def sourceModel(self):
        """Public method to retrieve the source model"""
        return self._source_model

    def filterModel(self):
        """Public method to retrieve the filter model"""
        return self._filter_model

    def setRowSelection(self, index):
        """Select the full row in the configurator and set the `index`"""
        if not index.isValid():
            self.selectionModel().clearSelection()
            return

        model = index.model()
        parent = index.parent()
        selection = QItemSelection(model.index(index.row(), 0, parent),
                                   model.index(index.row(), 2, parent))
        self.selectionModel().select(selection,
                                     QItemSelectionModel.ClearAndSelect)
        self.setCurrentIndex(index)

    @contextmanager
    def assign_popup(self):
        """Assign the existing popup index to a new one"""
        try:
            index = self._popup_showing_index
            if index is not None:
                parent = index.parent()
                if not parent.isValid():
                    index = index.row(), index.column(), parent
                    self._popup_showing_index = QModelIndex()
                else:
                    index = None
                    self.close_popup_widget()
            yield
        finally:
            if index is not None:
                index = self.model().index(*index)
                if index.isValid():
                    self._popup_showing_index = index
                else:
                    self.close_popup_widget()

    def assign_proxy(self, proxy):
        with self.assign_popup():
            model = self.sourceModel()
            model.root = proxy
            if proxy is None:
                return

            # Show second column only for devices
            if isinstance(proxy, DeviceProxy):
                self.setColumnHidden(1, False)
            else:
                self.setColumnHidden(1, True)
            # XXX: This is slightly hacky, but it keeps the buttons consistent
            model.notify_of_modifications()

    def apply_all(self):
        self.sourceModel().apply_changes()

    def decline_all(self):
        model = self.sourceModel()
        model.decline_changes()
        model.announceDataChanged()

    def clear(self):
        self.assign_proxy(None)
        self.close_popup_widget()

    def close_popup_widget(self):
        if self.popup_widget is not None:
            self.popup_widget.close()
            self.popup_widget = None
        self._popup_showing_index = None

    # ------------------------------------
    # Private methods

    def _get_popup_info(self, index):
        # Get the index's stored object
        obj = self.model().index_ref(index)
        if obj is None:
            return

        # Attributes show the info of their parent proxy
        if isinstance(obj, BaseBinding):
            index = index.parent()
            obj = self.model().index_ref(index)
            if obj is None:
                return

        binding = obj.binding
        attributes = binding.attributes
        property_name = index.data(Qt.DisplayRole)
        info = OrderedDict()
        if property_name:
            info['Property'] = property_name
        if KARABO_SCHEMA_DESCRIPTION in attributes:
            info['Description'] = attributes.get(KARABO_SCHEMA_DESCRIPTION)

        info['Key'] = obj.path
        if isinstance(binding, BaseBinding):
            # XXX: Maybe make this a little different looking
            name = type(binding).__name__[:-len('binding')]
            info['Value Type'] = name
        if KARABO_SCHEMA_DEFAULT_VALUE in attributes:
            info['Default Value'] = attributes.get(KARABO_SCHEMA_DEFAULT_VALUE)
        if KARABO_SCHEMA_ACCESS_MODE in attributes:
            info['AccessMode'] = AccessMode(
                attributes[KARABO_SCHEMA_ACCESS_MODE]).name
        if KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL in attributes:
            info['AccessLevel'] = AccessLevel(attributes.get(
                KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)).name
        if KARABO_SCHEMA_ASSIGNMENT in attributes:
            info['Assignment'] = Assignment(
                attributes[KARABO_SCHEMA_ASSIGNMENT]).name
        if KARABO_SCHEMA_ALIAS in attributes:
            info['Alias'] = attributes.get(KARABO_SCHEMA_ALIAS)
        if KARABO_SCHEMA_TAGS in attributes:
            info['Tags'] = ", ".join(attributes.get(KARABO_SCHEMA_TAGS))
        if binding.timestamp is not None:
            info['Timestamp'] = binding.timestamp.toLocal()
            info['Timing Id'] = binding.timestamp.tid
        display_type = binding.display_type
        if display_type and display_type.startswith('bin|'):
            info['Bits'] = display_type[4:]
        if isinstance(obj.root_proxy, DeviceProxy):
            info['Value on device'] = get_proxy_value(index, obj)

        # Unit related attributes
        additional_attrs = (KARABO_SCHEMA_METRIC_PREFIX_SYMBOL,
                            KARABO_SCHEMA_UNIT_SYMBOL)
        for attr_name in additional_attrs:
            attr = attributes.get(attr_name)
            info[attr_name] = attr

        # Other additional attributes
        additional_attrs = [
            ('Warn low', WARN_LOW), ('Warn high', WARN_HIGH),
            ('Alarm low', ALARM_LOW), ('Alarm high', ALARM_HIGH),
            (KARABO_SCHEMA_MIN_EXC, KARABO_SCHEMA_MIN_EXC),
            (KARABO_SCHEMA_MAX_EXC, KARABO_SCHEMA_MAX_EXC),
            (KARABO_SCHEMA_MIN_INC, KARABO_SCHEMA_MIN_INC),
            (KARABO_SCHEMA_MAX_INC, KARABO_SCHEMA_MAX_INC),
            (KARABO_SCHEMA_MIN_SIZE, KARABO_SCHEMA_MIN_SIZE),
            (KARABO_SCHEMA_MAX_SIZE, KARABO_SCHEMA_MAX_SIZE),
            ('ArchivePolicy', KARABO_SCHEMA_ARCHIVE_POLICY)]

        for label, attr_name in additional_attrs:
            attr = attributes.get(attr_name)
            info[label] = 'n/a' if attr is None else attr

        return info

    def _show_popup_widget(self, index, event_pos):
        # Only if the icon was clicked
        # Get the tree widget's x position
        tree_x = self.header().sectionViewportPosition(0)
        # Get the x coordinate of the root index in order to calculate
        # the identation of the index
        root_x = self.visualRect(self.rootIndex()).x()
        # Get the rectangle of the viewport occupied by index
        vis_rect = self.visualRect(index)
        # Calculate the x coordinate of the item
        index_x = tree_x + vis_rect.x() - root_x
        # Get the rect surrounding the icon
        icon_rect = QRect(index_x, vis_rect.y(), vis_rect.height(),
                          vis_rect.height())

        if not icon_rect.contains(event_pos):
            return

        if self.popup_widget is None:
            self.popup_widget = PopupWidget(can_freeze=True, parent=self)
        else:
            # NOTE: Reset pop-up widget, e.g. clean cache
            self.popup_widget.reset()

        info = self._get_popup_info(index)
        self._popup_showing_index = index
        self.popup_widget.setInfo(info)

        pos = QCursor.pos()
        pos.setX(pos.x() + 10)
        pos.setY(pos.y() + 10)
        self.popup_widget.move(pos)
        self.popup_widget.show()

    def _selected_editable_index(self):
        """Return the currently selected editable index
        """
        indexes = self.selectionModel().selectedIndexes()
        if indexes:
            last_column = self.model().columnCount() - 1
            index = [idx for idx in indexes if idx.column() == last_column][0]
            if index.flags() & Qt.ItemIsEditable == Qt.ItemIsEditable:
                return index

    @Slot(QPoint)
    def _show_context_menu(self, event):
        """Show a custom context menu
        """
        selected_index = self._selected_editable_index()
        if selected_index is None:
            return None

        selected_obj = self.model().index_ref(selected_index)
        binding = getattr(selected_obj, 'binding', None)
        if (isinstance(selected_obj, PropertyProxy) and binding is not None and
                KARABO_SCHEMA_DEFAULT_VALUE in binding.attributes):
            menu = QMenu()
            text = "Reset to default"
            acResetToDefault = QAction(icons.revert, text, None)
            acResetToDefault.setStatusTip(text)
            acResetToDefault.setToolTip(text)
            acResetToDefault.setIconVisibleInMenu(True)
            acResetToDefault.triggered.connect(partial(self._reset_to_default,
                                                       selected_obj))
            menu.addAction(acResetToDefault)
            menu.exec(QCursor.pos())

    @Slot(object)
    def _reset_to_default(self, proxy):
        """Reset the value of the given `proxy` to the default
        """
        binding = proxy.binding
        if binding is not None:
            default_value = binding.attributes.get(KARABO_SCHEMA_DEFAULT_VALUE)
            if isinstance(proxy.root_proxy, DeviceProxy):
                proxy.edit_value = default_value
            else:
                proxy.value = default_value
            self.sourceModel().announceDataChanged()

    @Slot(QModelIndex, QModelIndex)
    def _update_popup_contents(self, topLeft, bottomRight):
        """When the data in the model changes, blindly update the popup widget
        if it happens to be showing.
        """
        if self.popup_widget is None or not self.popup_widget.isVisible():
            return

        if self._popup_showing_index.isValid():
            info = self._get_popup_info(self._popup_showing_index)
            if info is None:
                # index wasn't actually valid because the device proxy changed
                self.popup_widget.hide()
            else:
                self.popup_widget.setInfo(info)

    # ------------------------------------
    # Event handlers

    def closeEditor(self, editor, hint):
        """XXX: PyQt does not send this signal properly in the item delegate,
        so we avoid signals altogether and call it directly...
        """
        self.edit_delegate.close_editor(editor, hint)
        super().closeEditor(editor, hint)

    def currentChanged(self, current, previous):
        """Pass selection changes along to the value delegate
        """
        self.edit_delegate.current_changed(current)
        super().currentChanged(current, previous)

    def setAccessLevel(self, level: AccessLevel):
        """Set the Global Access Level"""
        model = self.sourceModel()
        proxy = model.root
        self.assign_proxy(None)
        model.setAccessLevel(level)
        self.slot_delegate.setAccessLevel(level)
        self.assign_proxy(proxy)

    def setMode(self, expert: bool):
        model = self.sourceModel()
        proxy = model.root
        self.assign_proxy(None)
        model.setMode(expert)
        self.assign_proxy(proxy)

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            index = self.indexAt(event.pos())
            if index.isValid() and index.column() == 0:
                self._show_popup_widget(index, event.pos())
        super().mousePressEvent(event)

    def mouseDoubleClickEvent(self, event):
        if event.button() == Qt.LeftButton:
            index = self.indexAt(event.pos())
            if index.isValid() and index.column() == 1:
                proxy = self.model().index_ref(index)
                model = get_property_proxy_model(proxy, include_images=True)
                if model is not None:
                    data = {"model": model}
                    broadcast_event(KaraboEvent.ShowUnattachedController, data)
                event.accept()

        super().mouseDoubleClickEvent(event)

    def keyPressEvent(self, event):
        """Reimplemented function of QTreeView.

        Emulates the QTreeView enter/escape key behavior for rows containing
        tables. NOTE: All other row types are ignored!
        """
        key_event = event.key()
        if key_event in (Qt.Key_Return, Qt.Key_Enter, Qt.Key_Escape):
            # Get current index
            selected_index = self._selected_editable_index()
            if selected_index is not None:
                # NOTE: Ensure this is a VectorHash!
                obj = self.model().index_ref(selected_index)
                is_table = isinstance(getattr(obj, 'binding', None),
                                      VectorHashBinding)
                # Act on that item
                if is_table and key_event in (Qt.Key_Return, Qt.Key_Enter):
                    self.edit_delegate.update_model_data(
                        selected_index, QAbstractItemDelegate.SubmitModelCache)
                    return
                elif key_event == Qt.Key_Escape:
                    self.edit_delegate.update_model_data(
                        selected_index, QAbstractItemDelegate.RevertModelCache)
                    return

        super().keyPressEvent(event)
