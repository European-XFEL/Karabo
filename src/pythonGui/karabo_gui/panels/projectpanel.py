#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtCore import QSize
from PyQt4.QtGui import QAction, QDialog

from karabo.common.api import set_modified_flag
from karabo.common.project.api import ProjectModel
from karabo.middlelayer import OldProject, convert_old_project
from karabo_gui.events import (
    register_for_broadcasts, KaraboBroadcastEvent, KaraboEventSender)
import karabo_gui.icons as icons
from karabo_gui.actions import KaraboAction, build_qaction
from karabo_gui.project.api import ProjectView
from karabo_gui.project.dialog.project_handle import NewProjectDialog
from karabo_gui.project.utils import (
    load_project, maybe_save_modified_project, save_object)
from karabo_gui.toolbar import ToolBar
from karabo_gui.util import getOpenFileName, get_spin_widget
from .base import BasePanelWidget


class ProjectPanel(BasePanelWidget):
    """ A dockable panel which contains a view of the project
    """
    def __init__(self):
        super(ProjectPanel, self).__init__("Projects")

        # Register for broadcast events.
        # This object lives as long as the app. No need to unregister.
        register_for_broadcasts(self)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        self.project_view = ProjectView()
        return self.project_view

    def toolbars(self):
        """This should create and return one or more `ToolBar` instances needed
        by this panel.
        """
        self._toolbar_actions = []
        new = KaraboAction(
            icon=icons.new, text="&New Project",
            tooltip="Create a New (Sub)project",
            triggered=_project_new_handler,
        )
        load = KaraboAction(
            icon=icons.load, text="&Load Project",
            tooltip="Load an Existing Project",
            triggered=_project_load_handler,
        )
        save = KaraboAction(
            icon=icons.save, text="&Save Project",
            tooltip="Save Project Snapshot",
            triggered=_project_save_handler,
        )
        load_old = KaraboAction(
            icon=icons.load, text="&Load Legacy Project",
            tooltip="Load a Legacy Project",
            triggered=_old_project_load_handler,
        )

        for k_action in (new, load, save, None, load_old):
            if k_action is None:
                q_ac = QAction(self)
                q_ac.setSeparator(True)
                self._toolbar_actions.append(q_ac)
            else:
                q_ac = build_qaction(k_action, self)
                q_ac.setEnabled(False)
                item_model = self.project_view.model()
                q_ac.triggered.connect(partial(k_action.triggered, item_model))
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

    def eventFilter(self, obj, event):
        """ Router for incoming broadcasts
        """
        if isinstance(event, KaraboBroadcastEvent):
            data = event.data
            if event.sender is KaraboEventSender.NetworkConnectStatus:
                self._handle_network_status_change(data['status'])
            elif event.sender is KaraboEventSender.DatabaseIsBusy:
                is_processing = data['is_processing']
                self._enable_toolbar(not is_processing)
                # Show or hide spin widget
                self.spin_action.setVisible(is_processing)
            return False
        return super(ProjectPanel, self).eventFilter(obj, event)

    def _handle_network_status_change(self, status):
        if not status:
            # Don't show projects when there's no server connection
            self.project_view.destroy()

        self._enable_toolbar(status)

    def _enable_toolbar(self, enable):
        for qaction in self._toolbar_actions:
            qaction.setEnabled(enable)


def _project_load_handler(item_model):
    """ Load a project model and assign it to the `item_model`

    :param item_model: The `ProjectViewItemModel` of the `ProjectView`
    """
    # XXX: Hardcoding of the domain must be replaced with user selection!
    from karabo_gui.project.api import TEST_DOMAIN

    # Check for modififications before showing dialog
    traits_data_model = item_model.traits_data_model
    if not maybe_save_modified_project(traits_data_model):
        return

    project = load_project(TEST_DOMAIN)
    if project is not None:
        item_model.traits_data_model = project


def _old_project_load_handler(item_model):
    """ Load an old project model and assign it to the `item_model`

    :param item_model: The `ProjectViewItemModel` of the `ProjectView`
    """
    # Check for modififications before showing dialog
    traits_data_model = item_model.traits_data_model
    if not maybe_save_modified_project(traits_data_model):
        return

    fn = getOpenFileName(caption='Load Old Project',
                         filter='Legacy Karabo Projects (*.krb)')
    if not fn:
        return

    project = OldProject(fn)
    project.unzip()
    model = convert_old_project(project)
    # Set modified flag recursively to True to make sure that EVERYTHING gets
    # saved to the database
    set_modified_flag(model, value=True)
    item_model.traits_data_model = model


def _project_new_handler(item_model):
    """ Create a new project model and assign it to the given `item_model`

    :param item_model: The `ProjectViewItemModel` of the `ProjectView`
    """
    # Check for modififications before showing dialog
    traits_data_model = item_model.traits_data_model
    if not maybe_save_modified_project(traits_data_model):
        return

    dialog = NewProjectDialog()
    if dialog.exec() == QDialog.Accepted:
        # This overwrites the current model
        model = ProjectModel(simple_name=dialog.simple_name, initialized=True,
                             modified=True)
        item_model.traits_data_model = model


def _project_save_handler(item_model):
    """ Save the project model of the given `item_model`

    :param item_model: The `ProjectViewItemModel` of the `ProjectView`
    """
    traits_model = item_model.traits_data_model
    if traits_model is not None:
        save_object(traits_model)
