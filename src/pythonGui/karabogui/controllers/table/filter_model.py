#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from qtpy.QtCore import QSortFilterProxyModel


class TableSortFilterModel(QSortFilterProxyModel):
    def __init__(self, parent=None):
        super().__init__(parent)

    def get_model_data(self, row, column):
        """Relay the request of model data to the source model"""
        index = self.index(row, column)
        source_index = self.mapToSource(index)
        return self.sourceModel().get_model_data(source_index.row(),
                                                 source_index.column())

    def index_ref(self, index):
        source_index = self.mapToSource(index)
        return source_index

    def get_header_key(self, section):
        """Relay the request of header data to the source model"""
        return self.sourceModel().get_header_key(section)

    def add_row(self, row, copy_row=None):
        """Relay request to `sourceModel` to add row"""
        return self.sourceModel().add_row(row, copy_row=copy_row)

    def add_row_below(self, row, copy_row=None):
        """Relay request to `sourceModel` to add row below"""
        return self.sourceModel().add_row_below(row, copy_row=copy_row)

    def moveRow(self, source_parent, source_row, parent, row):
        """Relay request to `sourceModel` for moving a row"""
        return self.sourceModel().moveRow(
            source_parent, source_row, parent, row)

    def remove_row(self, row):
        """Relay request to `sourceModel` for removing row"""
        return self.sourceModel().remove_row(row)

    def move_row_up(self, row):
        """Relay request to `sourceModel` for moving row up"""
        return self.sourceModel().move_row_up(row)

    def move_row_down(self, row):
        """Relay request to `sourceModel` for moving row down"""
        return self.sourceModel().move_row_down(row)
