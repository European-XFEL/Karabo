#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QItemSelectionModel, QStandardItemModel


class ProjectItemModel(QStandardItemModel):
    """ A QStandardItemModel which mediates between our Traits-based data model
    and Qt
    """

    def __init__(self, parent=None):
        super(ProjectItemModel, self).__init__(parent)
        self.q_selection_model = QItemSelectionModel(self)

    def flags(self, index):
        return super(ProjectItemModel, self).flags(self, index)

    def mimeData(self, indexes):
        """ QAbstractItemModel::mimeData
        """
        return super(ProjectItemModel, self).mimeData(indexes)

    def dropMimeData(self, data, action, row, column, parent):
        """ QAbstractItemModel::dropMimeData
        """
        return super(ProjectItemModel, self).dropMimeData(data, action, row,
                                                          column, parent)
