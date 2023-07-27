#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on June 2, 2022
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
from qtpy.QtCore import QSortFilterProxyModel, Qt


class ConfiguratorFilterModel(QSortFilterProxyModel):

    def __init__(self, source_model=None, parent=None):
        super().__init__(parent)
        self.setFilterCaseSensitivity(Qt.CaseInsensitive)
        self.setFilterRole(Qt.DisplayRole)
        self.setFilterKeyColumn(0)
        self.setRecursiveFilteringEnabled(True)
        self.setDynamicSortFilter(False)
        self.setSourceModel(source_model)

    @property
    def root(self):
        """Return the root proxy of the source model"""
        return self.sourceModel().root

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
