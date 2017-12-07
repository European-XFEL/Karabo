#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 9, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtGui import QDialog, QHBoxLayout

from karabogui.controllers.edit.table import EditableTableElement


class TableEditDialog(QDialog):
    def __init__(self, proxy, parent=None):
        """A dialog to configure a VectorHash

        :param proxy: The `PropertyProxy` object representing the VectorHash
        :param parent: The parent of the dialog
        """
        super(TableEditDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'table_edit.ui')
        uic.loadUi(filepath, self)

        # XXX: Do not change the name of this variable since the
        # `EditDelegate` method `setModelData` refers to this dialog as the
        # passed `editor` and fetches the data of the `controller`
        self.controller = EditableTableElement(proxy=proxy)
        self.controller.create(parent)
        self.controller.set_read_only(False)

        layout = QHBoxLayout()
        layout.setContentsMargins(2, 2, 2, 2)
        layout.addWidget(self.controller.widget)
        # `empty_widget` is defined in table_edit.ui
        self.empty_widget.setLayout(layout)
