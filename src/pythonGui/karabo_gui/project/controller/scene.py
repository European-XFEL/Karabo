#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
from io import StringIO
import weakref

from PyQt4.QtGui import QAction, QDialog, QMenu, QStandardItem
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import SceneModel, read_scene, write_scene
from karabo_gui import icons
from karabo_gui.const import PROJECT_CONTROLLER_REF
from karabo_gui.events import (broadcast_event, KaraboBroadcastEvent,
                               KaraboEventSender)
from karabo_gui.project.dialog.object_handle import ObjectDuplicateDialog
from karabo_gui.project.dialog.scene_handle import SceneHandleDialog
from karabo_gui.project.utils import save_object
from karabo_gui.util import getSaveFileName
from .bases import BaseProjectController


class SceneController(BaseProjectController):
    """ A controller for SceneModel objects
    """
    # Redefine model with the correct type
    model = Instance(SceneModel)

    def context_menu(self, parent_project, parent=None):
        menu = QMenu(parent)
        edit_action = QAction('Edit', menu)
        edit_action.triggered.connect(self._edit_scene)
        dupe_action = QAction('Duplicate', menu)
        dupe_action.triggered.connect(partial(self._duplicate_scene,
                                              parent_project))
        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_scene,
                                                parent_project))
        save_action = QAction('Save', menu)
        save_action.triggered.connect(partial(save_object, self.model))
        save_as_action = QAction('Save As...', menu)
        save_as_action.triggered.connect(self._save_scene_to_file)
        menu.addAction(edit_action)
        menu.addAction(dupe_action)
        menu.addAction(delete_action)
        menu.addSeparator()
        menu.addAction(save_action)
        menu.addAction(save_as_action)
        return menu

    def create_qt_item(self):
        item = QStandardItem(self.model.simple_name)
        item.setData(weakref.ref(self), PROJECT_CONTROLLER_REF)
        item.setIcon(icons.image)
        item.setEditable(False)
        self.set_qt_item_text(item, self.model.simple_name)
        return item

    def double_click(self, parent_project, parent=None):
        data = {'model': self.model}
        broadcast_event(KaraboBroadcastEvent(KaraboEventSender.OpenSceneView,
                                             data))

    @on_trait_change("model.modified, model.simple_name")
    def modified_change(self):
        """ Whenever the project is modified it should be visible.

        The scene name is modified in the project panel.
        """
        if not self.is_ui_initialized():
            return
        self.set_qt_item_text(self.qt_item, self.model.simple_name)

    @on_trait_change("model.simple_name")
    def on_model_name_change(self):
        """ New scene name should appear in the middle panel """
        if not self.is_ui_initialized():
            return
        data = {'model': self.model}
        broadcast_event(KaraboBroadcastEvent(KaraboEventSender.RenameSceneView,
                                             data))

    # ----------------------------------------------------------------------
    # action handlers

    def _delete_scene(self, project):
        """ Remove the scene associated with this item from its project
        """
        scene = self.model
        if scene in project.scenes:
            project.scenes.remove(scene)

        broadcast_event(KaraboBroadcastEvent(
            KaraboEventSender.RemoveSceneView, {'model': scene}))

    def _edit_scene(self):
        dialog = SceneHandleDialog(self.model)
        result = dialog.exec()
        if result == QDialog.Accepted:
            self.model.simple_name = dialog.simple_name

    def _duplicate_scene(self, project):
        scene = self.model
        dialog = ObjectDuplicateDialog(scene.simple_name)
        if dialog.exec() == QDialog.Accepted:
            xml = write_scene(scene)
            for simple_name in dialog.duplicate_names:
                dupe_scene = read_scene(StringIO(xml))
                dupe_scene.simple_name = simple_name
                project.scenes.append(dupe_scene)

    def _save_scene_to_file(self):
        scene = self.model
        fn = getSaveFileName(caption='Save scene to file',
                             filter='SVG (*.svg)',
                             suffix='svg',
                             selectFile=scene.simple_name)
        if not fn:
            return

        if not fn.endswith('.svg'):
            fn = '{}.svg'.format(fn)

        with open(fn, 'w') as fout:
            fout.write(write_scene(scene))
