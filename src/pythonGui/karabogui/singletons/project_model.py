#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
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
import json
import re
from contextlib import contextmanager

from qtpy.QtCore import (
    QAbstractItemModel, QItemSelection, QItemSelectionModel, QMimeData,
    QModelIndex, Qt)

from karabo.common.api import walk_traits_object
from karabo.common.project.api import MacroModel
from karabo.common.scenemodel.api import SceneModel
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.indicators import get_instance_info_icon
from karabogui.project.controller.build import (
    create_project_controller, destroy_project_controller)
from karabogui.project.controller.device import DeviceInstanceController
from karabogui.project.utils import show_no_configuration
from karabogui.singletons.api import get_topology
from karabogui.util import create_table_string

TABLE_HEADER_LABELS = ["Projects", ""]

PROJECT_COLUMN = 0
STATUS_COLUMN = 1

ROLES = [Qt.CheckStateRole, Qt.DisplayRole, Qt.DecorationRole,
         Qt.ForegroundRole, Qt.FontRole, Qt.ToolTipRole]


class ProjectViewItemModel(QAbstractItemModel):
    """ A QAbstractItemModel which mediates between our Traits-based data model
    and Qt
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self.q_selection_model = QItemSelectionModel(self, self)

        self._traits_model = None
        self._controller = None

    def supportedDragActions(self):
        return Qt.CopyAction

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

    def controller_data_update(self, controller, column=PROJECT_COLUMN,
                               roles=ROLES):
        index_row = self._controller_row(controller)
        index = self.createIndex(index_row, column, controller)
        if index.isValid():
            self.dataChanged.emit(index, index, roles)

    @property
    def root_controller(self):
        """ Return the project controller object at the root of the hierarchy
        """
        return self._controller

    @property
    def root_model(self):
        """ Return the project object at the root of the hierarchy
        """
        return self._traits_model

    @root_model.setter
    def root_model(self, model):
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

    def mimeData(self, indices):
        """Reimplemented function of QAbstractItemModel.

        Provide data for Drag & Drop operations.
        """
        # Get one selection per row
        rows = {idx.row(): idx for idx in indices if idx.isValid()}
        # Extract info() dictionaries from controllers
        data = []
        for index in rows.values():
            controller = index.internalPointer()
            if controller is None:
                continue
            data.append(controller.info())

        mimeData = QMimeData()
        mimeData.setData('treeItems', bytearray(json.dumps(data),
                                                encoding='UTF-8'))
        return mimeData

    def selectModel(self, model):
        """Find and select a controller from the project `model`"""
        controller = None

        def _visitor(obj):
            """Find all items matching"""
            if obj.model is model:
                nonlocal controller
                controller = obj
                return True

        self.visit(_visitor)
        if controller is not None:
            self.selectNode(controller)
        return controller is not None

    def findNodes(self, text, case_sensitive=True,
                  use_reg_ex=True, full_match=False):
        """ Find in the ``self._traits_model`` all objects maching criteria"""
        if self._controller is None:
            return []

        controllers = []

        pattern = text if use_reg_ex else f".*{re.escape(text)}"
        flags = 0 if case_sensitive else re.IGNORECASE
        regex = re.compile(pattern, flags=flags)

        matcher = regex.fullmatch if full_match else regex.match

        def _visitor(obj):
            """Find all items matching"""
            if matcher(obj.display_name) is not None:
                controllers.append(obj)

        self.visit(_visitor)
        return controllers

    def visit(self, visitor):
        """Walk every controller in the model and run a `visitor` function on
        each item.
        """

        def _iter_tree_node(node):
            yield node
            for child in getattr(node, 'children', []):
                yield from _iter_tree_node(child)

        for controller in _iter_tree_node(self._controller):
            visitor(controller)

    def selectIndex(self, index):
        """Select the given `index` of type `QModelIndex` if this is not None
        """
        if index is None:
            self.q_selection_model.selectionChanged.emit(
                QItemSelection(), QItemSelection())
            return

        self.q_selection_model.setCurrentIndex(
            index, QItemSelectionModel.ClearAndSelect)

        treeview = super().parent()
        treeview.scrollTo(index)

    def selectNode(self, controller):
        """Select the node matching the given `controller`
        """
        if controller is not None:
            index = self.createIndex(
                self._controller_row(controller),
                PROJECT_COLUMN, controller)
        else:
            # Select nothing
            index = None
        self.selectIndex(index)

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
        broadcast_event(KaraboEvent.RemoveProjectModelViews,
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

    # ----------------------------
    # Qt methods

    def flags(self, index):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return Qt.NoItemFlags

        # All items have these properties
        flags = Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsDragEnabled
        controller = index.internalPointer()
        if isinstance(controller, DeviceInstanceController):
            flags |= Qt.ItemNeverHasChildren

        return flags

    def data(self, index, role=Qt.DisplayRole):
        """Reimplemented function of QAbstractItemModel.
        """
        if not index.isValid():
            return

        controller = index.internalPointer()
        if controller is None:
            return

        column = index.column()
        ui_data = controller.ui_data
        if column == PROJECT_COLUMN:
            if role == Qt.DisplayRole:
                return controller.ui_item_text()
            elif (role == Qt.ToolTipRole and not isinstance(
                    controller, DeviceInstanceController)):
                return self._create_tool_tip(controller.model)
            elif role == Qt.DecorationRole:
                return ui_data.icon
            elif role == Qt.ForegroundRole:
                return ui_data.brush
            elif role == Qt.FontRole:
                return ui_data.font
            elif role == Qt.CheckStateRole:
                if ui_data.checkable:
                    return ui_data.check_state
        elif column == STATUS_COLUMN and role == Qt.DecorationRole:
            if isinstance(controller, DeviceInstanceController):
                return get_instance_info_icon(ui_data.instance_status)

    def _create_tool_tip(self, model):
        selection = ["uuid", "date"]
        info = {}
        for akey in selection:
            avalue = getattr(model, akey)
            if avalue is not None:
                info.update({akey: str(avalue)})

        return create_table_string(info)

    def setData(self, index, value, role):
        """Reimplemented function of QAbstractItemModel.
        """
        if role != Qt.CheckStateRole:
            return False

        if index.column() == PROJECT_COLUMN:
            controller = index.internalPointer()
            if controller is None:
                return False

        # Value was successfully updated
        return True

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
            parent_controller = parent.internalPointer()

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

        child_controller = index.internalPointer()
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
            parent_controller = parent.internalPointer()

        if parent_controller is None:
            return 0

        return parent_controller.rows()
