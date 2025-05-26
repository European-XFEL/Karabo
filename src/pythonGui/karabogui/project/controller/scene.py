#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
import os.path as op
import re
from functools import partial
from io import StringIO

from qtpy.QtWidgets import QAction, QDialog, QMenu, QMessageBox
from traits.api import Instance

from karabo.common.api import BaseSavableModel, walk_traits_object
from karabo.common.project.api import get_user_cache
from karabo.common.scenemodel.api import SceneModel, read_scene, write_scene
from karabo.native import read_project_model
from karabogui import icons, messagebox
from karabogui.access import AccessRole, access_role_allowed
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.itemtypes import ProjectItemTypes
from karabogui.project.dialog.object_handle import (
    ObjectDuplicateDialog, ObjectEditDialog)
from karabogui.singletons.api import (
    get_config, get_db_conn, get_panel_wrangler)
from karabogui.util import getOpenFileName, getSaveFileName, move_to_cursor
from karabogui.wizards.api import CinemaWizardController

from .bases import BaseProjectController, ProjectControllerUiData


class SceneController(BaseProjectController):
    """ A controller for SceneModel objects
    """
    # Redefine model with the correct type
    model = Instance(SceneModel)

    def context_menu(self, project_controller, parent=None):
        project_allowed = access_role_allowed(AccessRole.PROJECT_EDIT)

        menu = QMenu(parent)
        edit_action = QAction(icons.edit, 'Edit', menu)
        edit_action.triggered.connect(partial(self._edit_scene, parent=parent))
        edit_action.setEnabled(project_allowed)

        dupe_action = QAction(icons.editCopy, 'Duplicate', menu)
        dupe_action.triggered.connect(partial(self._duplicate_scene,
                                              project_controller,
                                              parent=parent))
        dupe_action.setEnabled(project_allowed)

        delete_action = QAction(icons.delete, 'Delete', menu)
        delete_action.triggered.connect(partial(self._delete_scene,
                                                project_controller,
                                                parent=parent))
        delete_action.setEnabled(project_allowed)

        save_as_action = QAction(icons.saveAs, 'Save to file', menu)
        save_as_action.triggered.connect(partial(self._save_scene_to_file,
                                                 parent=parent))
        replace_action = QAction(icons.editPasteReplace,
                                 'Replace from file', menu)
        replace_action.triggered.connect(partial(self._replace_scene,
                                                 project_controller,
                                                 parent=parent))
        replace_action.setEnabled(project_allowed)

        revert_action = QAction(icons.revert, 'Revert changes', menu)
        revert_action.triggered.connect(partial(self._revert_changes,
                                                parent=parent))
        can_revert = not self._is_showing() and self.model.modified
        revert_action.setEnabled(can_revert and project_allowed)

        project_model = project_controller.model
        cinema_action = QAction(icons.run, 'Create cinema link', menu)
        cinema_action.triggered.connect(partial(self._create_cinema_link,
                                                project_model=project_model,
                                                parent=parent))

        menu.addAction(edit_action)
        menu.addAction(dupe_action)
        menu.addAction(delete_action)
        menu.addSeparator()
        menu.addAction(save_as_action)
        menu.addAction(replace_action)
        menu.addAction(revert_action)
        menu.addSeparator()
        menu.addAction(cinema_action)

        return menu

    def info(self):
        return {'type': ProjectItemTypes.SCENE,
                'uuid': self.model.uuid,
                'simple_name': self.model.simple_name}

    def create_ui_data(self):
        return ProjectControllerUiData(icon=icons.image)

    def double_click(self, project_controller, parent=None):
        broadcast_event(KaraboEvent.ShowSceneView, {'model': self.model})

    def delete_press(self, project_controller, parent=None):
        """Reimplemented function on `BaseProjectController`"""
        self._delete_scene(project_controller, parent)

    # ----------------------------------------------------------------------
    # action handlers

    def _replace_scene(self, project_controller, parent=None):
        """ Replace a scene from local disk
        """
        # Make sure scene is closed before!
        scene = self.model
        path = get_config()['data_dir']
        directory = path if path and op.isdir(path) else ""

        fn = getOpenFileName(caption='Replace scene',
                             filter='SVG Files (*.svg)',
                             directory=directory,
                             parent=parent)
        if not fn:
            return

        # Store scene dir path
        get_config()['data_dir'] = op.dirname(fn)

        project = project_controller.model
        # NOTE: Close the old one first!
        broadcast_event(KaraboEvent.RemoveProjectModelViews,
                        {'models': [scene]})

        if scene in project.scenes:
            # Read SceneModel for information to replace with! We catch all
            # errors because we can have wrong user input.
            try:
                new_scene = read_scene(fn)
            except Exception:
                messagebox.show_error("Scene file could not be read!",
                                      parent=parent)
                return

            # Only use copyable traits
            scene.copy_traits(new_scene, traits=['width', 'height',
                                                 'children'])

            def visitor(model):
                if isinstance(model, BaseSavableModel):
                    model.initialized = True

            # Mark everything as initialized, otherwise `modified` won't work.
            walk_traits_object(scene, visitor)

            scene.modified = True

    def _delete_scene(self, project_controller, parent=None):
        """ Remove the scene associated with this item from its project
        """
        scene = self.model
        ask = ('Are you sure you want to delete \"<b>{}</b>\".<br /> '
               'Continue action?'.format(scene.simple_name))
        msg_box = QMessageBox(QMessageBox.Question, 'Delete scene',
                              ask, QMessageBox.Yes | QMessageBox.No,
                              parent=parent)
        msg_box.setModal(False)
        msg_box.setDefaultButton(QMessageBox.No)
        move_to_cursor(msg_box)
        if msg_box.exec() == QMessageBox.Yes:
            project = project_controller.model
            if scene in project.scenes:
                project.scenes.remove(scene)

            broadcast_event(KaraboEvent.RemoveProjectModelViews,
                            {'models': [scene]})

    def _edit_scene(self, parent=None):
        dialog = ObjectEditDialog(object_type='scene', model=self.model,
                                  parent=parent)
        move_to_cursor(dialog)
        result = dialog.exec()
        if result == QDialog.Accepted:
            self.model.simple_name = dialog.simple_name

    def _duplicate_scene(self, project_controller, parent=None):
        scene = self.model
        project = project_controller.model
        dialog = ObjectDuplicateDialog(scene.simple_name, parent=parent)
        move_to_cursor(dialog)
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

    def _revert_changes(self, parent=None):
        scene = self.model
        # NOTE: The domain only changes when a project is saved or loaded
        domain = get_db_conn().default_domain
        data = get_user_cache().retrieve(domain, scene.uuid)
        if data is None:
            msg = 'That scene has been removed from the local cache!'
            messagebox.show_error(msg, title='Revert failed', parent=parent)
            return

        read_project_model(StringIO(data), existing=scene)
        scene.modified = False

    def _save_scene_to_file(self, parent=None):
        config = get_config()
        path = config['data_dir']
        directory = path if path and op.isdir(path) else ""
        scene = self.model
        filename = scene.simple_name
        filename = re.sub(r'[\W]', '-', filename)
        fn = getSaveFileName(caption='Save scene to file',
                             filter='SVG (*.svg)',
                             suffix='svg',
                             selectFile=filename,
                             directory=directory,
                             parent=parent)
        if not fn:
            return

        # Store old scene dialog path
        config['data_dir'] = op.dirname(fn)

        if not fn.endswith('.svg'):
            fn = f'{fn}.svg'

        with open(fn, 'w') as fout:
            fout.write(write_scene(scene))

    def _create_cinema_link(self, project_model=None, parent=None):
        wizard = CinemaWizardController(selected_scenes=[self.model],
                                        project_model=project_model,
                                        parent=parent)
        wizard.run()
