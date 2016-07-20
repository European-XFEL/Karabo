from abc import abstractmethod
from io import StringIO

from PyQt4.QtCore import QByteArray, QMimeData
from PyQt4.QtGui import QApplication, QDialog, QMessageBox

from karabo_gui.scenemodel.api import (BaseLayoutModel, BaseWidgetObjectData,
                                       SceneModel, read_scene, write_scene)
from karabo_gui.sceneview.bases import BaseSceneAction
from karabo_gui.dialogs.dialogs import ReplaceDialog

MIME_TYPE = 'image/svg+xml'


def _add_models_to_clipboard(models, rect):
    """ Given a list of models, add their SVG representation to the clipboard
    """
    scene = SceneModel(children=models, width=rect.width(),
                       height=rect.height())
    xml = write_scene(scene)
    mime_data = QMimeData()
    mime_data.setData(MIME_TYPE, QByteArray(xml))
    QApplication.clipboard().setMimeData(mime_data)


def _read_models_from_clipboard():
    """ Read SVG data from the clipboard into a SceneModel and return the child
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
    """ Copy all selected objects from the scene view
    """
    def perform(self, scene_view):
        selection_model = scene_view.selection_model
        if len(selection_model) == 0:
            return

        models = [o.model for o in selection_model]
        rect = selection_model.get_selection_bounds()
        _add_models_to_clipboard(models, rect)


class SceneCutAction(BaseSceneAction):
    """ Remove all selected objects from the scene view.
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
    """ Base class for paste actions.

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
        """ Implemented by derived classes to implement what should happen with
            the copied ``models``. """


class ScenePasteAction(BaseScenePasteAction):
    """ Paste the contents of the clipboard to the scene view.
    """
    def run_action(self, models, scene_view):
        """ Add ``models`` to scene view."""
        scene_view.add_models(*models)


class ScenePasteReplaceAction(BaseScenePasteAction):
    """ Paste and replace the contents of the clipboard to the scene view.
    """
    def _get_widget_model_keys(self, model, keys_list):
        """ This recursive method fills the given ``key_list`` with model keys
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
                    new_key = "{}.{}".format(new_device_id, property_key)
                new_key_list.append(new_key)
            # Overwrite key list with new keys
            model.keys = new_key_list

    def run_action(self, models, scene_view):
        """ Modify the device IDs of ``models``and add them to the scene view.
        """

        keys = []
        for m in models:
            self._get_widget_model_keys(m, keys)

        device_ids = sorted(set(k.split('.', 1)[0] for k in keys))
        dialog = ReplaceDialog(device_ids)
        if dialog.exec_() != QDialog.Accepted:
            return

        mapped_device_ids = dialog.mappedDevices()
        for m in models:
            self._set_widget_model_keys(m, mapped_device_ids)
        scene_view.add_models(*models)


class SceneSelectAllAction(BaseSceneAction):
    """ Select all objects in the scene view.
    """
    def perform(self, scene_view):
        scene_view.select_all()


class SceneDeleteAction(BaseSceneAction):
    """ Delete selected objects from the scene view.
    """
    def perform(self, scene_view):
        selection_model = scene_view.selection_model
        if len(selection_model) == 0:
            return

        result = QMessageBox.question(None, "Really delete?", "Do you really"
                                      "want to delete the items?",
                                      QMessageBox.Yes | QMessageBox.No)
        if result == QMessageBox.No:
            return

        for o in selection_model:
            scene_view.remove_model(o.model)
        selection_model.clear_selection()
