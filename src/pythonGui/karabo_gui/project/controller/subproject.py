#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QAction, QDialog, QMenu

from karabo.common.project.api import ProjectModel
from karabo_gui.project.dialog.project_handle import NewProjectDialog
from .project_groups import ProjectSubgroupController


class SubprojectController(ProjectSubgroupController):
    """ A controller for 'subprojects' subgroups.
    """
    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)
        add_action = QAction('Add new project...', menu)
        add_action.triggered.connect(self._add_project)
        load_action = QAction('Load project...', menu)
        load_action.triggered.connect(self._load_project)
        menu.addAction(add_action)
        menu.addAction(load_action)
        return menu

    # ----------------------------------------------------------------------
    # action handlers

    def _add_project(self):
        """ Add a new subproject to the associated project
        """
        dialog = NewProjectDialog()
        if dialog.exec() == QDialog.Accepted:
            # XXX: TODO check for existing
            project = ProjectModel(simple_name=dialog.simple_name)
            # Set initialized and modified last to avoid bumping revision
            project.initialized = project.modified = True
            self.model.subprojects.append(project)

    def _load_project(self):
        """ Add an existing project as a subproject.
        """
        pass
