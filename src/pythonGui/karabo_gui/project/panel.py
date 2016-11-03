#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtGui import QDialog, QStackedLayout, QWidget

import karabo_gui.icons as icons
from karabo_gui.actions import KaraboAction, build_qaction
from karabo_gui.docktabwindow import Dockable
from .dialog.project_handle import LoadDialog, NewDialog, SaveDialog


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
            text="&Open Project",
            tooltip="Open an Existing Project",
            triggered=_project_open_handler,
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


def _project_open_handler(item_model):
    """ Load a project model and assign it to the `item_model`

    :param item_model: The `ProjectItemModel` of the `ProjectView`
    """
    # XXX: HACK. This is only written this way to get _something_ loaded.
    # It must change when integrating into the full GUI
    from karabo.common.project.api import ProjectModel, read_lazy_object
    from karabo_gui.project.api import TEST_DOMAIN
    from karabo.middlelayer_api.newproject.io import read_project_model
    from karabo_gui.topology import Manager

    dialog = LoadDialog()
    result = dialog.exec()
    if result == QDialog.Accepted:
        item = dialog.selected_item()
        if item is not None:
            db_conn = Manager().proj_db_conn
            model = ProjectModel(uuid=item, revision=0)
            read_lazy_object(TEST_DOMAIN, item, 0, db_conn,
                             read_project_model, existing=model)
            item_model.traits_data_model = model


def _project_new_handler(item_model):
    """ Create a new project model and assign it to the given `item_model`

    :param item_model: The `ProjectItemModel` of the `ProjectView`
    """
    # XXX: HACK. This is only written this way to get _something_ loaded.
    # It must change when integrating into the full GUI
    from karabo.common.project.api import (get_user_cache, ProjectModel,
                                           read_lazy_object)
    from karabo_gui.project.api import TEST_DOMAIN
    from karabo.middlelayer_api.newproject.io import read_project_model
    dialog = NewDialog()
    if dialog.exec() == QDialog.Accepted:
        # XXX: TODO check for existing
        item = dialog.selected_item()
        if item is not None:
            cache = get_user_cache()
            model = read_lazy_object(TEST_DOMAIN, item, 0, cache,
                                     read_project_model)
        else:
            model = ProjectModel(simple_name=dialog.simple_name)
        item_model.traits_data_model = model


def _project_save_handler(item_model):
    """ Save the project model of the given `item_model`

    :param item_model: The `ProjectItemModel` of the `ProjectView`
    """
    # XXX: HACK. This is only written this way to get _something_ saved.
    # It must change when integrating into the full GUI
    from karabo.common.project.api import (get_user_cache,
                                           PROJECT_OBJECT_CATEGORIES)
    from karabo.middlelayer_api.newproject.io import write_project_model
    from karabo_gui.project.api import TEST_DOMAIN
    dialog = SaveDialog()
    if dialog.exec() == QDialog.Accepted:
        proj_model = item_model.traits_data_model
        storage = get_user_cache()
        for childname in PROJECT_OBJECT_CATEGORIES:
            children = getattr(proj_model, childname)
            for child in children:
                data = write_project_model(child)
                storage.store(TEST_DOMAIN, child.uuid, child.revision, data)
        data = write_project_model(proj_model)
        storage.store(TEST_DOMAIN, proj_model.uuid, proj_model.revision, data)
