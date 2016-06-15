from PyQt4.QtGui import QBoxLayout
from traits.api import Callable, Int

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


class BoxSceneAction(BaseSceneAction):
    direction = Int

    def perform(self, scene_view):
        """ Perform the box grouping. """
        selection_model = scene_view.selection_model
        # It does not make sense to create a layout for less than 2 items
        if len(selection_model) < 2:
            return

        model = BoxLayoutModel()
        model.direction = self.direction
        selection_rect = calc_bounding_rect(selection_model)
        model.x, model.y, model.width, model.height = selection_rect
        for obj in selection_model:
            scene_view.remove_model(obj.model)
            model.children.append(obj.model)
        scene_view.add_model(model)
        selection_model.clear_selection()


class BoxVSceneAction(BoxSceneAction):
    direction = QBoxLayout.TopToBottom

    """ Group vertically action"""
    def perform(self, scene_view):
        """ Perform the vertical grouping. """
        super(BoxVSceneAction, self).perform(scene_view)


class BoxHSceneAction(BoxSceneAction):
    direction = QBoxLayout.LeftToRight

    """ Group horizontally action"""
    def perform(self, scene_view):
        """ Perform the horizontal grouping. """
        super(BoxHSceneAction, self).perform(scene_view)


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
