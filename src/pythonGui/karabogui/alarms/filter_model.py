#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on March 18, 2019
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

from qtpy.QtCore import QModelIndex, QSortFilterProxyModel, Qt

from .const import ALARM_WARNING_TYPES

INSTANCE_COLUMN = 2
ALARM_TYPE_COLUMN = 4


class AlarmFilterModel(QSortFilterProxyModel):
    def __init__(self, source_model=None, parent=None):
        super().__init__(parent)
        self.filter_type = ALARM_WARNING_TYPES  # default filter
        self.setFilterRole(Qt.DisplayRole)
        self.setFilterKeyColumn(INSTANCE_COLUMN)
        self.setFilterCaseSensitivity(False)
        self.setSourceModel(source_model)

    @property
    def instanceId(self):
        """Provide the alarm service instance Id"""
        model = self.sourceModel()
        return model.instanceId

    def get_alarm_id(self, index):
        source_index = self.mapToSource(index)
        return self.sourceModel().get_alarm_id(source_index)

    def filterAcceptsRow(self, row, parent=QModelIndex()):
        model = self.sourceModel()
        _type = model.data(model.index(row, ALARM_TYPE_COLUMN))
        if _type not in self.filter_type:
            return False
        return super().filterAcceptsRow(row, parent)
