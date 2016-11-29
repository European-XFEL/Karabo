#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtGui import QDialog, QStackedLayout, QWidget

from karabo.common.project.api import DeviceServerModel
import karabo_gui.icons as icons
from karabo_gui.actions import KaraboAction, build_qaction
from karabo_gui.docktabwindow import Dockable
from karabo_gui.project.dialog.object_handle import ObjectSaveDialog
from karabo_gui.singletons.api import get_db_conn
from .dialog.project_handle import LoadProjectDialog, NewProjectDialog


class ProjectPanel(Dockable, QWidget):
    """ A dockable panel which contains a view of the project
    """
    def __init__(self, project_view):
        super(ProjectPanel, self).__init__()

        title = "Projects"
        self.setWindowTitle(title)

        self.project_view = project_view
        layout = QStackedLayout(self)
        layout.addWidget(self.project_view)
        self.setLayout(layout)

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

        qactions = []
        for k_action in (new, load, save):
            q_ac = build_qaction(k_action, self)
            item_model = self.project_view.model()
            q_ac.triggered.connect(partial(k_action.triggered, item_model))
            qactions.append(q_ac)
        return qactions

    def setupToolBars(self, toolbar, widget):
        """ Setup the project specific toolbar """
        qactions = self._create_actions()
        for ac in qactions:
            toolbar.addAction(ac)

    def onDock(self):
        pass

    def onUndock(self):
        pass


def _project_load_handler(item_model):
    """ Load a project model and assign it to the `item_model`

    :param item_model: The `ProjectItemModel` of the `ProjectView`
    """
    # XXX: HACK. This is only written this way to get _something_ loaded.
    # It must change when integrating into the full GUI
    from karabo.common.project.api import ProjectModel, read_lazy_object
    from karabo_gui.project.api import TEST_DOMAIN
    from karabo.middlelayer_api.newproject.io import read_project_model

    dialog = LoadProjectDialog()
    result = dialog.exec()
    if result == QDialog.Accepted:
        uuid, revision = dialog.selected_item()
        if uuid is not None and revision is not None:
            db_conn = get_db_conn()
            model = ProjectModel(uuid=uuid, revision=revision)
            read_lazy_object(TEST_DOMAIN, uuid, revision, db_conn,
                             read_project_model, existing=model)
            item_model.traits_data_model = model


def _project_new_handler(item_model):
    """ Create a new project model and assign it to the given `item_model`

    :param item_model: The `ProjectItemModel` of the `ProjectView`
    """
    from karabo.common.project.api import ProjectModel

    dialog = NewProjectDialog()
    if dialog.exec() == QDialog.Accepted:
        # This overwrites the current model
        model = ProjectModel(simple_name=dialog.simple_name, initialized=True)
        item_model.traits_data_model = model


def _project_save_handler(item_model):
    """ Save the project model of the given `item_model`

    :param item_model: The `ProjectItemModel` of the `ProjectView`
    """
    from karabo.common.project.api import PROJECT_OBJECT_CATEGORIES
    from karabo_gui.project.api import TEST_DOMAIN

    def store_obj(db_conn, obj):
        db_conn.store(TEST_DOMAIN, obj.uuid, obj.revision, obj)

    # XXX: This is saving EVERYTHING in the project, regardless of need.
    # XXX: Don't do this unless explicitly requested! Save objects individually
    proj_model = item_model.traits_data_model
    dialog = ObjectSaveDialog(proj_model)
    if dialog.exec() == QDialog.Accepted:
        proj_model.alias = dialog.alias
        db_conn = get_db_conn()
        for childname in PROJECT_OBJECT_CATEGORIES:
            children = getattr(proj_model, childname)
            for child in children:
                if isinstance(child, DeviceServerModel):
                    for dev_inst in child.devices:
                        for dev_conf in dev_inst.configs:
                            store_obj(db_conn, dev_conf)
                store_obj(db_conn, child)
        store_obj(db_conn, proj_model)
