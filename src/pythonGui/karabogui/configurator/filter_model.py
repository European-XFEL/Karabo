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
        self.setRecursiveFilteringEnabled(True)
        self.setSourceModel(source_model)

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
