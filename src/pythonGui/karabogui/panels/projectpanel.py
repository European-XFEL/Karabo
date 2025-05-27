#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
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
from functools import partial

from qtpy.QtCore import QSize
from qtpy.QtWidgets import QDialog, QVBoxLayout, QWidget

from karabo.common.project.api import ProjectModel
from karabogui import icons
from karabogui.access import AccessRole, access_role_allowed
from karabogui.actions import KaraboAction, build_qaction
from karabogui.events import (
    KaraboEvent, broadcast_event, register_for_broadcasts)
from karabogui.logger import get_logger
from karabogui.project.dialog.project_handle import NewProjectDialog
from karabogui.project.restore import add_restore, get_restore_data
from karabogui.project.utils import (
    load_project, load_project_with_item, maybe_save_modified_project,
    reload_project, save_object, show_modified_project_message,
    show_reload_scene_project_message)
from karabogui.project.view import ProjectView
from karabogui.singletons.api import get_db_conn
from karabogui.util import get_spin_widget
from karabogui.widgets.toolbar import ToolBar

from .base import BasePanelWidget
from .searchwidget import SearchBar


class ProjectPanel(BasePanelWidget):
    """ A dockable panel which contains a view of the project
    """

    def __init__(self):
        super().__init__("Projects")
        # Bool to set if the project manager version is okay for the
        # load with device action.
        self._manager_version = False

        # Register for broadcast events.
        # This object lives as long as the app. No need to unregister.
        event_map = {
            KaraboEvent.NetworkConnectStatus: self._event_network,
            KaraboEvent.DatabaseIsBusy: self._event_db_busy,
            KaraboEvent.ProjectFilterUpdated: self._event_filter_updated,
            KaraboEvent.AccessLevelChanged: self._event_access_level,
            KaraboEvent.LoginUserChanged: self._event_access_level,
            KaraboEvent.ShowProjectDevice: self._event_show_project_device,
            KaraboEvent.ShowProjectModel: self._event_select_project_model,
        }
        register_for_broadcasts(event_map)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(self)
        main_layout = QVBoxLayout(widget)
        main_layout.setContentsMargins(2, 2, 2, 2)

        self.tree_view = ProjectView(parent=widget)
        self.sbar = SearchBar(parent=widget)
        model = self.tree_view.model()
        self.sbar.setModel(model)

        main_layout.addWidget(self.sbar)
        main_layout.addWidget(self.tree_view)

        return widget

    def __repr__(self):
        qt_model = self.tree_view.model()
        root = qt_model.root_model
        project_name = root.simple_name if root is not None else None
        return f"<ProjectPanel project={project_name}>"

    def toolbars(self):
        """This should create and return one or more `ToolBar` instances needed
        by this panel.
        """
        self._toolbar_actions = []
        new = KaraboAction(
            icon=icons.new, text="&New Project",
            tooltip="Create a New (Sub)project",
            triggered=_project_new_handler,
            name="new"
        )
        load = KaraboAction(
            icon=icons.load, text="&Load Project",
            tooltip="Load an Existing Project",
            triggered=_project_load_handler,
            name="load"
        )
        load_with_item = KaraboAction(
            icon=icons.filein, text="Find and &Load Project with Item Names",
            tooltip="Find and Load Project with Item Names",
            triggered=_project_with_device_load_handler,
            name="load_with_item"
        )
        save = KaraboAction(
            icon=icons.save, text="&Save Project",
            tooltip="Save Project Snapshot",
            triggered=_project_save_handler,
            name="save"
        )
        reload = KaraboAction(
            icon=icons.reset, text="&Reload Project",
            tooltip="Reload the current project",
            triggered=_project_reload_handler,
            name="reload"
        )
        trash = KaraboAction(
            icon=icons.delete, text="&Declare as trashed or untrashed",
            tooltip=("Mark a project as trashed or untrashed depending on "
                     "the project state."),
            triggered=_project_trash_handler,
            name="declare"
        )

        for k_action in (new, load, load_with_item, save, reload, trash):
            q_ac = build_qaction(k_action, self)
            q_ac.setEnabled(True)
            q_ac.setVisible(False)
            tree_view = self.tree_view
            q_ac.triggered.connect(partial(k_action.triggered,
                                           tree_view))
            self._toolbar_actions.append(q_ac)

        toolbar = ToolBar(parent=self)
        for ac in self._toolbar_actions:
            toolbar.addAction(ac)

        # Add a spinner which is visible whenever database is processing
        toolbar.add_expander()
        spin_widget = get_spin_widget(icon='wait-black',
                                      scaled_size=QSize(20, 20),
                                      parent=toolbar)
        spin_widget.setVisible(False)
        self.spin_action = toolbar.addWidget(spin_widget)

        return [toolbar]

    # -----------------------------------------------------------------------
    # Karabo Events

    def _event_access_level(self, data):
        self._access_toolbar()

    def _event_db_busy(self, data):
        loading_failed = data.get('loading_failed', False)
        is_processing = data['is_processing']
        if loading_failed:
            self._visible_partial_toolbar()
        else:
            self._set_toolbar_visible(not is_processing)
            self.sbar.reset(not is_processing)
        self.spin_action.setVisible(is_processing)

    def _event_filter_updated(self, data):
        self.sbar.reset(data['status'])

    def _event_network(self, data):
        status = data['status']
        if not status:
            # Don't show projects when there's no server connection
            self.tree_view.destroy()
            # We lost track about the project manager
            self._manager_version = False

        self._set_toolbar_visible(status)
        if not status:
            self.sbar.reset(status)

    def _event_show_project_device(self, data):
        """Event to show a project device """
        model = self.tree_view.model()
        nodes = model.findNodes(data.get("deviceId"), full_match=True)
        assert len(nodes) <= 1
        if nodes:
            # Select first entry
            model.selectNode(nodes[0])

    def _event_select_project_model(self, data):
        """Event to show a project device """
        model = self.tree_view.model()
        model.selectModel(data.get("model"))

    # -----------------------------------------------------------------------

    def _set_toolbar_visible(self, enable):
        for qaction in self._toolbar_actions:
            qaction.setVisible(enable)

    def _visible_partial_toolbar(self):
        """ Project loading failed, restrict options
        """
        for qaction in self._toolbar_actions:
            if qaction.objectName() in ("new", "load"):
                qaction.setVisible(True)

    def _access_toolbar(self):
        enable = access_role_allowed(AccessRole.PROJECT_EDIT)
        for qaction in self._toolbar_actions:
            if qaction.objectName() in ("new", "save", "declare"):
                qaction.setEnabled(enable)


