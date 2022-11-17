#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on June 2, 2022
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from qtpy.QtCore import QSortFilterProxyModel, Qt


class ConfiguratorFilterModel(QSortFilterProxyModel):

    def __init__(self, source_model=None, parent=None):
        super().__init__(parent)
        self.setFilterCaseSensitivity(Qt.CaseInsensitive)
        self.setFilterRole(Qt.DisplayRole)
        self.setFilterKeyColumn(0)
        self.setSourceModel(source_model)

    def filterAcceptsRow(self, source_row, source_parent):
        model = self.sourceModel()
        source_index = model.index(source_row, 0, source_parent)
        if source_index.isValid():
            node = model.index_ref(source_index)
            if node is None:
                return True

            if self.filterRegExp().isEmpty():
                return True
            row_count = model.rowCount(source_index)
            for row in range(row_count):
                if self.filterAcceptsRow(row, source_index):
                    return True

        return super().filterAcceptsRow(source_row, source_parent)

    # --------------------------------------------------------------------
    # Index Methods

    def index_ref(self, index):
        source_index = self.mapToSource(index)
        if not source_index.isValid():
            return None
        return self.sourceModel().index_ref(source_index)

    def indexInfo(self, index):
        source_index = self.mapToSource(index)
        if not source_index.isValid():
            return {}
        return self.sourceModel().indexInfo(source_index)

    def clear_index_modification(self, index):
        index = self.mapToSource(index)
        return self.sourceModel().clear_index_modification(index)

    def flush_index_modification(self, index):
        index = self.mapToSource(index)
        return self.sourceModel().flush_index_modification(index)
