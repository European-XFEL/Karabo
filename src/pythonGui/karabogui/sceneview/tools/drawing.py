from PyQt5.QtCore import QLine, QPoint, QRect
from PyQt5.QtGui import QPen
from PyQt5.QtWidgets import QDialog
from traits.api import Instance

from karabo.common.scenemodel.api import (
    LineModel, RectangleModel, SceneLinkModel, StickerModel, WebLinkModel)
from karabogui.dialogs.dialogs import SceneLinkDialog
from karabogui.dialogs.textdialog import TextDialog
from karabogui.dialogs.webdialog import WebDialog
from karabogui.sceneview.bases import BaseSceneTool
from karabogui.sceneview.utils import calc_snap_pos


class TextSceneTool(BaseSceneTool):
    def mouse_down(self, scene_view, event):
        pass

    def mouse_move(self, scene_view, event):
        pass

    def mouse_up(self, scene_view, event):
        """A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
        dialog = TextDialog(parent=scene_view)
        if dialog.exec_() == QDialog.Rejected:
            return

        model = dialog.label_model
        pos = event.pos()
        model.x = pos.x()
        model.y = pos.y()
        scene_view.add_models(model, initialize=True)
        scene_view.set_tool(None)


class LineSceneTool(BaseSceneTool):
    visible = True
    line = Instance(QLine)
    start_pos = Instance(QPoint)

    def draw(self, scene_view, painter):
        """Draw the line
        """
        if self.line is not None:
            painter.setPen(QPen())
            painter.drawLine(self.line)

    def mouse_down(self, scene_view, event):
        """A callback which is fired whenever the user clicks in the
        SceneView.
        """
        pos = event.pos()
        if scene_view.snap_to_grid:
            pos = calc_snap_pos(pos)
        self.start_pos = pos
        self.line = QLine(pos, pos)

    def mouse_move(self, scene_view, event):
        """A callback which is fired whenever the user moves the mouse
        in the SceneView.
        """
        if event.buttons() and self.line is not None:
            pos = event.pos()
            if scene_view.snap_to_grid:
                pos = calc_snap_pos(pos)
            self.line.setPoints(self.start_pos, pos)

    def mouse_up(self, scene_view, event):
        """A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
        if self.line is not None:
            model = LineModel(x1=self.line.x1(), y1=self.line.y1(),
                              x2=self.line.x2(), y2=self.line.y2(),
                              stroke='#000000')
            scene_view.add_models(model, initialize=True)
            scene_view.set_tool(None)
            scene_view.select_model(model)


class RectangleSceneTool(BaseSceneTool):
    visible = True
    rect = Instance(QRect)
    start_pos = Instance(QPoint)

    def draw(self, scene_view, painter):
        """Draw the rect
        """
        if self.rect is not None:
            painter.setPen(QPen())
            painter.drawRect(self.rect)

    def mouse_down(self, scene_view, event):
        """A callback which is fired whenever the user clicks in the
        SceneView.
        """
        self.start_pos = event.pos()
        self.rect = QRect(self.start_pos, self.start_pos)

    def mouse_move(self, scene_view, event):
        """A callback which is fired whenever the user moves the mouse
        in the SceneView.
        """
        if event.buttons() and self.rect is not None:
            self.rect = QRect(self.start_pos, event.pos())

    def mouse_up(self, scene_view, event):
        """A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
        if self.rect is not None:
            model = RectangleModel(x=self.rect.x(), y=self.rect.y(),
                                   height=self.rect.height(),
                                   width=self.rect.width(),
                                   stroke='#000000')
            scene_view.add_models(model, initialize=True)
            scene_view.set_tool(None)


class SceneLinkTool(BaseSceneTool):
    def mouse_down(self, scene_view, event):
        pass

    def mouse_move(self, scene_view, event):
        pass

    def mouse_up(self, scene_view, event):
        """A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
        mouse_pos = event.pos()
        model = SceneLinkModel(x=mouse_pos.x(), y=mouse_pos.y(),
                               width=100, height=30)
        dialog = SceneLinkDialog(model, parent=scene_view)
        result = dialog.exec_()
        if result == QDialog.Accepted:
            model.target = dialog.selectedScene
            model.target_window = dialog.selectedTargetWindow
            scene_view.add_models(model, initialize=True)
            scene_view.set_tool(None)


class StickerTool(BaseSceneTool):
    def mouse_down(self, scene_view, event):
        pass

    def mouse_move(self, scene_view, event):
        pass

    def mouse_up(self, scene_view, event):
        """A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
        mouse_pos = event.pos()
        model = StickerModel(x=mouse_pos.x(), y=mouse_pos.y(),
                             width=100, height=100)
        scene_view.add_models(model, initialize=True)
        scene_view.set_tool(None)


class WebLinkTool(BaseSceneTool):
    def mouse_down(self, scene_view, event):
        pass

    def mouse_move(self, scene_view, event):
        pass

    def mouse_up(self, scene_view, event):
        """A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
        mouse_pos = event.pos()
        model = WebLinkModel(x=mouse_pos.x(), y=mouse_pos.y(),
                             width=100, height=30)
        dialog = WebDialog(parent=scene_view)
        if dialog.exec_() == QDialog.Accepted:
            model.target = dialog.target
            scene_view.add_models(model, initialize=True)
            scene_view.set_tool(None)
