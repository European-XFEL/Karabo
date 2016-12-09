#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QAction, QCursor, QMessageBox, QTreeView

from karabo.common.project.api import ProjectModel, find_parent_object
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
from karabo_gui.project.utils import save_project
from karabo_gui.singletons.api import get_project_model
from .model.project import ProjectModelItem
from .model.project_groups import ProjectSubgroupItem


class ProjectView(QTreeView):
    """ An object representing the view for a Karabo project
    """
    def __init__(self, parent=None):
        super(ProjectView, self).__init__(parent)

        self.setModel(get_project_model())
        self.selectionModel().selectionChanged.connect(self._selection_change)

        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self._show_context_menu)

    # ----------------------------
    # Qt methods

    def closeEvent(self, event):
        self.destroy()
        event.accept()

    def mouseDoubleClickEvent(self, event):
        selected_item_model = self._get_selected_model()
        if selected_item_model is not None:
            parent_project = self._parent_project(selected_item_model)
            selected_item_model.double_click(parent_project, parent=self)

    # ----------------------------
    # Public methods

    def destroy(self):
        """ Do some cleanup of the project's objects before death.
        """
        self.model().traits_data_model = None

    # ----------------------------
    # Private methods

    def _get_selected_model(self):
        """ Return the currently selected item model.
        """
        indices = self.selectionModel().selectedIndexes()
        if not indices:
            return None

        first_index = indices[0]
        model_ref = first_index.data(PROJECT_ITEM_MODEL_REF)
        return model_ref()

    def _parent_project(self, model):
        """ Find the parent project model of a given item model
        """
        if isinstance(model, (ProjectModelItem, ProjectSubgroupItem)):
            return model.model
        root_project = self.model().traits_data_model
        return find_parent_object(model.model, root_project, ProjectModel)

    def _selection_change(self, selected, deselected):
        """ Notify item model objects when their Qt list item object is
        selected.
        """
        selected_item_model = self._get_selected_model()
        if selected_item_model is not None:
            parent_project = self._parent_project(selected_item_model)
            selected_item_model.single_click(parent_project, parent=self)

    def _show_context_menu(self):
        """ Show a context menu for the currently selected item.
        """
        selected_item_model = self._get_selected_model()
        if selected_item_model is not None:
            parent_project = self._parent_project(selected_item_model)
            menu = selected_item_model.context_menu(parent_project,
                                                    parent=self)
            is_project = isinstance(selected_item_model, ProjectModelItem)
            if parent_project is None or is_project:
                project_model = selected_item_model.model
                save_action = QAction('Save', menu)
                save_action.triggered.connect(partial(save_project,
                                                      project_model))
                close_action = QAction('Close project', menu)
                close_action.triggered.connect(partial(self._close_project,
                                                       project_model,
                                                       parent_project))
                menu.addAction(save_action)
                menu.addAction(close_action)

            menu.exec(QCursor.pos())

    def _save_project(self, project):
        # Save possible changes
        if project.modified:
            ask = ('The project has be modified.<br />Do you want to save the '
                   'project?')
            options = (QMessageBox.Save | QMessageBox.Cancel)
            reply = QMessageBox.question(None, 'Save project',
                                         ask, options, QMessageBox.Save)
            if reply == QMessageBox.Cancel:
                return

            if reply == QMessageBox.Save:
                save_project(project)

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

        self._save_project(project)

        if project is not parent_project:
            # A subproject
            if project in parent_project.subprojects:
                parent_project.subprojects.remove(project)
        else:
            # The master project
            self.model().traits_data_model = None
