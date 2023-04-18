#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 28, 2017
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
from qtpy.QtCore import QModelIndex, Qt
from qtpy.QtGui import QStandardItem, QStandardItemModel
from qtpy.QtWidgets import QAbstractItemView, QHeaderView, QTreeView
from traits.api import Bool, Instance, List

from karabo.common.scenemodel.api import RunConfiguratorModel
from karabo.native import Hash
from karabogui.binding.api import ListOfNodesBinding, get_editor_value
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller, with_display_type)

HEADER_LABELS = ['source', 'type', 'behavior', 'monitored', 'access']
NODE_CLASS_NAME = '_RunConfiguratorGroup'
# XXX: This widget ONLY supports a LIST_ELEMENT from the RunConfigurator
_is_compatible = with_display_type('RunConfigurator')


@register_binding_controller(ui_name='List Of Nodes', can_edit=True,
                             klassname='RunConfiguratorEdit',
                             binding_type=ListOfNodesBinding,
                             is_compatible=_is_compatible, priority=10)
class RunConfiguratorEdit(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(RunConfiguratorModel, args=())
    # Private traits
    _is_editing = Bool(False)
    _expanded = Bool(False)

    _expanded_indexes = List(args=())

    def create_widget(self, parent):
        widget = QTreeView(parent=parent)
        widget.setFocusPolicy(Qt.StrongFocus)
        widget.setUniformRowHeights(True)
        item_model = QStandardItemModel(parent=widget)
        item_model.setHorizontalHeaderLabels(HEADER_LABELS)
        item_model.itemChanged.connect(self._item_edited)

        header = widget.header()
        header.setSectionResizeMode(QHeaderView.ResizeToContents)
        header.sectionDoubleClicked.connect(self.onDoubleClickHeader)

        widget.setSelectionBehavior(QAbstractItemView.SelectRows)
        widget.setModel(item_model)

        return widget

    def value_update(self, proxy):
        # Avoid messing with the item model when the user checks an item
        if self._is_editing:
            return

        # We reset on every value update!
        self._expanded = False

        def _build(group_node, parent_item):
            name = group_node.value.groupId.value or 'NONAME'
            group_item = QStandardItem(name)
            # Group items are only checkable, not editable
            group_item.setEditable(False)
            group_item.setCheckable(True)
            if group_node.value.use.value:
                group_item.setCheckState(Qt.Checked)

            parent_item.appendRow(group_item)
            row, column = 0, 0
            for src in group_node.value.sources.value:
                for attr in HEADER_LABELS:
                    add_item = QStandardItem(str(src.get(attr, '')))
                    add_item.setEditable(False)
                    group_item.setChild(row, column, add_item)
                    column += 1
                row, column = row + 1, 0

        self.widget.setUpdatesEnabled(False)
        # Since we are about to reset our model, we store the expanded state
        # of the widget for comfort
        self.save_expanded()
        # Clear item_model before updating
        item_model = self.widget.model()
        item_model.clear()
        item_model.setHorizontalHeaderLabels(HEADER_LABELS)
        root_item = item_model.invisibleRootItem()
        for entry in get_editor_value(proxy, []):
            _build(entry, root_item)

        # Finally, we safely restore the expanded index by displayed name
        self.restore_expanded()
        self.widget.setUpdatesEnabled(True)

    def save_expanded(self):
        self._expanded_indexes = []
        model = self.widget.model()
        for row in range(model.rowCount()):
            index = model.index(row, 0)
            index_data = index.data(role=Qt.DisplayRole)
            if index_data is not None and self.widget.isExpanded(index):
                self._expanded_indexes.append(index_data)

    def restore_expanded(self):
        model = self.widget.model()
        for row in range(model.rowCount()):
            index = model.index(row, 0)
            index_data = index.data(role=Qt.DisplayRole)
            if index_data is not None and index_data in self._expanded_indexes:
                self.widget.setExpanded(index, True)
        self._expanded_indexes = []

    def _item_edited(self, item):
        if self.proxy.binding is None:
            return
        # Only react when top-level items are edited (ie: check-state changes)
        if item.parent() is None:
            try:
                self._is_editing = True
                self.proxy.edit_value = self._build_value()
            finally:
                self._is_editing = False

    def onDoubleClickHeader(self):
        if self._expanded:
            self.widget.collapseAll()
        else:
            self.widget.expandAll()

        self._expanded = not self._expanded

    def _build_value(self):
        def _build_source_hash(item, row):
            hsh = Hash()
            for col, name in enumerate(HEADER_LABELS):
                hsh[name] = item.child(row, col).text()
            return hsh

        values = []
        item_model = self.widget.model()
        for i in range(item_model.rowCount(QModelIndex())):
            item = item_model.item(i)
            item_hash = Hash('groupId', item.text(),
                             'use', item.checkState() == Qt.Checked)
            item_hash['sources'] = [_build_source_hash(item, j)
                                    for j in range(item.rowCount())]
            values.append(Hash(NODE_CLASS_NAME, item_hash))
        return values
