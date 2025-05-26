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
from abc import abstractmethod
from io import StringIO
from operator import sub

from qtpy.QtCore import QMimeData, QPoint
from qtpy.QtGui import QCursor
from qtpy.QtWidgets import QApplication, QDialog, QMessageBox

from karabo.common.scenemodel.api import (
    BaseLayoutModel, BaseWidgetObjectData, SceneModel, read_scene, write_scene)
from karabogui.dialogs.api import ReplaceDialog
from karabogui.sceneview.bases import BaseSceneAction
from karabogui.sceneview.utils import add_offset, calc_relative_pos
from karabogui.util import move_to_cursor

MIME_TYPE = 'image/svg+xml'
GRID_INC = 10


def _add_models_to_clipboard(models, rect):
    """Given a list of models, add their SVG representation to the clipboard
    """
    scene = SceneModel(children=models, width=rect.width(),
                       height=rect.height())
    xml = write_scene(scene)
    mime_data = QMimeData()
    mime_data.setData(MIME_TYPE, xml.encode('utf-8'))
    QApplication.clipboard().setMimeData(mime_data)


def _read_models_from_clipboard():
    """Read SVG data from the clipboard into a SceneModel and return the child
    models.
    """
    mime_data = QApplication.clipboard().mimeData()
    if not mime_data.hasFormat(MIME_TYPE):
        return []

    byte_array = mime_data.data(MIME_TYPE)
    file_obj = StringIO(byte_array.data().decode('utf-8'))
    scene = read_scene(file_obj)

    return scene.children


class SceneCopyAction(BaseSceneAction):
    """Copy all selected objects from the scene view
    """
    def perform(self, scene_view):
        selection_model = scene_view.selection_model
        if len(selection_model) == 0:
            return

        models = [o.model for o in selection_model]
        rect = selection_model.get_selection_bounds()
        _add_models_to_clipboard(models, rect)


class SceneCutAction(BaseSceneAction):
    """Remove all selected objects from the scene view.
    """
    def perform(self, scene_view):
        selection_model = scene_view.selection_model
        if len(selection_model) == 0:
            return

        models = [o.model for o in selection_model]
        rect = selection_model.get_selection_bounds()
        selection_model.clear_selection()

        # Make sure they can be pasted
        _add_models_to_clipboard(models, rect)

        for model in models:
            scene_view.remove_model(model)
        scene_view.update()


class BaseScenePasteAction(BaseSceneAction):
    """Base class for paste actions.

    This contains the boilerplate that all layout actions have in common.
    """
    def perform(self, scene_view):
        pasted_models = _read_models_from_clipboard()
        if len(pasted_models) == 0:
            return

        self.run_action(pasted_models, scene_view)
        scene_view.update()

    @abstractmethod
    def run_action(self, models, scene_view):
        """Implemented by derived classes to implement what should happen with
        the copied ``models``.
        """

    def add_to_scene(self, models, scene_view):
        # Map cursor to widget
        pos = scene_view.mapFromGlobal(QCursor.pos())
        rect = scene_view.visibleRegion().boundingRect()
        if rect.contains(pos):
            new_origin = (int(pos.x()), int(pos.y()))
        else:
            # Get the relative
            new_origin = (rect.x() + 10, rect.y() + 10)
        old_origin = calc_relative_pos(models)
        x_off, y_off = map(sub, new_origin, old_origin)

        # Add offset on models
        for model in models:
            add_offset(model, x=x_off, y=y_off)

        scene_view.add_models(*models)
        scene_view.select_models(*models)


class ScenePasteAction(BaseScenePasteAction):
    """Paste the contents of the clipboard to the scene view.
    """
    def run_action(self, models, scene_view):
        """Add ``models`` to scene view."""
        # Add some offset on the pasted model to help users see that there are
        # new objects from the command
        if not len(models):
            # Do nothing
            return

        self.add_to_scene(models, scene_view)


