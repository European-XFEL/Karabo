#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtCore import QSize
from PyQt4.QtGui import QDialog, QVBoxLayout, QWidget

from karabo.common.project.api import ProjectModel
from karabogui import icons
from karabogui.actions import build_qaction, KaraboAction
from karabogui.events import KaraboEventSender, register_for_broadcasts
from karabogui.project.dialog.project_handle import NewProjectDialog
from karabogui.project.utils import (
    load_project, maybe_save_modified_project, save_object)
from karabogui.project.view import ProjectView
from karabogui.singletons.api import get_db_conn
from karabogui.util import get_spin_widget
from karabogui.widgets.toolbar import ToolBar
from .base import BasePanelWidget, Searchable


class ProjectPanel(Searchable, BasePanelWidget):
    """ A dockable panel which contains a view of the project
    """
    def __init__(self):
        super(ProjectPanel, self).__init__("Projects")
        self._init_search_filter()

        # Register for broadcast events.
        # This object lives as long as the app. No need to unregister.
        register_for_broadcasts(self)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(self)
        main_layout = QVBoxLayout(widget)
        main_layout.setContentsMargins(5, 5, 5, 5)

        self.project_view = ProjectView()
        h_layout = self.create_search_bar(self.project_view.model())
        main_layout.addLayout(h_layout)

        main_layout.addWidget(self.project_view)
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
        trash = KaraboAction(
            icon=icons.delete, text="&Move to trash",
            tooltip=("Move the current project to trash. This option can be "
                     "reverted"),
            triggered=_project_trash_handler,
            name="delete"
        )

        for k_action in (new, load, save, trash):
            q_ac = build_qaction(k_action, self)
            q_ac.setEnabled(False)
            project_view = self.project_view
            q_ac.triggered.connect(partial(k_action.triggered,
                                   project_view))
            self._toolbar_actions.append(q_ac)

        toolbar = ToolBar(parent=self)
        for ac in self._toolbar_actions:
            toolbar.addAction(ac)

        # Add a spinner which is visible whenever database is processing
        toolbar.add_expander()
        spin_widget = get_spin_widget(scaled_size=QSize(20, 20),
                                      parent=toolbar)
        spin_widget.setVisible(False)
        self.spin_action = toolbar.addWidget(spin_widget)

        return [toolbar]

    def karaboBroadcastEvent(self, event):
        """ Router for incoming broadcasts
        """
        data = event.data
        if event.sender is KaraboEventSender.NetworkConnectStatus:
            self._handle_network_status_change(data['status'])
        elif event.sender is KaraboEventSender.DatabaseIsBusy:
            self._handle_database_is_busy(data)
        elif event.sender is KaraboEventSender.ProjectFilterUpdated:
            self._init_search_filter(data['status'])
            # we are the only one interested!
            return True
        return False

    def _handle_network_status_change(self, status):
        if not status:
            # Don't show projects when there's no server connection
            self.project_view.destroy()

        self._enable_toolbar(status)

        if not status:
            self._init_search_filter(status)

    def _handle_database_is_busy(self, data):
        loading_failed = data.get('loading_failed', False)
        is_processing = data['is_processing']
        if loading_failed:
            self._enable_partial_toolbar()
        else:
            self._enable_toolbar(not is_processing)
            self._init_search_filter(not is_processing)
        self.spin_action.setVisible(is_processing)

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
    if not maybe_save_modified_project(root_model):
        return

    project = load_project()
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
    if not maybe_save_modified_project(root_model):
        return

    dialog = NewProjectDialog()
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
