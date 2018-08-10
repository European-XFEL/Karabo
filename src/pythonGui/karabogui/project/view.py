#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QAction, QDialog, QCursor, QMessageBox, QTreeView

from karabo.common.project.api import find_parent_object
from karabogui.events import KaraboEventSender, broadcast_event
from karabogui.project.dialog.project_handle import NewProjectDialog
from karabogui.project.utils import (
    maybe_save_modified_project, save_as_object, save_object)
from karabogui.singletons.api import (get_db_conn, get_project_model,
                                      get_selection_tracker)
from karabogui.util import is_database_processing, set_treeview_header
from .controller.bases import BaseProjectGroupController
from .controller.device import DeviceInstanceController
from .controller.project import ProjectController
from .controller.project_groups import ProjectSubgroupController
from .controller.subproject import SubprojectController


class ProjectView(QTreeView):
    """ An object representing the view for a Karabo project
    """

    def __init__(self, parent=None):
        super(ProjectView, self).__init__(parent)

        project_model = get_project_model()
        project_model.setParent(self)

        self.setModel(project_model)
        project_model.rowsInserted.connect(self._items_added)
        self.setSelectionModel(project_model.q_selection_model)
        self.selectionModel().selectionChanged.connect(self._selection_change)

        set_treeview_header(self)

        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self._show_context_menu)

    # ----------------------------
    # Public methods

    def destroy(self):
        """ Unset project's data model before death.
        """
        self.model().root_model = None

    # ----------------------------
    # Qt methods

    def closeEvent(self, event):
        self.destroy()
        event.accept()

    def mouseDoubleClickEvent(self, event):
        selected_controller = self._get_selected_controller()
        if selected_controller is not None:
            project_controller = self._project_controller(selected_controller)
            selected_controller.double_click(project_controller, parent=self)

            if isinstance(selected_controller, BaseProjectGroupController):
                # Double clicks expand groups
                indices = self.selectionModel().selectedIndexes()
                self.expand(indices[0])

    # ----------------------------
    # Slots

    @pyqtSlot(object, int, int)
    def _items_added(self, parent_index, start, end):
        """React to the addition of an item (or items).
        """
        # Bail immediately if not the first item
        if start != 0:
            return

        controller = self.model().controller_ref(parent_index)
        if (controller is not None and
                isinstance(controller, BaseProjectGroupController) and not
                isinstance(controller, DeviceInstanceController)):
            # If a group just added its first item, expand it
            # except for device instances
            self.expand(parent_index)

    @pyqtSlot(object, object)
    def _selection_change(self, selected, deselected):
        """ Notify controller objects when their Qt list item object is
        selected.
        """
        # NOTE: There are two project views at all items, because the
        # configurator has a $%@#ing hidden navigation/project pair.
        # Avoid doing double the work...
        if not self.hasFocus():
            return

        selected_controller = self._get_selected_controller()
        if selected_controller is not None:
            project_controller = self._project_controller(selected_controller)
            selected_controller.single_click(project_controller, parent=self)

            # Grab control of the global selection
            get_selection_tracker().grab_selection(self.selectionModel())

    @pyqtSlot()
    def _show_context_menu(self):
        """ Show a context menu for the currently selected item.
        """
        # Make sure there are not pending DB things in the pipe
        if is_database_processing():
            return

        selected_controller = self._get_selected_controller()
        if selected_controller is not None:
            project_controller = self._project_controller(selected_controller)
            menu = selected_controller.context_menu(project_controller,
                                                    parent=self)
            if isinstance(selected_controller, ProjectController):
                selected_project = selected_controller.model
                rename_action = QAction('Rename', menu)
                rename_action.triggered.connect(partial(self._rename_project,
                                                        selected_project))
                save_action = QAction('Save', menu)
                save_action.triggered.connect(partial(save_object,
                                                      selected_project,
                                                      domain=None))
                save_as_action = QAction('Save as...', menu)
                save_as_action.triggered.connect(partial(save_as_object,
                                                         selected_project))
                close_action = QAction('Close project', menu)
                close_action.triggered.connect(partial(self._close_project,
                                                       selected_project,
                                                       project_controller,
                                                       show_dialog=True))
                is_trashed = selected_project.is_trashed
                if is_trashed:
                    text = 'Restore from trash'
                else:
                    text = 'Move to trash'
                trash_action = QAction(text, menu)
                trash_action.triggered.connect(partial(self.update_is_trashed,
                                                       selected_project,
                                                       project_controller))
                menu.addAction(rename_action)
                menu.addSeparator()
                menu.addAction(save_action)
                menu.addAction(save_as_action)
                menu.addAction(close_action)
                menu.addSeparator()
                menu.addAction(trash_action)

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

    def _project_controller(self, controller):
        """ Find the parent project controller of a given controller
        """
        parent_types = (ProjectController, ProjectSubgroupController,
                        SubprojectController)
        root_controller = self.model().root_controller
        return find_parent_object(controller, root_controller, parent_types)

    def _rename_project(self, project):
        """ Change the ``simple_name`` of the given ``project``
        """
        dialog = NewProjectDialog(model=project, is_rename=True)
        result = dialog.exec()
        if result == QDialog.Accepted:
            project.simple_name = dialog.simple_name

    def _close_project(self, project, project_controller, show_dialog=False):
        """ Close the given `project`
        """
        if show_dialog:
            ask = ('Do you really want to close the project \"<b>{}</b>\"'
                   '?').format(project.simple_name)
            reply = QMessageBox.question(None, 'Close project', ask,
                                         QMessageBox.Yes | QMessageBox.No,
                                         QMessageBox.No)
            if reply == QMessageBox.No:
                return

        if project_controller is not None:
            # Check for modififications before closing
            if show_dialog and not maybe_save_modified_project(project):
                return
            # A subproject
            parent_project_model = project_controller.model
            if project in parent_project_model.subprojects:
                parent_project_model.subprojects.remove(project)
        else:
            # Check for modififications before closing
            model = self.model().root_model
            if show_dialog and not maybe_save_modified_project(model):
                return
            # The master project
            self.model().root_model = None
            broadcast_event(KaraboEventSender.ProjectFilterUpdated,
                            {'status': False})

    def update_is_trashed(self, project, project_controller):
        """ Mark the given `project` as (un-)trashed
        """
        project.is_trashed = not project.is_trashed
        db_conn = get_db_conn()
        db_conn.update_attribute(db_conn.default_domain, 'project',
                                 project.uuid, 'is_trashed',
                                 str(project.is_trashed).lower())
        # We directly save on attribute update!
        save_object(project)
