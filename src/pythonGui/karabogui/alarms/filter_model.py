#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on March 18, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
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
