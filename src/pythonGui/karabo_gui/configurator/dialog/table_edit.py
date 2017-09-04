#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 9, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtGui import QDialog, QHBoxLayout

from karabo_gui.editablewidgets.editabletableelement import (
    EditableTableElement
)


class TableEditDialog(QDialog):
    def __init__(self, obj, parent=None):
        """ A dialog to configure a VectorHash

        :param obj: The data object representing the VectorHash
        :param parent: The parent of the dialog
        """
        super(TableEditDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'table_edit.ui')
        uic.loadUi(filepath, self)

        config_hsh = obj.toHash()
        # XXX: Do not change the name of this variable
        self.editable_widget = EditableTableElement(obj, self)
        self.editable_widget.valueChanged(obj, config_hsh[0])
        layout = QHBoxLayout()
        layout.setContentsMargins(2, 2, 2, 2)
        layout.addWidget(self.editable_widget.widget)
        self.empty_widget.setLayout(layout)