class ScenePasteReplaceAction(BaseScenePasteAction):
    """Paste and replace the contents of the clipboard to the scene view.
    """
    def _get_widget_model_keys(self, model, keys_list):
        """This recursive method fills the given ``key_list`` with model keys
        of ``BaseWidgetObjectData`` types.
        """
        if isinstance(model, BaseLayoutModel):
            for child in model.children:
                self._get_widget_model_keys(child, keys_list)
        elif isinstance(model, BaseWidgetObjectData):
            keys_list.extend(model.keys)

    def _set_widget_model_keys(self, model, mapped_device_ids):
        if isinstance(model, BaseLayoutModel):
            for child in model.children:
                self._set_widget_model_keys(child, mapped_device_ids)
        elif isinstance(model, BaseWidgetObjectData):
            new_key_list = []
            for key in model.keys:
                device_id, property_key = key.split('.', 1)
                new_device_id = mapped_device_ids.get(device_id)
                if new_device_id is None:
                    new_key = key
                else:
                    new_key = f"{new_device_id}.{property_key}"
                new_key_list.append(new_key)
            # Overwrite key list with new keys
            model.keys = new_key_list

    def run_action(self, models, scene_view):
        """Modify the device IDs of ``models``and add them to the scene view.
        """
        if not len(models):
            # Do nothing
            return

        keys = []
        for m in models:
            self._get_widget_model_keys(m, keys)

        device_ids = sorted({k.split('.', 1)[0] for k in keys})
        dialog = ReplaceDialog(device_ids, parent=scene_view)
        if dialog.exec() != QDialog.Accepted:
            return

        mapped_device_ids = dialog.mappedDevices()
        for m in models:
            self._set_widget_model_keys(m, mapped_device_ids)
        self.add_to_scene(models, scene_view)


class SceneSelectAllAction(BaseSceneAction):
    """Select all objects in the scene view.
    """
    def perform(self, scene_view):
        scene_view.select_all()


class SceneDeleteAction(BaseSceneAction):
    """Delete selected objects from the scene view.
    """
    def perform(self, scene_view):
        selection_model = scene_view.selection_model
        if len(selection_model) == 0:
            return

        dialog = QMessageBox(
            QMessageBox.Question, "Really delete?", "Do you really "
            "want to delete the items?", QMessageBox.Yes | QMessageBox.No,
            parent=scene_view)
        move_to_cursor(dialog)

        if dialog.exec() == QMessageBox.No:
            return

        for o in selection_model:
            scene_view.remove_model(o.model)
        selection_model.clear_selection()


class SceneMoveAction(BaseSceneAction):
    """Move selected objects inside the scene view.
    """
    def perform(self, scene_view):
        selection_model = scene_view.selection_model
        if len(selection_model) == 0:
            return

        inc = GRID_INC if scene_view.snap_to_grid else 1
        increment = {"Left": QPoint(-inc, 0),
                     "Right": QPoint(inc, 0),
                     "Up": QPoint(0, -inc),
                     "Down": QPoint(0, inc)}
        offset = increment.get(self.text)
        # move each item in the selection with increment!
        for c in scene_view.selection_model:
            c.translate(offset)
        scene_view.update()


class SceneAlignAction(BaseSceneAction):
    """Align selected objects inside the scene view.
    """
    def perform(self, scene_view):
        selection_model = scene_view.selection_model
        if len(selection_model) == 0:
            return

        align_coord = {
            "Left": min([c.model.x for c in selection_model]),
            "Right": max(
                [c.model.x + c.model.width for c in selection_model]),
            "Top": min([c.model.y for c in selection_model]),
            "Bottom": max(
                [c.model.y + c.model.height for c in selection_model])

        }
        for c in selection_model:
            if self.text == "Left":
                offset = QPoint(align_coord["Left"] - c.model.x, 0)
            elif self.text == "Right":
                offset = QPoint(
                    align_coord["Right"] - c.model.x - c.model.width, 0)
            elif self.text == "Top":
                offset = QPoint(0, align_coord["Top"] - c.model.y)
            elif self.text == "Bottom":
                offset = QPoint(
                    0, align_coord["Bottom"] - c.model.y - c.model.height)
            c.translate(offset)

        scene_view.update()
