from traits.api import Callable

from karabo_gui.scenemodel.layouts import BoxLayoutModel
from karabo_gui.sceneview.bases import BaseSceneAction
from karabo_gui.sceneview.utils import calc_bounding_rect


class CreateToolAction(BaseSceneAction):
    """ An action which simplifies the creation of actions which only change
    the currently active tool in the scene view.
    """
    # A factory for the tool to be added
    tool_factory = Callable

    def perform(self, scene_view):
        """ Perform the action on a SceneView instance.
        """
        tool = self.tool_factory()
        scene_view.set_tool(tool)


class GroupSceneAction(BaseSceneAction):
    """ Group without layout action"""
    def perform(self, scene_view):
        """ Perform the grouping. """


class BoxVSceneAction(BaseSceneAction):
    """ Group vertically action"""
    def perform(self, scene_view):
        """ Perform the vertical grouping. """
        selection_model = scene_view.selection_model
        if not scene_view.selection_model.has_selection():
            return
        # It does not make sense to create a layout for less than 2 items
        if selection_model.count() < 2:
            return

        model = BoxLayoutModel()
        model.direction = 2 #QBoxLayout.TopToBottom
        model.x, model.y, model.width, model.height = calc_bounding_rect(
                                                        selection_model)
        for obj in selection_model:
            scene_view.remove_model(obj.model)
            model.children.append(obj.model)
        scene_view.add_model(model)


class BoxHSceneAction(BaseSceneAction):
    """ Group horizontally action"""
    def perform(self, scene_view):
        """ Perform the horizontal grouping. """


class GridSceneAction(BaseSceneAction):
    """ Group in grid action"""
    def perform(self, scene_view):
        """ Perform the grouping in grid. """


class UngroupSceneAction(BaseSceneAction):
    """ Ungroup action"""
    def perform(self, scene_view):
        """ Perform the ungrouping. """


class GroupEntireSceneAction(BaseSceneAction):
    """ Group entirely"""
    def perform(self, scene_view):
        """ Perform entire window grouping. """
