#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 28, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot, QModelIndex, Qt
from PyQt4.QtGui import (QAbstractItemView, QStandardItemModel, QTreeView,
                         QStandardItem)
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import RunConfiguratorModel
from karabo.middlelayer import Hash
from karabo_gui.binding.api import (
    BaseBindingController, ListOfNodesBinding, register_binding_controller
)
from karabo_gui.controllers.util import with_display_type

HEADER_LABELS = ['source', 'type', 'behavior', 'monitored', 'access']
NODE_CLASS_NAME = '_RunConfiguratorGroup'
# XXX: This widget ONLY supports a LIST_ELEMENT from the RunConfigurator
_is_compatible = with_display_type('RunConfigurator')


# XXX: priority = 10
@register_binding_controller(ui_name='List Of Nodes',
                             binding_type=ListOfNodesBinding,
                             is_compatible=_is_compatible)
class RunConfiguratorEdit(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(RunConfiguratorModel)

    def create_widget(self, parent):
        widget = QTreeView(parent=parent)
        widget.setSelectionBehavior(QAbstractItemView.SelectRows)
        item_model = QStandardItemModel(parent=self.widget)
        item_model.setHorizontalHeaderLabels(HEADER_LABELS)
        item_model.itemChanged.connect(self._item_edited)
        widget.setModel(item_model)
        return widget

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        """Update the item_model to the incoming `value`
        """
        if self.widget is None:
            return

        def _build(group_node, parent_item):
            name = group_node.value.groupId.value or 'NONAME'
            group_item = QStandardItem(name)
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

        # Clear item_model before updating
        item_model = self.widget.model()
        item_model.clear()
        item_model.setHorizontalHeaderLabels(HEADER_LABELS)
        root_item = item_model.invisibleRootItem()
        for entry in value:
            _build(entry, root_item)

    @pyqtSlot(object)
    def _item_edited(self, item):
        # Only react when top-level items are edited (ie: check-state changes)
        if item.parent() is None:
            self.proxy.value = self._build_value()

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
