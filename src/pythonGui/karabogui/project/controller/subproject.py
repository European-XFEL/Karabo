#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 26, 2016
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
from collections import defaultdict
from functools import partial

from qtpy.QtWidgets import QAction, QDialog, QMenu
from traits.api import Instance, List, on_trait_change

import karabogui.icons as icons
from karabo.common.api import walk_traits_object
from karabo.common.project.api import (
    DeviceInstanceModel, ProjectModel, device_instance_exists)
from karabo.common.project.utils import get_project_models
from karabogui import messagebox
from karabogui.access import AccessRole, access_role_allowed
from karabogui.project.dialog.project_handle import NewProjectDialog
from karabogui.project.loading_watcher import ProjectLoadingWatcher
from karabogui.project.utils import load_project
from karabogui.singletons.api import get_project_model
from karabogui.util import create_list_string, move_to_cursor

from .project_groups import ProjectSubgroupController


class SubprojectController(ProjectSubgroupController):
    """ A controller for 'subprojects' subgroups.
    """
    # These are subprojects which are asynchronously loading
    _watchers = List(Instance(ProjectLoadingWatcher))

    def context_menu(self, project_controller, parent=None):
        project_allowed = access_role_allowed(AccessRole.PROJECT_EDIT)

        menu = QMenu(parent)
        add_action = QAction(icons.add, 'Add new project...', menu)
        add_action.triggered.connect(partial(self._add_project, parent=parent))
        load_action = QAction(icons.load, 'Load project...', menu)
        load_action.triggered.connect(partial(self._load_project,
                                              parent=parent))
        add_action.setEnabled(project_allowed)
        load_action.setEnabled(project_allowed)

        menu.addAction(add_action)
        menu.addAction(load_action)
        return menu

    # ----------------------------------------------------------------------
    # action handlers

    def _add_project(self, parent=None):
        """ Add a new subproject to the associated project
        """
        dialog = NewProjectDialog(default=True, parent=parent)
        move_to_cursor(dialog)
        if dialog.exec() == QDialog.Accepted:
            # XXX: TODO check for existing
            project = ProjectModel(simple_name=dialog.simple_name)
            # Set initialized and modified last
            project.initialized = project.modified = True
            self.model.subprojects.append(project)

    def _load_project(self, parent=None):
        """ Add an existing project as a subproject.
        """
        project = load_project(is_subproject=True, parent=parent)
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
        duplicated = []

        def visitor(model):
            if isinstance(model, DeviceInstanceModel):
                device_ids.append(model.instance_id)

        # collect all the device ids in the project being added
        walk_traits_object(project, visitor)

        for project in get_project_models(root_project):
            if devices := device_instance_exists(project, device_ids):
                duplicated.extend(devices)

        if duplicated:
            msg = ('The subproject which you wish to load contains devices<br>'
                   'which already exist in the current project. Therefore <br>'
                   'it will not be added.')

            details = create_list_string(set(duplicated))
            messagebox.show_warning(msg, details=details,
                                    title='Device already exists')
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
            msg = ('Projects will be OR are loaded already multiple times! '
                   'Please investigate and have a look at the '
                   'project(s) <b>{}</b>! You have <b>{}</b> projects '
                   'that are loaded with an empty name!'.format(
                    project_names, empty_projects))
            messagebox.show_warning(msg, title='Load Subproject Failed')
            return True
        return False
