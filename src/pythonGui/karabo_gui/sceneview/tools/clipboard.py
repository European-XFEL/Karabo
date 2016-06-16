from karabo_gui.sceneview.bases import BaseSceneAction


class SceneCopyAction(BaseSceneAction):
    def perform(self, scene_view):
        pass


class SceneCutAction(BaseSceneAction):
    def perform(self, scene_view):
        pass


class ScenePasteAction(BaseSceneAction):
    def perform(self, scene_view):
        pass


class SceneSelectAllAction(BaseSceneAction):
    """ Select all objects in the scene view.
    """
    def perform(self, scene_view):
        scene_view.select_all()
