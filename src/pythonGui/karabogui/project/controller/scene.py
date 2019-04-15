#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
from io import StringIO
import os.path as op

from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QAction, QDialog, QMenu, QMessageBox
from traits.api import Instance

from karabo.common.project.api import get_user_cache
from karabo.common.scenemodel.api import SceneModel, read_scene, write_scene
from karabo.native import read_project_model
from karabogui import icons, messagebox
from karabogui.enums import ProjectItemTypes
from karabogui.events import broadcast_event, KaraboEvent
from karabogui.project.dialog.object_handle import (
    ObjectDuplicateDialog, ObjectEditDialog)
from karabogui.singletons.api import get_db_conn, get_panel_wrangler
from karabogui.util import getOpenFileName, getSaveFileName
from karabogui.singletons.api import get_config
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
        save_as_action = QAction('Save to file', menu)
        save_as_action.triggered.connect(self._save_scene_to_file)
        replace_action = QAction('Replace from file', menu)
        replace_action.triggered.connect(partial(self._replace_scene,
                                                 project_controller))
        revert_action = QAction('Revert changes', menu)
        revert_action.triggered.connect(self._revert_changes)
        can_revert = not self._is_showing() and self.model.modified
        revert_action.setEnabled(can_revert)
        menu.addAction(edit_action)
        menu.addAction(dupe_action)
        menu.addAction(delete_action)
        menu.addSeparator()
        menu.addAction(save_as_action)
        menu.addAction(replace_action)
        menu.addAction(revert_action)

        return menu

    def info(self):
        return {'type': ProjectItemTypes.SCENE,
                'uuid': self.model.uuid,
                'simple_name': self.model.simple_name}

    def create_ui_data(self):
        return ProjectControllerUiData(icon=icons.image)

    def double_click(self, project_controller, parent=None):
        broadcast_event(KaraboEvent.ShowSceneView, {'model': self.model})

    # ----------------------------------------------------------------------
    # action handlers

    def _replace_scene(self, project_controller):
        """ Replace a scene from local disk
        """
        # Make sure scene is closed before!
        scene = self.model
        path = get_config()['scene_dir']
        directory = path if path and op.isdir(path) else ""

        fn = getOpenFileName(caption='Replace scene',
                             filter='SVG Files (*.svg)',
                             directory=directory)
        if not fn:
            return

        # Store scene dir path
        get_config()['scene_dir'] = op.dirname(fn)

        project = project_controller.model
        if scene in project.scenes:
            # Read SceneModel for information to replace with! We catch all
            # errors because we can have wrong user input.
            try:
                new_scene = read_scene(fn)
            except Exception:
                messagebox.show_error("Scene file could not be read!")
                return

            # NOTE: We can successfully read the scene, now close the old one
            broadcast_event(KaraboEvent.RemoveProjectModelViews,
                            {'models': [scene]})
            # Only use copyable traits
            scene.copy_traits(new_scene, traits=['width', 'height',
                                                 'children', 'description'])
            scene.modified = True

    def _delete_scene(self, project_controller):
        """ Remove the scene associated with this item from its project
        """
        scene = self.model
        ask = ('Are you sure you want to delete \"<b>{}</b>\".<br /> '
               'Continue action?'.format(scene.simple_name))
        msg_box = QMessageBox(QMessageBox.Question, 'Delete scene',
                              ask, QMessageBox.Yes | QMessageBox.No)
        msg_box.setModal(False)
        msg_box.setDefaultButton(QMessageBox.No)
        if msg_box.exec() == QMessageBox.Yes:
            project = project_controller.model
            if scene in project.scenes:
                project.scenes.remove(scene)

            broadcast_event(KaraboEvent.RemoveProjectModelViews,
                            {'models': [scene]})

    @pyqtSlot()
    def _edit_scene(self):
        dialog = ObjectEditDialog(object_type='scene', model=self.model)
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
                dupe_scene.reset_uuid()
                project.scenes.append(dupe_scene)

    def _is_showing(self):
        scene = self.model
        wrangler = get_panel_wrangler()
        return wrangler.is_showing_project_item(scene)

    @pyqtSlot()
    def _revert_changes(self):
        scene = self.model
        # NOTE: The domain only changes when a project is saved or loaded
        domain = get_db_conn().default_domain
        data = get_user_cache().retrieve(domain, scene.uuid)
        if data is None:
            msg = 'That scene has been removed from the local cache!'
            messagebox.show_error(msg, title='Revert failed')
            return

        read_project_model(StringIO(data), existing=scene)
        scene.modified = False

    @pyqtSlot()
    def _save_scene_to_file(self):
        config = get_config()
        path = config['scene_dir']
        directory = path if path and op.isdir(path) else ""
        scene = self.model
        filename = scene.simple_name
        filename = filename.translate(str.maketrans("/|: ", "----"))
        fn = getSaveFileName(caption='Save scene to file',
                             filter='SVG (*.svg)',
                             suffix='svg',
                             selectFile=filename,
                             directory=directory)
        if not fn:
            return

        # Store old scene dialog path
        config['scene_dir'] = op.dirname(fn)

        if not fn.endswith('.svg'):
            fn = '{}.svg'.format(fn)

        with open(fn, 'w') as fout:
            fout.write(write_scene(scene))
