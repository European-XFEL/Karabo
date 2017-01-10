#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtGui import QAction, QDialog, QStackedLayout, QWidget

from karabo.common.project.api import ProjectModel, read_lazy_object
from karabo.common.savable import set_modified_flag
from karabo.middlelayer_api.project.convert import convert_old_project
from karabo.middlelayer_api.project.old import Project
from karabo_gui.docktabwindow import Dockable
from karabo_gui.events import (
    register_for_broadcasts, KaraboBroadcastEvent, KaraboEventSender)
import karabo_gui.icons as icons
from karabo_gui.actions import KaraboAction, build_qaction
from karabo_gui.project.api import ProjectView
from karabo_gui.project.dialog.project_handle import (
    LoadProjectDialog, NewProjectDialog)
from karabo_gui.project.utils import save_project
from karabo_gui.singletons.api import get_db_conn
from karabo_gui.util import getOpenFileName


class ProjectPanel(Dockable, QWidget):
    """ A dockable panel which contains a view of the project
    """
    def __init__(self):
        super(ProjectPanel, self).__init__()

        # Register for broadcast events.
        # This object lives as long as the app. No need to unregister.
        register_for_broadcasts(self)

        title = "Projects"
        self.setWindowTitle(title)

        self._toolbar_actions = []

        self.project_view = ProjectView(parent=self)
        layout = QStackedLayout(self)
        layout.addWidget(self.project_view)
        self.setLayout(layout)

    def eventFilter(self, obj, event):
        """ Router for incoming broadcasts
        """
        if isinstance(event, KaraboBroadcastEvent):
            if event.sender is KaraboEventSender.NetworkConnectStatus:
                self._handle_network_status_change(event.data['status'])
            return False
        return super(ProjectPanel, self).eventFilter(obj, event)

    def _create_actions(self):
        """ Create actions and return list of them"""
        new = KaraboAction(
            icon=icons.new,
            text="&New Project",
            tooltip="Create a New (Sub)project",
            triggered=_project_new_handler,
        )
        load = KaraboAction(
            icon=icons.load,
            text="&Load Project",
            tooltip="Load an Existing Project",
            triggered=_project_load_handler,
        )
        save = KaraboAction(
            icon=icons.save,
            text="&Save Project",
            tooltip="Save Project Snapshot",
            triggered=_project_save_handler,
        )
        load_old = KaraboAction(
            icon=icons.load,
            text="&Load Legacy Project",
            tooltip="Load a Legacy Project",
            triggered=_old_project_load_handler,
        )

        qactions = []
        for k_action in (new, load, save, None, load_old):
            if k_action is None:
                q_ac = QAction(self)
                q_ac.setSeparator(True)
                qactions.append(q_ac)
                continue
            q_ac = build_qaction(k_action, self)
            q_ac.setEnabled(False)
            item_model = self.project_view.model()
            q_ac.triggered.connect(partial(k_action.triggered, item_model))
            qactions.append(q_ac)
        return qactions

    def setupToolBars(self, toolbar, widget):
        """ Setup the project specific toolbar """
        self._toolbar_actions = self._create_actions()
        for ac in self._toolbar_actions:
            toolbar.addAction(ac)

    def onDock(self):
        pass

    def onUndock(self):
        pass

    def _handle_network_status_change(self, status):
        if not status:
            # Don't show projects when there's no server connection
            self.project_view.destroy()

        for qaction in self._toolbar_actions:
            qaction.setEnabled(status)


def _project_load_handler(item_model):
    """ Load a project model and assign it to the `item_model`

    :param item_model: The `ProjectViewItemModel` of the `ProjectView`
    """
    # XXX: HACK. This is only written this way to get _something_ loaded.
    # It must change once the ProjectDB is fully supported
    from karabo_gui.project.api import TEST_DOMAIN
    from karabo.middlelayer_api.project.io import read_project_model

    dialog = LoadProjectDialog()
    result = dialog.exec()
    if result == QDialog.Accepted:
        uuid, revision = dialog.selected_item()
        if uuid is not None and revision is not None:
            db_conn = get_db_conn()
            model = ProjectModel(uuid=uuid, revision=revision)
            read_lazy_object(TEST_DOMAIN, uuid, revision, db_conn,
                             read_project_model, existing=model)
            set_modified_flag(model, value=False)
            item_model.traits_data_model = model


def _old_project_load_handler(item_model):
    """ Load an old project model and assign it to the `item_model`

    :param item_model: The `ProjectViewItemModel` of the `ProjectView`
    """
    fn = getOpenFileName(caption='Load Old Project',
                         filter='Legacy Karabo Projects (*.krb)')
    if not fn:
        return
    project = Project(fn)
    project.unzip()
    model, _ = convert_old_project(project)
    # Set modified flag recursively to True to make sure that EVERYTHING gets
    # saved to the database
    set_modified_flag(model, value=True)
    item_model.traits_data_model = model


def _project_new_handler(item_model):
    """ Create a new project model and assign it to the given `item_model`

    :param item_model: The `ProjectViewItemModel` of the `ProjectView`
    """
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
        save_project(traits_model)
