#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QAction, QDialog, QMenu, QMessageBox
from traits.api import Instance, List, on_trait_change

from karabo.common.api import walk_traits_object
from karabo.common.project.api import (
    DeviceInstanceModel, ProjectModel, device_instance_exists)
from karabo_gui.project.dialog.project_handle import NewProjectDialog
from karabo_gui.project.loading_watcher import ProjectLoadingWatcher
from karabo_gui.project.utils import load_project
from karabo_gui.singletons.api import get_project_model
from .project_groups import ProjectSubgroupController


class SubprojectController(ProjectSubgroupController):
    """ A controller for 'subprojects' subgroups.
    """
    # These are subprojects which are asynchronously loading
    _watchers = List(Instance(ProjectLoadingWatcher))

    def context_menu(self, project_controller, parent=None):
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
            # Set initialized and modified last
            project.initialized = project.modified = True
            self.model.subprojects.append(project)

    def _load_project(self):
        """ Add an existing project as a subproject.
        """
        project = load_project(is_subproject=True)
        if project is None:
            return

        # Loading projects is complex, due to asynchronicity. Use a helper
        # to determine when all the project data is present.
        watcher = ProjectLoadingWatcher(project=project)
        if watcher.finished:
            self._check_and_add_project(project)
        else:
            self._watchers.append(watcher)

    # ----------------------------------------------------------------------
    # trait handlers

    @on_trait_change('_watchers:finished_event')
    def _watcher_finished(self, obj, name, old, new):
        """Called when a project finishes loading all of its data
        """
        self._watchers.remove(obj)
        self._check_and_add_project(obj.project)

    # ----------------------------------------------------------------------
    # util methods

    def _check_and_add_project(self, project):
        """Make sure a subproject doesn't have conflicting device IDs
        """
        device_ids = []

        def visitor(model):
            if isinstance(model, DeviceInstanceModel):
                device_ids.append(model.instance_id)

        # collect all the device ids in the project being added
        walk_traits_object(project, visitor)

        # ... and make sure they don't already exist in any project
        root_project = get_project_model().traits_data_model
        if device_instance_exists(root_project, device_ids):
            # Computer says no....
            msg = ('The subproject which you wish to load contains devices<br>'
                   'which already exist in the current project. Therefore <br>'
                   'it will not be added.')
            QMessageBox.warning(None, 'Device already exists', msg)
            return

        # Finally add the subproject
        self.model.subprojects.append(project)
