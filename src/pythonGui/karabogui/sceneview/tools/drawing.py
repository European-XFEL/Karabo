# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import sys
from pathlib import Path

from qtpy.QtCore import QLine, QLineF, QPoint, QRect, Qt
from qtpy.QtGui import (
    QBrush, QImageReader, QPainterPath, QPen, QPixmap, QTransform)
from qtpy.QtSvg import QSvgRenderer
from qtpy.QtWidgets import QDialog
from traits.api import Instance

from karabo.common.scenemodel.api import (
    ARROW_HEAD, ArrowModel, DeviceSceneLinkModel, ImageRendererModel,
    InstanceStatusModel, LineModel, RectangleModel, SceneLinkModel,
    SceneTargetWindow, StickerModel, WebLinkModel, convert_to_svg_image)
from karabogui import messagebox
from karabogui.dialogs.api import (
    DeviceCapabilityDialog, SceneLinkDialog, TextDialog, TopologyDeviceDialog,
    WebDialog)
from karabogui.fonts import get_font_metrics
from karabogui.pathparser import Parser
from karabogui.sceneview.bases import BaseSceneTool
from karabogui.sceneview.utils import calc_rotated_point, calc_snap_pos
from karabogui.singletons.api import get_config
from karabogui.util import getOpenFileName


class TextSceneTool(BaseSceneTool):
    def mouse_down(self, scene_view, event):
        pass

    def mouse_move(self, scene_view, event):
        pass

    def mouse_up(self, scene_view, event):
        """A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
        dialog = TextDialog(alignment=True, parent=scene_view)
        if dialog.exec() == QDialog.Rejected:
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
                              stroke="#000000")
            scene_view.add_models(model, initialize=True)
            scene_view.set_tool(None)
            scene_view.select_model(model)


class ArrowSceneTool(LineSceneTool):
    """The arrow tool is a derivative of the line tool as they are both
       1D shapes."""

    marker_path = Instance(QPainterPath)

    def draw(self, scene_view, painter):
        super(ArrowSceneTool, self).draw(scene_view, painter)
        if self.line is not None:
            line = QLineF(self.line)
            if line.length():
                # Calculate angle
                angle = line.angle()
                offset = self.line.p2() - calc_rotated_point(x=0, y=3,
                                                             angle=angle)
                # Calculate transform
                transform = QTransform()
                transform.translate(offset.x(), offset.y())
                transform.rotate(-angle)

                # Paint transformed path
                painter.setBrush(QBrush(Qt.black))
                painter.drawPath(transform.map(self.marker_path))

    def mouse_up(self, scene_view, event):
        """A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
        if self.line is not None:
            # Define an arrow model from the start and end points
            arrow_model = ArrowModel(x1=self.line.x1(), y1=self.line.y1(),
                                     x2=self.line.x2(), y2=self.line.y2(),
                                     stroke="#000000")
            scene_view.add_models(arrow_model, initialize=True)
            scene_view.set_tool(None)
            scene_view.select_model(arrow_model)

    def _marker_path_default(self):
        """Uses the arrow head path string to generate the painter path"""
        return Parser(ARROW_HEAD["path"]).parse()


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
                                   stroke="#000000")
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
        result = dialog.exec()
        if result == QDialog.Accepted:
            model.target = dialog.selectedScene
            model.target_window = dialog.selectedTargetWindow
            scene_view.add_models(model, initialize=True)
            scene_view.set_tool(None)


class DeviceSceneLinkTool(BaseSceneTool):
    def mouse_down(self, scene_view, event):
        pass

    def mouse_move(self, scene_view, event):
        pass

    def mouse_up(self, scene_view, event):
        """A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
        mouse_pos = event.pos()
        dialog = DeviceCapabilityDialog(parent=scene_view)
        result = dialog.exec()
        if result == QDialog.Accepted:
            device_id = dialog.device_id
            scene_name = dialog.capa_name

            _LINK_MARGIN = 10
            _LINK_SIZE_HIT = 30
            fm = get_font_metrics()
            model = DeviceSceneLinkModel(x=mouse_pos.x(), y=mouse_pos.y())
            width = max(fm.width(device_id) + _LINK_MARGIN, _LINK_SIZE_HIT)
            height = max(fm.height() + _LINK_MARGIN, _LINK_SIZE_HIT)
            model.width = width
            model.height = height

            # Link properties
            model.keys = [f"{device_id}.availableScenes"]
            model.text = f"{device_id}"
            model.target = scene_name
            model.target_window = SceneTargetWindow.Dialog
            scene_view.add_models(model, initialize=True)
            scene_view.set_tool(None)


class InstanceStatusTool(BaseSceneTool):
    def mouse_down(self, scene_view, event):
        pass

    def mouse_move(self, scene_view, event):
        pass

    def mouse_up(self, scene_view, event):
        """A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
        dialog = TopologyDeviceDialog(parent=scene_view)
        if dialog.exec() == QDialog.Rejected:
            return
        mouse_pos = event.pos()
        device_id = dialog.device_id
        model = InstanceStatusModel(
            x=mouse_pos.x(), y=mouse_pos.y(),
            width=30, height=30,
            keys=[f"{device_id}.deviceId"])
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
        if dialog.exec() == QDialog.Accepted:
            model.target = dialog.target
            scene_view.add_models(model, initialize=True)
            scene_view.set_tool(None)


class ImageRendererTool(BaseSceneTool):
    """This tool will place an `ImageRenderer` widget on the scene"""

    def mouse_down(self, scene_view, event):
        pass

    def mouse_move(self, scene_view, event):
        pass

    def mouse_up(self, scene_view, event):
        """A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
        fn = getOpenFileName(filter="Images (*.png *.jpg *.jpeg *.svg)",
                             directory=get_config()["data_dir"])
        if not fn:
            return

        get_config()["data_dir"] = str(Path(fn).parent)
        with open(fn, "rb") as fp:
            b = fp.read()

        limit = 102400  # 100 KB
        file_size = sys.getsizeof(b)
        if file_size > limit:
            scene_view.set_tool(None)
            messagebox.show_error("Selected image byte size is larger than "
                                  f"the allowed size of {limit // 1024}kB.",
                                  parent=scene_view)
            return

        image_format = str(QImageReader.imageFormat(fn), "utf-8")
        if image_format == "svg":
            renderer = QSvgRenderer(b)
            if not renderer.isValid() or renderer.defaultSize().isNull():
                scene_view.set_tool(None)
                messagebox.show_error(
                    "Selected image is not a valid svg file.",
                    parent=scene_view)
                return
            size = renderer.defaultSize()
        else:
            pixmap = QPixmap()
            pixmap.loadFromData(b)
            if pixmap.isNull():
                scene_view.set_tool(None)
                messagebox.show_error(
                    "Selected image is not a valid image file.",
                    parent=scene_view)
                return
            size = pixmap.size()

        mouse_pos = event.pos()
        image = convert_to_svg_image(image_format, b)
        model = ImageRendererModel(x=mouse_pos.x(), y=mouse_pos.y(),
                                   image=image,
                                   width=size.width(), height=size.height())
        scene_view.add_models(model, initialize=True)
        scene_view.set_tool(None)
