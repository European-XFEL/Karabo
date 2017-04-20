#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
from io import StringIO

from PyQt4.QtGui import QAction, QDialog, QMenu
from traits.api import Instance

from karabo.common.scenemodel.api import SceneModel, read_scene, write_scene
from karabo_gui import icons
from karabo_gui.events import broadcast_event, KaraboEventSender
from karabo_gui.project.dialog.object_handle import ObjectDuplicateDialog
from karabo_gui.project.dialog.scene_handle import SceneHandleDialog
from karabo_gui.project.utils import show_no_configuration
from karabo_gui.util import getSaveFileName
from .bases import BaseProjectController, ProjectControllerUiData


class SceneController(BaseProjectController):
    """ A controller for SceneModel objects
    """
    # Redefine model with the correct type
    model = Instance(SceneModel)

    def context_menu(self, project_controller, parent=None):
        menu = QMenu(parent)
        edit_action = QAction('Edit', menu)
        edit_action.triggered.connect(self._edit_scene)
        dupe_action = QAction('Duplicate', menu)
        dupe_action.triggered.connect(partial(self._duplicate_scene,
                                              project_controller))
        delete_action = QAction('Delete', menu)
        delete_action.triggered.connect(partial(self._delete_scene,
                                                project_controller))
        save_as_action = QAction('Save As...', menu)
        save_as_action.triggered.connect(self._save_scene_to_file)
        menu.addAction(edit_action)
        menu.addAction(dupe_action)
        menu.addAction(delete_action)
        menu.addSeparator()
        menu.addAction(save_as_action)
        return menu

    def create_ui_data(self):
        return ProjectControllerUiData(icon=icons.image)

    def single_click(self, project_controller, parent=None):
        show_no_configuration()

    def double_click(self, project_controller, parent=None):
        broadcast_event(KaraboEventSender.OpenSceneView, {'model': self.model})

    # ----------------------------------------------------------------------
    # action handlers

    def _delete_scene(self, project_controller):
        """ Remove the scene associated with this item from its project
        """
        scene = self.model
        project = project_controller.model
        if scene in project.scenes:
            project.scenes.remove(scene)

        broadcast_event(KaraboEventSender.RemoveSceneView, {'model': scene})

    def _edit_scene(self):
        dialog = SceneHandleDialog(self.model)
        result = dialog.exec()
        if result == QDialog.Accepted:
            self.model.simple_name = dialog.simple_name

    def _duplicate_scene(self, project_controller):
        scene = self.model
        project = project_controller.model
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
