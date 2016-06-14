from PyQt4.QtGui import QDialog

from karabo_gui.dialogs.textdialog import TextDialog
from karabo_gui.sceneview.bases import BaseSceneTool
from karabo_gui.sceneview.simple_widgets import LabelWidget


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
        dialog = TextDialog()
        if dialog.exec() == QDialog.Rejected:
            return

        model = dialog.label_model
        pos = event.pos()
        model.x = pos.x()
        model.y = pos.y()
        scene_view.add_model(model)
        scene_view.set_tool(None)
        scene_view.remove_model(model)


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
