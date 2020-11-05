#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 9, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt5 import uic
from PyQt5.QtWidgets import QDialog, QDialogButtonBox, QHBoxLayout

from karabogui.controllers.edit.table import EditableTableElement


class TableDialog(QDialog):
    def __init__(self, proxy, editable, parent=None):
        """A dialog to configure a VectorHash

        :param proxy: The `PropertyProxy` object representing the VectorHash
        :param parent: The parent of the dialog
        """
        super(TableDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'table_view.ui')
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

    def done(self, result):
        self.controller.destroy_widget()
        return super(TableDialog, self).done(result)
