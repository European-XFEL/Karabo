#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 9, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from pathlib import Path

from qtpy import uic
from qtpy.QtWidgets import QDialog, QDialogButtonBox, QHBoxLayout

from karabogui.controllers.edit.table import EditableTableElement

MAX_WIDTH = 1024
COLUMN_FACTOR = 120


class TableDialog(QDialog):
    def __init__(self, proxy, editable, parent=None):
        """A dialog to configure a VectorHash

        :param proxy: The `PropertyProxy` object representing the VectorHash
        :param editable: If the controller inside the dialog is editable
        :param parent: The parent of the dialog
        """
        super().__init__(parent)
        filepath = Path(__file__).parent / 'table_view.ui'
        uic.loadUi(filepath, self)
        # XXX: Do not change the name of this variable since the
        # `EditDelegate` method `setModelData` refers to this dialog as the
        # passed `editor` and fetches the data of the `controller`
        self.controller = EditableTableElement(proxy=proxy)
        # This is parented by the `EditDelegate`, which means it will stay
        # alive very long!
        self.controller.create(parent)
        self.controller.set_read_only(not editable)
        self.controller.binding_update(proxy)
        self.controller.value_update(proxy)

        layout = QHBoxLayout()
        layout.setContentsMargins(2, 2, 2, 2)
        layout.addWidget(self.controller.widget)
        # `empty_widget` is defined in table_edit.ui
        self.empty_widget.setLayout(layout)
        cancel_button = self.buttonBox.button(QDialogButtonBox.Cancel)
        cancel_button.setVisible(editable)

        width = 450
        height = 350

        binding = proxy.binding
        if binding is not None:
            columns = len(binding.bindings)
            # minimum width of 450, but can go up to `MAX_WIDTH`
            width = min(max(columns * COLUMN_FACTOR, width), MAX_WIDTH)
        self.resize(width, height)

    def done(self, result):
        self.controller.destroy_widget()
        return super().done(result)