# ------------------------------------------------------------------------
# Helper functions


def _project_load_handler(project_view):
    """ Load a project model (`ProjectViewItemModel`) and assign it to the
    `ProjectViewItemModel` of the given `project_view`

    :param project_view: The `ProjectView` of the panel
    """
    item_model = project_view.model()
    # Check for modififications before showing dialog
    root_model = item_model.root_model
    if not maybe_save_modified_project(root_model, parent=project_view):
        return

    project = load_project(parent=project_view)
    if project is not None:
        item_model.root_model = project


def _project_with_device_load_handler(project_view):
    """ Load a project with a user specified device, assigning it to the
    `ProjectViewItemModel` of the given `project_view`.

    :param project_view: The `ProjectView` of the panel
    """
    item_model = project_view.model()
    # Check for modififications before showing dialog
    root_model = item_model.root_model
    if not maybe_save_modified_project(root_model, parent=project_view):
        return

    project = load_project_with_item(parent=project_view)
    if project is not None:
        item_model.root_model = project


def _project_reload_handler(project_view):
    """ Reload a project model (`ProjectViewItemModel`)

    :param project_view: The `ProjectView` of the panel
    """
    item_model = project_view.model()
    root_model = item_model.root_model
    if root_model is None:
        return
    if not show_modified_project_message(root_model, parent=project_view):
        return

    data = get_restore_data()
    restore = show_reload_scene_project_message(data)
    project = reload_project(root_model)
    # We created a new project object!
    if project is not None:
        item_model.root_model = project
        text = (f"Reloading project <b>{root_model.simple_name}</b> from "
                "project database")
        get_logger().info(text)
        if restore:
            add_restore(project=project, scene_data=data)


def _project_new_handler(project_view):
    """ Create a new project model (`ProjectViewItemModel`) and assign it to
    the given `project_view`

    :param project_view: The `ProjectView` of the panel
    """
    item_model = project_view.model()
    # Check for modififications before showing dialog
    root_model = item_model.root_model
    if not maybe_save_modified_project(root_model, parent=project_view):
        return

    dialog = NewProjectDialog(parent=project_view)
    if dialog.exec() == QDialog.Accepted:
        # Set domain
        get_db_conn().default_domain = dialog.domain
        # This overwrites the current model
        model = ProjectModel(simple_name=dialog.simple_name, initialized=True,
                             modified=True)
        item_model.root_model = model
        text = f"Creating new project <b>{dialog.simple_name}</b>"
        get_logger().info(text)
        broadcast_event(KaraboEvent.ProjectName,
                        {"simple_name": model.simple_name})


def _project_save_handler(project_view):
    """ Save the project model (`ProjectViewItemModel`) of the given
    `project_view`

    :param project_view: The `ProjectView` of the panel
    """
    item_model = project_view.model()
    root_model = item_model.root_model
    if root_model is not None:
        save_object(root_model)
        text = (f"Request to save project <b>{root_model.simple_name}</b> in "
                "the project database")
        get_logger().info(text)
        broadcast_event(KaraboEvent.ProjectName,
                        {"simple_name": root_model.simple_name})


def _project_trash_handler(project_view):
    """ Move the project model (`ProjectViewItemModel`) of the given
    `project_view` to trash

    :param project_view: The `ProjectView` of the panel
    """
    item_model = project_view.model()
    root_model = item_model.root_model
    if root_model is not None:
        project_view.update_is_trashed(project=root_model,
                                       project_controller=None)
        text = ("Changing the <b>trash</b> attribute of the project "
                f"<b>{root_model.simple_name}</b>")
        get_logger().warning(text)
