#############################################################################
# Author: <dennis.goeries@xfel.eu>
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

from qtpy.QtCore import QModelIndex, Slot
from qtpy.QtGui import QKeySequence
from qtpy.QtWidgets import QHeaderView, QTableView

from karabo.native import decodeBinary


class KaraboTableView(QTableView):
    """The KaraboTableView supports internal `Drag and Drop` operations"""

    def __init__(self, parent=None):
        super().__init__(parent)
        self._readonly = True
        header = self.horizontalHeader()
        header.sectionDoubleClicked.connect(self._header_resize)

    def set_read_only(self, ro):
        """Public method to set the readonly status `ro` of the table view

        Note: Method added with Karabo 2.16.X
        """
        self._readonly = ro
        writable = not ro
        self.setAcceptDrops(writable)
        self.setDragEnabled(writable)
        self.setDropIndicatorShown(writable)

    def referenceIndex(self):
        """Convenience function to return the current reference index

        Note: Method added with Karabo 2.16.X
        """
        selection = self.selectionModel()
        if selection is not None and selection.hasSelection():
            return self.model().index_ref(selection.currentIndex())
        return QModelIndex()

    def keyPressEvent(self, event):
        """Reimplemented method of `QTableView`"""
        if self._readonly:
            return super().keyPressEvent(event)

        if event.matches(QKeySequence.New):
            index = self.referenceIndex()
            model = self.model()
            if not index.isValid():
                index = model.index(0, 0)
            row = index.row()
            model.add_row(row + 1)
            self.selectRow(row + 1)
            event.accept()
            return
        elif event.matches(QKeySequence.Delete):
            index = self.referenceIndex()
            if index.isValid():
                row = index.row()
                self.model().remove_row(row)
            event.accept()
            return
        elif event.matches(QKeySequence.SelectPreviousLine):
            index = self.referenceIndex()
            if index.isValid():
                row = index.row()
                self.model().move_row_up(row)
                self.selectRow(row - 1)
            event.accept()
            return
        elif event.matches(QKeySequence.SelectNextLine):
            index = self.referenceIndex()
            if index.isValid():
                row = index.row()
                self.model().move_row_down(row)
                self.selectRow(row + 1)
            event.accept()
            return
        return super().keyPressEvent(event)

    @Slot()
    def _header_resize(self):
        """Resize the table view to the model contents"""
        model = self.model()
        if model is not None and model.columnCount() > 1:
            columns = model.columnCount() - 1
            header = self.horizontalHeader()
            header.resizeSections(QHeaderView.ResizeToContents)
            header.resizeSection(columns, QHeaderView.Stretch)

    def dragEnterEvent(self, event):
        """Reimplemented method of `QTableView`"""
        self._inspect_event(event)

    def dragMoveEvent(self, event):
        """Reimplemented method of `QTableView`"""
        self._inspect_event(event)

    def dropEvent(self, event):
        """Reimplemented method of `QTableView`"""
        mime = self._inspect_event(event)
        source_row, row = mime
        if row == source_row:
            return
        self.model().moveRow(QModelIndex(), source_row,
                             QModelIndex(), row)
        self.selectRow(row)

    def _inspect_event(self, event):
        """Inspect the drag and either ignore or accept and provide data"""
        if event.source() is not self:
            event.ignore()
            return

        model = self.model()
        if model is None:
            event.ignore()
            return

        mime = event.mimeData().data("tableData").data()
        if not mime:
            event.ignore()
            return

        source_row = decodeBinary(mime)["row"]
        index = self.indexAt(event.pos())
        index = model.index_ref(index)
        row = index.row() if index.isValid() else model.rowCount() - 1
        event.accept()
        return source_row, row
