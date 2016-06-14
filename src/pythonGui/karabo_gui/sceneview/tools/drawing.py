from PyQt4.QtCore import QLine, QPoint, QRect
from PyQt4.QtGui import QDialog, QPen
from traits.api import Instance

from karabo_gui.dialogs.textdialog import TextDialog
from karabo_gui.scenemodel.api import LineModel, RectangleModel
from karabo_gui.sceneview.bases import BaseSceneTool


class TextSceneTool(BaseSceneTool):
    def mouse_down(self, scene_view, event):
        pass

    def mouse_move(self, scene_view, event):
        pass

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


class LineSceneTool(BaseSceneTool):
    visible = True
    line = Instance(QLine)
    start_pos = Instance(QPoint)

    def draw(self, painter):
        """ Draw the line
        """
        if self.line is not None:
            painter.setPen(QPen())
            painter.drawLine(self.line)

    def mouse_down(self, scene_view, event):
        """ A callback which is fired whenever the user clicks in the
        SceneView.
        """
        self.start_pos = event.pos()
        self.line = QLine(self.start_pos, self.start_pos)

    def mouse_move(self, scene_view, event):
        """ A callback which is fired whenever the user moves the mouse
        in the SceneView.
        """
        if event.buttons() and self.line is not None:
            self.line.setPoints(self.start_pos, event.pos())

    def mouse_up(self, scene_view, event):
        """ A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
        if self.line is not None:
            model = LineModel(x1=self.line.x1(), y1=self.line.y1(),
                              x2=self.line.x2(), y2=self.line.y2(),
                              stroke='#000000')
            scene_view.add_model(model)
            scene_view.set_tool(None)


class RectangleSceneTool(BaseSceneTool):
    visible = True
    rect = Instance(QRect)
    start_pos = Instance(QPoint)

    def draw(self, painter):
        """ Draw the rect
        """
        if self.rect is not None:
            painter.setPen(QPen())
            painter.drawRect(self.rect)

    def mouse_down(self, scene_view, event):
        """ A callback which is fired whenever the user clicks in the
        SceneView.
        """
        self.start_pos = event.pos()
        self.rect = QRect(self.start_pos, self.start_pos)

    def mouse_move(self, scene_view, event):
        """ A callback which is fired whenever the user moves the mouse
        in the SceneView.
        """
        if event.buttons() and self.rect is not None:
            self.rect = QRect(self.start_pos, event.pos())

    def mouse_up(self, scene_view, event):
        """ A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
        if self.rect is not None:
            model = RectangleModel(x=self.rect.x(), y=self.rect.y(),
                                   height=self.rect.height(),
                                   width=self.rect.width(),
                                   stroke='#000000')
            scene_view.add_model(model)
            scene_view.set_tool(None)


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
