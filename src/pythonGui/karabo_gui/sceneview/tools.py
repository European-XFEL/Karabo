from traits.api import Callable

from .bases import BaseSceneAction, BaseSceneTool


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


class TextSceneTool(BaseSceneTool):
    def draw(self, painter):
        """ The method which is responsible for drawing this tool.
        The tool for a SceneView will be drawn after everything else in
        the view has been drawn.

        This method is optional.
        """

    def mouse_down(self, scene_view, event):
        """ A callback which is fired whenever the user clicks in the
        SceneView.
        """

    def mouse_move(self, scene_view, event):
        """ A callback which is fired whenever the user moves the mouse
        in the SceneView.
        """

    def mouse_up(self, scene_view, event):
        """ A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """


class LineSceneTool(BaseSceneTool):
    def draw(self, painter):
        """ The method which is responsible for drawing this tool.
        The tool for a SceneView will be drawn after everything else in
        the view has been drawn.

        This method is optional.
        """

    def mouse_down(self, scene_view, event):
        """ A callback which is fired whenever the user clicks in the
        SceneView.
        """

    def mouse_move(self, scene_view, event):
        """ A callback which is fired whenever the user moves the mouse
        in the SceneView.
        """

    def mouse_up(self, scene_view, event):
        """ A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """


class RectangleSceneTool(BaseSceneTool):
    def draw(self, painter):
        """ The method which is responsible for drawing this tool.
        The tool for a SceneView will be drawn after everything else in
        the view has been drawn.

        This method is optional.
        """

    def mouse_down(self, scene_view, event):
        """ A callback which is fired whenever the user clicks in the
        SceneView.
        """

    def mouse_move(self, scene_view, event):
        """ A callback which is fired whenever the user moves the mouse
        in the SceneView.
        """

    def mouse_up(self, scene_view, event):
        """ A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """


class SceneLinkTool(BaseSceneTool):
    def draw(self, painter):
        """ The method which is responsible for drawing this tool.
        The tool for a SceneView will be drawn after everything else in
        the view has been drawn.

        This method is optional.
        """

    def mouse_down(self, scene_view, event):
        """ A callback which is fired whenever the user clicks in the
        SceneView.
        """

    def mouse_move(self, scene_view, event):
        """ A callback which is fired whenever the user moves the mouse
        in the SceneView.
        """

    def mouse_up(self, scene_view, event):
        """ A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
