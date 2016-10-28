#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import weakref

from PyQt4.QtGui import QAction, QMenu, QStandardItem
from traits.api import Instance, List

from karabo.common.project.api import ProjectModel
from karabo_gui import icons
from .base import BaseProjectTreeItem
from .project_groups import ProjectSubgroupItem


class ProjectModelItem(BaseProjectTreeItem):
    """ A wrapper for ProjectModel objects
    """
    # Redefine model with the correct type
    model = Instance(ProjectModel)
    # The subgroups of this project
    children = List(Instance(ProjectSubgroupItem))

    def context_menu(self, parent):
        menu = QMenu(parent)
        close_action = QAction('Close project', menu)
        menu.addAction(close_action)
        return menu

    def _get_qt_item(self):
        item = QStandardItem('Project: {}'.format(self.model.simple_name))
        item.setData(weakref.ref(self), self.MODEL_REF_ITEM_ROLE)
        item.setIcon(icons.folder)
        item.setEditable(False)
        for child in self.children:
            item.appendRow(child.qt_item)

        font = item.font()
        font.setBold(True)
        item.setFont(font)
        return item
