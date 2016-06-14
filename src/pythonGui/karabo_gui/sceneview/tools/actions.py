from traits.api import Callable

from karabo_gui.sceneview.bases import BaseSceneAction


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
        print("GroupSceneAction")


class BoxVSceneAction(BaseSceneAction):
    """ Group vertically action"""
    def perform(self, scene_view):
        """ Perform the vertical grouping. """
        print("BoxVSceneAction")


class BoxHSceneAction(BaseSceneAction):
    """ Group horizontally action"""
    def perform(self, scene_view):
        """ Perform the horizontal grouping. """
        print("BoxHSceneAction")


class GridSceneAction(BaseSceneAction):
    """ Group in grid action"""
    def perform(self, scene_view):
        """ Perform the grouping in grid. """
        print("GridSceneAction")


class UngroupSceneAction(BaseSceneAction):
    """ Ungroup action"""
    def perform(self, scene_view):
        """ Perform the ungrouping. """
        print("UngroupSceneAction")


class GroupEntireSceneAction(BaseSceneAction):
    """ Group entirely"""
    def perform(self, scene_view):
        """ Perform entire window grouping. """
        print("GroupEntireSceneAction")
