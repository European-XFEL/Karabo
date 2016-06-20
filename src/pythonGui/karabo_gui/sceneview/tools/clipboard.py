from io import StringIO

from PyQt4.QtCore import QByteArray, QMimeData
from PyQt4.QtGui import QApplication, QMessageBox

from karabo_gui.scenemodel.api import SceneModel, read_scene, write_scene
from karabo_gui.sceneview.bases import BaseSceneAction

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


class ScenePasteAction(BaseSceneAction):
    """ Paste the contents of the clipboard to the scene view.
    """
    def perform(self, scene_view):
        pasted_models = _read_models_from_clipboard()
        if len(pasted_models) == 0:
            return

        scene_view.add_models(*pasted_models)
        scene_view.update()


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

        if (QMessageBox.question(None, "Really delete?", "Do you really want"
                                 "to delete the items?", QMessageBox.Yes |
                                 QMessageBox.No) == QMessageBox.No):
            return

        for o in selection_model:
            scene_view.remove_model(o.model)
        selection_model.clear_selection()
