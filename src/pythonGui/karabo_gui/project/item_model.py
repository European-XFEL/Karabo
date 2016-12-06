#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QItemSelectionModel, QStandardItemModel

from .model.shadow import (create_project_model_shadow,
                           destroy_project_model_shadow)

TABLE_HEADER_LABELS = ["Projects"]


class ProjectItemModel(QStandardItemModel):
    """ A QStandardItemModel which mediates between our Traits-based data model
    and Qt
    """

    def __init__(self, parent=None):
        super(ProjectItemModel, self).__init__(parent)
        self.q_selection_model = QItemSelectionModel(self, self)

        self._traits_model = None
        self._shadow_model = None

        self.setHorizontalHeaderLabels(TABLE_HEADER_LABELS)

    @property
    def traits_data_model(self):
        """ Return the project object at the root of the hierarchy
        """
        return self._traits_model

    @traits_data_model.setter
    def traits_data_model(self, model):
        """ Set the ProjectModel instance that we're presenting to Qt
        """
        # Clean up any previously created shadow models
        if self._shadow_model is not None:
            destroy_project_model_shadow(self._shadow_model)
            self.clear()
            # `clear()` removes the header data
            self.setHorizontalHeaderLabels(TABLE_HEADER_LABELS)

        self._traits_model = model
        if model is not None:
            self._shadow_model = create_project_model_shadow(model)
            self.invisibleRootItem().appendRow(self._shadow_model.qt_item)
