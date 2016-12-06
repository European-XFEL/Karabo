#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import weakref

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QAction, QMenu, QStandardItem
from traits.api import Instance, List, on_trait_change

from karabo.common.project.api import ProjectModel
from karabo_gui import icons
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
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
            menu.addAction(edit_action)

        return menu

    def create_qt_item(self):
        item = QStandardItem(self.model.simple_name)
        item.setData(weakref.ref(self), PROJECT_ITEM_MODEL_REF)
        item.setIcon(icons.folder)
        item.setEditable(False)
        for child in self.children:
            item.appendRow(child.qt_item)

        font = item.font()
        font.setBold(True)
        item.setFont(font)
        return item

    @on_trait_change("model.modified")
    def modified_change(self):
        """ Whenever the project is modified it should be visible"""
        if not self.is_ui_initialized():
            return

        # Show modified flag on parent projects
        brush = self.qt_item.foreground()
        if self.model.modified:
            # Change color to blue
            brush.setColor(Qt.blue)
            self.qt_item.setText("*{}".format(self.model.simple_name))
        else:
            # Change color to black
            brush.setColor(Qt.black)
            self.qt_item.setText("{}".format(self.model.simple_name))
        self.qt_item.setForeground(brush)

    # ----------------------------------------------------------------------
    # action handlers
