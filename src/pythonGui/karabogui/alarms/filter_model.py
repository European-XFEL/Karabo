#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on March 18, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from qtpy.QtCore import QModelIndex, Qt, QSortFilterProxyModel

from .const import ALARM_WARNING_TYPES

ALARM_TYPE_COLUMN = 5


class AlarmFilterModel(QSortFilterProxyModel):
    def __init__(self, source_model=None, parent=None):
        super(AlarmFilterModel, self).__init__(parent)
        self.setSourceModel(source_model)
        self.setFilterRole(Qt.DisplayRole)
        self.filter_type = ALARM_WARNING_TYPES  # default filter

    @property
    def instanceId(self):
        """Provide the alarm service instance Id"""
        model = self.sourceModel()
        return model.instanceId

    def filterAcceptsRow(self, row, parent=QModelIndex):
        model = self.sourceModel()
        _type = model.data(model.index(row, ALARM_TYPE_COLUMN))
        return _type in self.filter_type

    def updateFilter(self, filter_type):
        self.filter_type = filter_type
        self.invalidateFilter()
