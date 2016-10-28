#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import weakref

from PyQt4.QtGui import QAction, QMenu, QStandardItem
from traits.api import Instance

from karabo.common.scenemodel.api import SceneModel
from karabo_gui import icons
from .base import BaseProjectTreeItem


class SceneModelItem(BaseProjectTreeItem):
    """ A wrapper for SceneModel objects
    """
    # Redefine model with the correct type
    model = Instance(SceneModel)

    def context_menu(self, parent):
        menu = QMenu(parent)
        edit_action = QAction('Edit', menu)
        dupe_action = QAction('Duplicate', menu)
        delete_action = QAction('Delete', menu)
        save_as_action = QAction('Save As...', menu)
        menu.addAction(edit_action)
        menu.addAction(dupe_action)
        menu.addAction(delete_action)
        menu.addSeparator()
        menu.addAction(save_as_action)
        return menu

    def _get_qt_item(self):
        item = QStandardItem(self.model.title)
        item.setData(weakref.ref(self), self.MODEL_REF_ITEM_ROLE)
        item.setIcon(icons.image)
        return item
