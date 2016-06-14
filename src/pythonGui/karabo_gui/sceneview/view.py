#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import os.path

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QPalette, QPainter, QPen, QSizePolicy, QWidget

from karabo_gui.scenemodel.api import (read_scene, SCENE_MIN_WIDTH,
                                       SCENE_MIN_HEIGHT)
from .bases import BaseSceneTool
from .builder import (create_object_from_model, fill_root_layout,
                      remove_object_from_layout)
from .const import QT_CURSORS
from .layouts import GroupLayout
from .selection_model import SceneSelectionModel
from .tools.api import SceneSelectionTool
from .utils import save_painter_state


class SceneView(QWidget):
    """ An object representing the view for a Karabo GUI scene.
    """

    def __init__(self, parent=None, design_mode=False):
        super(SceneView, self).__init__(parent)

        self.title = None
        self.design_mode = design_mode
        self.scene_model = None
        self.selection_model = SceneSelectionModel()
        self.current_tool = None
        self._scene_obj_cache = {}

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
        self._set_scene_model(read_scene(filename))
        # Set width and height
        self.resize(max(self.scene_model.width, SCENE_MIN_WIDTH),
                    max(self.scene_model.height, SCENE_MIN_HEIGHT))

        self._scene_obj_cache = {}
        fill_root_layout(self.layout, self.scene_model, self,
                         self._scene_obj_cache)

    def item_at_position(self, pos):
        """ Returns the topmost object whose bounds contain `pos`.
        """
        for child in self.scene_model.children:
            obj = self._scene_obj_cache.get(child)
            if obj is not None and obj.geometry().contains(pos):
                return obj

    def items_in_rect(self, rect):
        """ Returns the topmost objects whose bounds are contained in `rect`.
        """
        items = []
        for child in self.scene_model.children:
            obj = self._scene_obj_cache.get(child)
            if obj is not None and rect.contains(obj.geometry()):
                items.append(obj)
        return items

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
            if self.design_mode:
                self.current_tool = SceneSelectionTool()

        self.update()

    def add_model(self, model):
        """ Adds a new model to the scene model."""
        self.scene_model.children.append(model)

    def remove_model(self, model):
        """ Removes the given ``model`` from the scene model."""
        self.scene_model.children.remove(model)

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
        with save_painter_state(painter):
            painter.setPen(white)
            painter.drawRect(rect)
            painter.setPen(black)
            painter.drawRect(rect)

    def _set_scene_model(self, scene_model):
        """ The scene model is set and all notification handlers are defined.
        """
        self.scene_model = scene_model
        self.scene_model.on_trait_change(self._model_modified,
                                         'children_items')

    def _model_modified(self, event):
        """ The scene model got modified."""
        for model in event.removed:
            obj = self._scene_obj_cache.get(model)
            if obj is not None:
                remove_object_from_layout(obj, self.layout)
                # Hide object from scene until reparenting is done
                obj.hide()
        for model in event.added:
            create_object_from_model(self.layout, model, self,
                                     self._scene_obj_cache)
