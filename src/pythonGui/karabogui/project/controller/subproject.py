#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import defaultdict

from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QAction, QDialog, QMenu
from traits.api import Instance, List, on_trait_change

from karabo.common.api import walk_traits_object
from karabo.common.project.api import (
    DeviceInstanceModel, ProjectModel, device_instance_exists)
from karabogui import messagebox
from karabogui.project.dialog.project_handle import NewProjectDialog
from karabogui.project.loading_watcher import ProjectLoadingWatcher
from karabogui.project.utils import load_project
from karabogui.singletons.api import get_project_model
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

    @pyqtSlot()
    def _add_project(self):
        """ Add a new subproject to the associated project
        """
        dialog = NewProjectDialog(default=True)
        if dialog.exec() == QDialog.Accepted:
            # XXX: TODO check for existing
            project = ProjectModel(simple_name=dialog.simple_name)
            # Set initialized and modified last
            project.initialized = project.modified = True
            self.model.subprojects.append(project)

    @pyqtSlot()
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
        """Make sure a subproject doesn't have conflicts with the open project
        """
        root_project = get_project_model().root_model
        if self._check_for_duplicate_projects(project, root_project):
            return
        if self._check_for_duplicate_devices(project, root_project):
            return

        # Finally add the subproject
        self.model.subprojects.append(project)

    def _check_for_duplicate_devices(self, project, root_project):
        """Make sure a subproject doesn't have conflicting device IDs
        """
        device_ids = []

        def visitor(model):
            if isinstance(model, DeviceInstanceModel):
                device_ids.append(model.instance_id)

        # collect all the device ids in the project being added
        walk_traits_object(project, visitor)

        # ... and make sure they don't already exist in any project
        if device_instance_exists(root_project, device_ids):
            # Computer says no....
            msg = ('The subproject which you wish to load contains devices<br>'
                   'which already exist in the current project. Therefore <br>'
                   'it will not be added.')
            messagebox.show_warning(msg, title='Device already exists')
            return True
        return False

    def _check_for_duplicate_projects(self, project, root_project):
        """Make sure a subproject doesn't include any projects which are
        already loaded.
        """
        project_uuids = defaultdict(int)
        project_names = []
        empty_projects = 0

        def _visitor(model):
            """Count the number of times each UUID appears in the tree"""
            nonlocal project_uuids, project_names, empty_projects
            if isinstance(model, ProjectModel):
                project_uuids[model.uuid] += 1
                if not model.simple_name:
                    empty_projects += 1
                if project_uuids[model.uuid] > 1:
                    if model.simple_name:
                        project_names.append(model.simple_name)

        # Walk both projects and count the instances of project models
        walk_traits_object(project, _visitor)
        walk_traits_object(root_project, _visitor)

        # If a UUID appeared more than once, we have a problem!
        if any(v > 1 for v in project_uuids.values()):
            msg = ('That project OR one of its sub-projects is already '
                   'loaded! Please investigate and have a look at the '
                   'project(s) <b>{}</b>! You have <b>{}</b> projects '
                   'that are loaded with an empty name!'.format(
                    project_names, empty_projects))
            messagebox.show_warning(msg, title='Load Subproject Failed')
            return True
        return False
