#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QAction, QDialog, QMenu, QMessageBox
from traits.api import (HasStrictTraits, Bool, Event, Instance, List, Property,
                        on_trait_change)

from karabo.common.api import walk_traits_object
from karabo.common.project.api import (
    BaseProjectObjectModel, DeviceInstanceModel, ProjectModel,
    device_instance_exists)
from karabo_gui.project.dialog.project_handle import NewProjectDialog
from karabo_gui.project.utils import load_project
from karabo_gui.singletons.api import get_project_model
from .project_groups import ProjectSubgroupController


class SubprojectController(ProjectSubgroupController):
    """ A controller for 'subprojects' subgroups.
    """
    # These are subprojects which are asynchronously loading
    _watchers = List(Instance('_ProjectLoadingWatcher'))

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
            # Set initialized and modified last
            project.initialized = project.modified = True
            self.model.subprojects.append(project)

    def _load_project(self):
        """ Add an existing project as a subproject.
        """
        # XXX: Hardcoding of the domain must be replaced with user selection!
        from karabo_gui.project.api import TEST_DOMAIN

        project = load_project(TEST_DOMAIN)
        if project is None:
            return

        # Loading projects is complex, due to asynchronicity. Use a helper
        # to determine when all the project data is present.
        watcher = _ProjectLoadingWatcher(project=project)
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


class _ProjectLoadingWatcher(HasStrictTraits):
    """An object which can be set as a traits notification handler for projects
    which are loading asynchronously from the database.
    """
    # A project to be loaded
    project = Instance(ProjectModel)
    # A property which can be checked by external code
    finished = Property(Bool)
    # When this event fires, the project is loaded!
    finished_event = Event

    # Project objects which are not yet loaded
    _loading_objects = List(Instance(BaseProjectObjectModel))

    def _get_finished(self):
        return len(self._loading_objects) == 0

    def _project_changed(self):
        if self.project is not None:
            self._watch_loading_object(self.project)

    def _notification_handler(self, obj, name, old, new):
        """Traits notification handler for loading objects.
        """
        if new:
            self._loading_objects.remove(obj)
            obj.on_trait_change(self._notification_handler, 'initialized',
                                remove=True)

            # It's possible that this object introduced more children which
            # now need to be watched
            self._watch_loading_object(obj)

        # Are we done yet?
        if self.finished:
            self.finished_event = True

    def _watch_loading_object(self, project_object):
        """Add a whole bunch of traits listeners to a project object and its
        children so that it can be determined when all of its children are
        loaded.
        """
        def visitor(model):
            if isinstance(model, BaseProjectObjectModel):
                if not model.initialized:
                    model.on_trait_change(self._notification_handler,
                                          'initialized')
                    self._loading_objects.append(model)

        walk_traits_object(project_object, visitor)
