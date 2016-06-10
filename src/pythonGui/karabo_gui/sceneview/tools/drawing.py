from .bases import BaseSceneTool


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
