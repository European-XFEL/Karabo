#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QAction, QCursor, QMessageBox, QTreeView

from karabo.common.project.api import ProjectModel, find_parent_object
from karabo_gui.project.utils import maybe_save_modified_project, save_object
from karabo_gui.singletons.api import get_project_model, get_selection_tracker
from karabo_gui.util import is_database_processing
from .controller.bases import BaseProjectGroupController
from .controller.project import ProjectController
from .controller.project_groups import ProjectSubgroupController


class ProjectView(QTreeView):
    """ An object representing the view for a Karabo project
    """
    def __init__(self, parent=None):
        super(ProjectView, self).__init__(parent)

        project_model = get_project_model()
        self.setModel(project_model)
        project_model.rowsInserted.connect(self._items_added)
        self.setSelectionModel(project_model.q_selection_model)
        self.selectionModel().selectionChanged.connect(self._selection_change)

        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self._show_context_menu)

    # ----------------------------
    # Public methods

    def destroy(self):
        """ Unset project's data model before death.
        """
        self.model().traits_data_model = None

    # ----------------------------
    # Qt methods

    def closeEvent(self, event):
        self.destroy()
        event.accept()

    def mouseDoubleClickEvent(self, event):
        selected_controller = self._get_selected_controller()
        if selected_controller is not None:
            parent_project = self._parent_project(selected_controller)
            selected_controller.double_click(parent_project, parent=self)

            if isinstance(selected_controller, BaseProjectGroupController):
                # Double clicks expand groups
                indices = self.selectionModel().selectedIndexes()
                self.expand(indices[0])

    # ----------------------------
    # Slots

    def _items_added(self, index, start, end):
        """React to the addition of an item (or items).
        """
        # Bail immediately if not the first item
        if start != 0:
            return

        parent_index = index.parent()
        controller = self.model().controller_ref(parent_index)
        if (controller is not None and
                isinstance(controller, BaseProjectGroupController)):
            # If a group just added its first item, expand it
            self.expand(parent_index)

    def _selection_change(self, selected, deselected):
        """ Notify controller objects when their Qt list item object is
        selected.
        """
        selected_controller = self._get_selected_controller()
        if selected_controller is not None:
            parent_project = self._parent_project(selected_controller)
            selected_controller.single_click(parent_project, parent=self)

            # Grab control of the global selection
            get_selection_tracker().grab_selection(self.selectionModel())

    def _show_context_menu(self):
        """ Show a context menu for the currently selected item.
        """
        # Make sure there are not pending DB things in the pipe
        if is_database_processing():
            return

        selected_controller = self._get_selected_controller()
        if selected_controller is not None:
            parent_project = self._parent_project(selected_controller)
            menu = selected_controller.context_menu(parent_project,
                                                    parent=self)
            is_project = isinstance(selected_controller, ProjectController)
            if parent_project is None or is_project:
                project_model = selected_controller.model
                save_action = QAction('Save', menu)
                save_action.triggered.connect(partial(save_object,
                                                      project_model))
                close_action = QAction('Close project', menu)
                close_action.triggered.connect(partial(self._close_project,
                                                       project_model,
                                                       parent_project))
                menu.addAction(save_action)
                menu.addAction(close_action)

            menu.exec(QCursor.pos())

    # ----------------------------
    # Private methods

    def _get_selected_controller(self):
        """ Return the currently selected controller.
        """
        indices = self.selectionModel().selectedIndexes()
        if not indices:
            return None

        first_index = indices[0]
        return self.model().controller_ref(first_index)

    def _parent_project(self, controller):
        """ Find the parent project model of a given controller
        """
        if isinstance(controller, ProjectSubgroupController):
            return controller.model
        root_project = self.model().traits_data_model
        return find_parent_object(controller.model, root_project, ProjectModel)

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

        if parent_project is not None:
            # Check for modififications before closing
            if not maybe_save_modified_project(project):
                return
            # A subproject
            if project in parent_project.subprojects:
                parent_project.subprojects.remove(project)
        else:
            # Check for modififications before closing
            model = self.model().traits_data_model
            if not maybe_save_modified_project(model):
                return
            # The master project
            self.model().traits_data_model = None
