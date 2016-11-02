#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
import weakref

from PyQt4.QtGui import QAction, QDialog, QMenu, QStandardItem
from traits.api import Instance

from karabo.common.scenemodel.api import SceneModel
from karabo_gui import icons
from karabo_gui.const import PROJECT_ITEM_MODEL_REF
from karabo_gui.project.dialog.scene_handle import SceneHandleDialog
from .bases import BaseProjectTreeItem


class SceneModelItem(BaseProjectTreeItem):
    """ A wrapper for SceneModel objects
    """
    # Redefine model with the correct type
    model = Instance(SceneModel)

    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)
        edit_action = QAction('Edit', menu)
        edit_action.triggered.connect(self._edit_scene)
        dupe_action = QAction('Duplicate', menu)
        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_scene,
                                                parent_project))
        save_as_action = QAction('Save As...', menu)
        menu.addAction(edit_action)
        menu.addAction(dupe_action)
        menu.addAction(delete_action)
        menu.addSeparator()
        menu.addAction(save_as_action)
        return menu

    def create_qt_item(self):
        item = QStandardItem(self.model.simple_name)
        item.setData(weakref.ref(self), PROJECT_ITEM_MODEL_REF)
        item.setIcon(icons.image)
        item.setEditable(False)
        return item

    # ----------------------------------------------------------------------
    # action handlers

    def _delete_scene(self, project):
        """ Remove the scene associated with this item from its project
        """
        scene = self.model
        if scene in project.scenes:
            project.scenes.remove(scene)

    def _edit_scene(self):
        dialog = SceneHandleDialog(self.model)
        result = dialog.exec()
        if result == QDialog.Accepted:
            self.model.simple_name = dialog.simple_name()
