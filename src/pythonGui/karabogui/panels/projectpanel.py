#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt5.QtCore import QSize
from PyQt5.QtWidgets import QDialog, QVBoxLayout, QWidget

from karabo.common.project.api import ProjectModel
from karabogui import icons
from karabogui.actions import build_qaction, KaraboAction
from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.project.dialog.project_handle import NewProjectDialog
from karabogui.project.utils import (
    load_project, maybe_save_modified_project, reload_project, save_object,
    show_modified_project_message)
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
        super(ProjectPanel, self).__init__("Projects")

        # Register for broadcast events.
        # This object lives as long as the app. No need to unregister.
        event_map = {
            KaraboEvent.NetworkConnectStatus: self._event_network,
            KaraboEvent.DatabaseIsBusy: self._event_db_busy,
            KaraboEvent.ProjectFilterUpdated: self._event_filter_updated
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

        for k_action in (new, load, save, reload, trash):
            q_ac = build_qaction(k_action, self)
            q_ac.setEnabled(False)
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

    def _event_db_busy(self, data):
        loading_failed = data.get('loading_failed', False)
        is_processing = data['is_processing']
        if loading_failed:
            self._enable_partial_toolbar()
        else:
            self._enable_toolbar(not is_processing)
            self.sbar.reset(not is_processing)
        self.spin_action.setVisible(is_processing)

    def _event_filter_updated(self, data):
        self.sbar.reset(data['status'])

    def _event_network(self, data):
        status = data['status']
        if not status:
            # Don't show projects when there's no server connection
            self.tree_view.destroy()

        self._enable_toolbar(status)
        if not status:
            self.sbar.reset(status)

    # -----------------------------------------------------------------------

    def _enable_toolbar(self, enable):
        for qaction in self._toolbar_actions:
            qaction.setEnabled(enable)

    def _enable_partial_toolbar(self):
        """ Project loading failed, restrict options
        """
        for qaction in self._toolbar_actions:
            if qaction.objectName() in ("new", "load"):
                qaction.setEnabled(True)

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

    project = reload_project(root_model)
    # We created a new project object!
    if project is not None:
        item_model.root_model = project


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


def _project_save_handler(project_view):
    """ Save the project model (`ProjectViewItemModel`) of the given
    `project_view`

    :param project_view: The `ProjectView` of the panel
    """
    item_model = project_view.model()
    root_model = item_model.root_model
    if root_model is not None:
        save_object(root_model)


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
