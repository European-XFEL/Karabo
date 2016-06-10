#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import os.path

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QPalette, QPainter, QPen, QSizePolicy, QWidget

from karabo_gui.scenemodel.api import (
    BoxLayoutModel, GridLayoutModel, LabelModel, LineModel, PathModel,
    RectangleModel, FixedLayoutModel, UnknownXMLDataModel, read_scene,
    SCENE_MIN_WIDTH, SCENE_MIN_HEIGHT
)
from .bases import BaseSceneTool
from .const import QT_BOX_LAYOUT_DIRECTION, QT_CURSORS
from .layouts import BoxLayout, GridLayout, GroupLayout
from .selection_model import SceneSelectionModel
from .shapes import LineShape, PathShape, RectangleShape
from .simple_widgets import LabelWidget, UnknownSvgWidget


def fill_root_layout(layout, parent_model, scene_widget):
    # Go through children and create corresponding GUI objects
    for child in parent_model.children:
        if isinstance(child, FixedLayoutModel):
            obj = GroupLayout(child)
            layout.add_layout(obj)
            fill_root_layout(obj, child, scene_widget)
        if isinstance(child, BoxLayoutModel):
            obj = BoxLayout(child, QT_BOX_LAYOUT_DIRECTION[child.direction])
            fill_root_layout(obj, child, scene_widget)
        if isinstance(child, GridLayoutModel):
            obj = GridLayout(child)
            fill_root_layout(obj, child, scene_widget)
        if isinstance(child, LineModel):
            obj = LineShape(child)
            layout.add_shape(obj)
        if isinstance(child, RectangleModel):
            obj = RectangleShape(child)
            layout.add_shape(obj)
        if isinstance(child, PathModel):
            obj = PathShape(child)
            layout.add_shape(obj)
        if isinstance(child, LabelModel):
            obj = LabelWidget(child, scene_widget)
            layout.add_widget(obj)
        if isinstance(child, UnknownXMLDataModel):
            obj = UnknownSvgWidget.create(child, parent=scene_widget)
            if obj is not None:
                layout.add_widget(obj)
            # XXX: UnknownXMLDataModel has a list of children, which might
            # include regular models. We're ignoring those children for now.


class SceneView(QWidget):
    """ An object representing the view for a Karabo GUI scene.
    """

    def __init__(self, parent=None, designMode=False):
        super(SceneView, self).__init__(parent)

        self.title = None
        self.designMode = designMode
        self.scene_model = None
        self.selection_model = SceneSelectionModel()
        self.current_tool = None

        self.layout = GroupLayout(None, parent=self)

        self.setFocusPolicy(Qt.StrongFocus)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.setAcceptDrops(True)
        self.setAttribute(Qt.WA_MouseTracking)
        self.setBackgroundRole(QPalette.Window)
        self.resize(SCENE_MIN_WIDTH, SCENE_MIN_HEIGHT)

    # ----------------------------
    # Qt Methods

    def mouseMoveEvent(self, event):
        if self.current_tool is not None:
            self.current_tool.mouse_move(self, event)
            if event.isAccepted():
                self.update()
            else:
                super(SceneView, self).mouseMoveEvent(event)

    def mousePressEvent(self, event):
        if self.current_tool is not None:
            self.current_tool.mouse_down(self, event)
            if event.isAccepted():
                self.update()
            else:
                super(SceneView, self).mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        if self.current_tool is not None:
            self.current_tool.mouse_up(self, event)
            if event.isAccepted():
                self.update()
            else:
                super(SceneView, self).mouseReleaseEvent(event)

    def paintEvent(self, event):
        """ Show view content.
        """
        with QPainter(self) as painter:
            self.layout.draw(painter)
            self._draw_selection(painter)
            if self.current_tool is not None and self.current_tool.visible:
                self.current_tool.draw(painter)

    # ----------------------------
    # Public methods

    def load(self, filename):
        """ The given ``filename`` is loaded.
        """
        # Set name
        self.title = os.path.basename(filename)
        # Read file into scene model
        self.scene_model = read_scene(filename)
        # Set width and height
        self.resize(max(self.scene_model.width, SCENE_MIN_WIDTH),
                    max(self.scene_model.height, SCENE_MIN_HEIGHT))
        fill_root_layout(self.layout, self.scene_model, self)

    def item_at_position(self, pos):
        """ Returns the topmost object whose bounds contain `pos`.
        """
        raise NotImplementedError

    def set_cursor(self, name):
        """ Sets the cursor for the scene view.
        """
        if name == 'none':
            self.unsetCursor()
        else:
            self.setCursor(QT_CURSORS[name])

    def set_tool(self, tool):
        """ Sets the current tool being used by the view.
        """
        assert tool is None or isinstance(tool, BaseSceneTool)
        self.current_tool = tool
        if tool is None:
            self.set_cursor('none')

        self.update()

    # ----------------------------
    # Private methods (yes, I know... It's just a convention)

    def _draw_selection(self, painter):
        """ Draw a dashed rect around the selected objects. """
        if not self.selection_model.has_selection():
            return

        black = QPen(Qt.black)
        black.setStyle(Qt.DashLine)
        white = QPen(Qt.white)

        rect = self.selection_model.get_selection_bounds()
        painter.setPen(white)
        painter.drawRect(rect)
        painter.setPen(black)
        painter.drawRect(rect)
