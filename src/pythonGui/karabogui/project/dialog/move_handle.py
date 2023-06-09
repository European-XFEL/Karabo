#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on June 9, 2023
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################

from qtpy.QtCore import Qt, Slot
from qtpy.QtWidgets import (
    QDialog, QHBoxLayout, QListWidget, QListWidgetItem, QPushButton,
    QVBoxLayout)

from karabogui import icons


class MoveHandleDialog(QDialog):
    def __init__(self, models, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Rearrange project items")
        self.setModal(False)

        main_layout = QHBoxLayout(self)
        widget = QListWidget(self)
        main_layout.addWidget(widget)
        self.widget = widget
        self.widget.setDragDropMode(QListWidget.InternalMove)

        # Button Actions
        vertical_layout = QVBoxLayout()
        self.move_up_button = QPushButton(icons.arrowFancyUp, "", self)
        self.move_up_button.clicked.connect(self._move_up_clicked)
        vertical_layout.addWidget(self.move_up_button)

        self.move_down_button = QPushButton(icons.arrowFancyDown, "", self)
        self.move_down_button.clicked.connect(self._move_down_clicked)
        vertical_layout.addWidget(self.move_down_button)
        vertical_layout.addStretch(1)

        button = QPushButton("OK", self)
        button.clicked.connect(self.accept)
        vertical_layout.addWidget(button)

        button = QPushButton("Cancel", self)
        button.clicked.connect(self.reject)
        vertical_layout.addWidget(button)
        main_layout.addLayout(vertical_layout)

        # Finally, add the data
        for model in models:
            self._add_item(model)

    # ----------------------------------------------------------------------
    # Public interface

    @property
    def items(self):
        return [self.widget.item(index).data(Qt.UserRole)
                for index in range(self.widget.count())]

    # ----------------------------------------------------------------------
    # Private interface

    def _add_item(self, model):
        """Add an item to the list widget with `simple_name`"""
        item = QListWidgetItem(str(model.simple_name))
        item.setData(Qt.UserRole, model)
        self.widget.addItem(item)

    # ----------------------------------------------------------------------
    # Slots

    @Slot()
    def _move_up_clicked(self):
        row = self.widget.currentRow()
        widget = self.widget
        if row > 0:
            item = widget.takeItem(row)
            widget.insertItem(row - 1, item)
            widget.setCurrentRow(row - 1)

    @Slot()
    def _move_down_clicked(self):
        row = self.widget.currentRow()
        widget = self.widget
        if row < widget.count() - 1:
            item = widget.takeItem(row)
            widget.insertItem(row + 1, item)
            widget.setCurrentRow(row + 1)
