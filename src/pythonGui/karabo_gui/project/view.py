#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QAction, QCursor, QMenu, QMessageBox, QTreeView

from karabo.common.project.api import ProjectModel, find_parent_object
from karabo.middlelayer_api.newproject.io import write_project_model
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
from .item_model import ProjectItemModel
from .model.project import ProjectModelItem
from .model.project_groups import ProjectSubgroupItem


class ProjectView(QTreeView):
    """ An object representing the view for a Karabo project
    """
    def __init__(self, parent=None):
        super(ProjectView, self).__init__(parent)

        item_model = ProjectItemModel(self)
        self.setModel(item_model)

        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self._show_context_menu)

    # ----------------------------
    # Qt methods

    def closeEvent(self, event):
        self.destroy()
        event.accept()

    # ----------------------------
    # Public methods

    def destroy(self):
        """ Do some cleanup of the project's objects before death.
        """
        self.model().traits_data_model = None

    # ----------------------------
    # Private methods

    def _show_context_menu(self):
        """ Show a context menu for the currently selected item.
        """
        def _parent_project(model):
            if isinstance(model, (ProjectItemModel, ProjectSubgroupItem)):
                return model.model
            root_project = self.model().traits_data_model
            return find_parent_object(model.model, root_project, ProjectModel)

        indices = self.selectionModel().selectedIndexes()
        if not indices:
            return

        first_index = indices[0]
        model_ref = first_index.data(PROJECT_ITEM_MODEL_REF)
        clicked_item_model = model_ref()
        if clicked_item_model is not None:
            parent_project = _parent_project(clicked_item_model)
            is_project = isinstance(clicked_item_model, ProjectModelItem)
            if parent_project is None or is_project:
                menu = QMenu(self)
                close_action = QAction('Close project', menu)
                project_model = clicked_item_model.model
                close_action.triggered.connect(partial(self._close_project,
                                                       project_model,
                                                       parent_project))
                menu.addAction(close_action)
            else:
                menu = clicked_item_model.context_menu(parent_project,
                                                       parent=self)
            menu.exec(QCursor.pos())

    def _close_project(self, project, parent_project):
        """ Close the given `project`
        """
        ask = 'Do you really want to close the project \"<b>{}</b>\"?'.format(
            project.simple_name)
        reply = QMessageBox.question(None, 'Close project', ask,
                                     QMessageBox.Yes | QMessageBox.No,
                                     QMessageBox.No)
        if reply == QMessageBox.No:
            return

        # Save possible changes - XXX TODO check `modified` flag
        if project.modified:
            ask = """
            The project has be modified.<br />Do you want to save the project
            before closing?
            """
            options = (QMessageBox.Save | QMessageBox.Discard |
                       QMessageBox.Cancel)
            reply = QMessageBox.question(None, 'Save changes before closing',
                                         ask, options, QMessageBox.Save)
            if reply == QMessageBox.Cancel:
                return

            if reply == QMessageBox.Save:
                write_project_model(project)

        if parent_project is not None:
            # A subproject
            if project in parent_project.subprojects:
                parent_project.subprojects.remove(project)
        else:
            # The master project
            self.model().traits_data_model = None
