#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QAction, QDialog, QFont, QMenu
from traits.api import Instance, List

from karabo.common.project.api import ProjectModel
from karabo_gui import icons
from karabo_gui.project.dialog.project_handle import NewProjectDialog
from .bases import BaseProjectController
from .project_groups import ProjectSubgroupController, ProjectControllerUiData


class ProjectController(BaseProjectController):
    """ A controller for ProjectModel objects
    """
    # Redefine model with the correct type
    model = Instance(ProjectModel)
    # The subgroups of this project
    children = List(Instance(ProjectSubgroupController))

    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)

        # If this project is the top of the hierarchy, its parent must be None
        # In that case, do NOT add a 'Delete' action
        if parent_project is not None:
            edit_action = QAction('Edit', menu)
            edit_action.triggered.connect(self._edit_project)
            menu.addAction(edit_action)

        return menu

    def create_ui_data(self):
        font = QFont()
        font.setBold(True)
        return ProjectControllerUiData(font=font, icon=icons.folder)

    def child(self, index):
        """Returns a child of this controller.

        :param index: An index into the list of this controller's children
        :returns: A BaseProjectController instance or None
        """
        return self.children[index]

    def rows(self):
        """Returns the number of rows 'under' this controller in the project
        tree view.
        """
        return len(self.children)

    # ----------------------------------------------------------------------
    # action handlers

    def _edit_project(self):
        dialog = NewProjectDialog(self.model)
        result = dialog.exec()
        if result == QDialog.Accepted:
            self.model.simple_name = dialog.simple_name
