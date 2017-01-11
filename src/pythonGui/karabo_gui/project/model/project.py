#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import weakref

from PyQt4.QtGui import QAction, QDialog, QMenu, QStandardItem
from traits.api import Instance, List, on_trait_change

from karabo.common.project.api import ProjectModel
from karabo_gui import icons
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
from karabo_gui.project.dialog.project_handle import NewProjectDialog
from .bases import BaseProjectTreeItem
from .project_groups import ProjectSubgroupItem


class ProjectModelItem(BaseProjectTreeItem):
    """ A wrapper for ProjectModel objects
    """
    # Redefine model with the correct type
    model = Instance(ProjectModel)
    # The subgroups of this project
    children = List(Instance(ProjectSubgroupItem))

    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)

        # If this project is the top of the hierarchy, its parent must be None
        # In that case, do NOT add a 'Delete' action
        if parent_project is not None:
            edit_action = QAction('Edit', menu)
            edit_action.triggered.connect(self._edit_project)
            menu.addAction(edit_action)

        return menu

    def create_qt_item(self):
        item = QStandardItem()
        item.setData(weakref.ref(self), PROJECT_ITEM_MODEL_REF)
        item.setIcon(icons.folder)
        item.setEditable(False)
        for child in self.children:
            item.appendRow(child.qt_item)

        font = item.font()
        font.setBold(True)
        item.setFont(font)
        self.set_qt_item_text(item, self.model.simple_name)
        return item

    @on_trait_change("model.modified,model.simple_name")
    def update_ui_label(self):
        """ Whenever the project is modified it should be visible to the user
        """
        if not self.is_ui_initialized():
            return
        self.set_qt_item_text(self._qt_item, self.model.simple_name)

    # ----------------------------------------------------------------------
    # action handlers

    def _edit_project(self):
        dialog = NewProjectDialog(self.model)
        result = dialog.exec()
        if result == QDialog.Accepted:
            self.model.simple_name = dialog.simple_name
