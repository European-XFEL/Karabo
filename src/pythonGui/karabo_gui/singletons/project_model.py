#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QItemSelectionModel, QStandardItemModel

from karabo_gui.events import broadcast_event, KaraboEventSender
from karabo_gui.project.api import (
    create_project_controller, destroy_project_controller
)
from karabo_gui.singletons.api import get_topology

TABLE_HEADER_LABELS = ["Projects"]


class ProjectViewItemModel(QStandardItemModel):
    """ A QStandardItemModel which mediates between our Traits-based data model
    and Qt
    """

    def __init__(self, parent=None):
        super(ProjectViewItemModel, self).__init__(parent)
        self.q_selection_model = QItemSelectionModel(self, self)

        self._traits_model = None
        self._controller = None

        self.setHorizontalHeaderLabels(TABLE_HEADER_LABELS)

    def _cleanup_project(self):
        """ Clean up the ``self._traits_model`` properly which means trigger
        certain events"""
        if self._traits_model is None:
            return

        for scene in self._traits_model.scenes:
            broadcast_event(KaraboEventSender.RemoveSceneView,
                            {'model': scene})

        for macro in self._traits_model.macros:
            broadcast_event(KaraboEventSender.RemoveMacro, {'model': macro})

    @property
    def traits_data_model(self):
        """ Return the project object at the root of the hierarchy
        """
        return self._traits_model

    @traits_data_model.setter
    def traits_data_model(self, model):
        """ Set the ProjectModel instance that we're presenting to Qt
        """
        # Clean up any previously created controllers
        if self._controller is not None:
            destroy_project_controller(self._controller)
            get_topology().clear_project_devices()
            self.clear()
            # Need to reset header data after clear()
            self.setHorizontalHeaderLabels(TABLE_HEADER_LABELS)

        self._cleanup_project()

        self._traits_model = model
        if model is not None:
            self._controller = create_project_controller(model)
            self.invisibleRootItem().appendRow(self._controller.qt_item)
