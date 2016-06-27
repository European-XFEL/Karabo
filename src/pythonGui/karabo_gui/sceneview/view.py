#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import os.path

from PyQt4.QtCore import Qt
from PyQt4.QtGui import (QPalette, QPainter, QPen, QSizePolicy, QStackedLayout,
                         QWidget)

from karabo_gui.scenemodel.api import (read_scene, FixedLayoutModel,
                                       SCENE_MIN_WIDTH, SCENE_MIN_HEIGHT)
from .bases import BaseSceneTool
from .builder import (bring_object_to_front, create_object_from_model,
                      fill_root_layout, remove_object_from_layout,
                      send_object_to_back)
from .const import QT_CURSORS
from .layout.api import GroupLayout
from .selection_model import SceneSelectionModel
from .tools.api import SceneSelectionTool
from .utils import save_painter_state


class SceneView(QWidget):
    """ An object representing the view for a Karabo GUI scene.
    """

    def __init__(self, parent=None, design_mode=False):
        super(SceneView, self).__init__(parent)

        layout_model = FixedLayoutModel(x=0, y=0, width=SCENE_MIN_WIDTH,
                                        height=SCENE_MIN_HEIGHT)
        # XXX: This is an interesting hack.
        # Basically, we want to put all of the scene's children into a widget
        # which is parented to this widget and completely covers it.
        # Then, the `design_mode` property toggles the mouse handling for this
        # child widget. In design mode, we want the main scene view and its
        # tools to get the mouse events. Out of design mode, we re-enable mouse
        # handling for all of the scene's children.
        self.layout = GroupLayout(layout_model, parent=self)
        self.inner = QWidget(self)
        self.inner.setLayout(self.layout)
        layout = QStackedLayout(self)
        layout.addWidget(self.inner)

        self.title = None
        self.scene_model = None
        self.selection_model = SceneSelectionModel()
        self.current_tool = None
        self.design_mode = design_mode
        self._scene_obj_cache = {}

        self.setFocusPolicy(Qt.StrongFocus)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.setAcceptDrops(True)
        self.setAttribute(Qt.WA_MouseTracking)
        self.setBackgroundRole(QPalette.Window)
        self.resize(SCENE_MIN_WIDTH, SCENE_MIN_HEIGHT)

    @property
    def design_mode(self):
        # If the inner view is transparent to events, this view is handling
        return self.inner.testAttribute(Qt.WA_TransparentForMouseEvents)

    @design_mode.setter
    def design_mode(self, value):
        # Toggle mouse handling in the inner view
        self.inner.setAttribute(Qt.WA_TransparentForMouseEvents, value)
        if not value:
            self.selection_model.clear_selection()
            self.set_tool(None)

    # ----------------------------
    # Qt Methods

    def mouseMoveEvent(self, event):
        if self.design_mode:
            self.current_tool.mouse_move(self, event)
            if event.isAccepted():
                self.update()
            else:
                super(SceneView, self).mouseMoveEvent(event)

    def mousePressEvent(self, event):
        if self.design_mode:
            self.current_tool.mouse_down(self, event)
            if event.isAccepted():
                self.update()
            else:
                super(SceneView, self).mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        if self.design_mode:
            self.current_tool.mouse_up(self, event)
            if event.isAccepted():
                self.update()
            else:
                super(SceneView, self).mouseReleaseEvent(event)

    def mouseDoubleClickEvent(self, event):
        if self.design_mode:
            item = self.item_at_position(event.pos())
            if item is not None:
                item.edit()
                self.update()
            event.accept()

    def paintEvent(self, event):
        """ Show view content.
        """
        with QPainter(self) as painter:
            self.layout.draw(painter)
            self._draw_selection(painter)
            if self.current_tool.visible:
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
        fill_root_layout(self.layout, self.scene_model, self.inner,
                         self._scene_obj_cache)

    def item_at_position(self, pos):
        """ Returns the topmost object whose bounds contain `pos`.
        """
        for child in self.scene_model.children[::-1]:
            obj = self._scene_obj_cache.get(child)
            if obj is not None and obj.geometry().contains(pos):
                return obj

    def items_in_rect(self, rect):
        """ Returns the topmost objects whose bounds are contained in `rect`.
        """
        items = []
        for child in self.scene_model.children[::-1]:
            obj = self._scene_obj_cache.get(child)
            if obj is not None and rect.contains(obj.geometry()):
                items.append(obj)
        return items

    def select_all(self):
        """ Add all children to the current selection
        """
        self.selection_model.clear_selection()
        for child in self.scene_model.children:
            obj = self._scene_obj_cache.get(child)
            if obj is not None:
                self.selection_model.select_object(obj)
        self.update()

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
            self.current_tool = SceneSelectionTool()

        self.update()

    def add_models(self, *models):
        """ Adds new child models to the scene model."""
        self.scene_model.children.extend(models)

    def remove_model(self, model):
        """ Removes the given ``model`` from the scene model."""
        self.scene_model.children.remove(model)

    def bring_to_front(self, model):
        """ The given ``model`` is moved to the end of the list."""
        # Remove model
        self.remove_model(model)
        # Add model to end
        self.add_models(model)

        scene_obj = self._scene_obj_cache.get(model)
        if scene_obj is not None:
            # In case of layouts or widgets
            bring_object_to_front(scene_obj)

    def send_to_back(self, model):
        """ The given ``model`` is moved to the beginning of the list."""
        # Copy list to trigger traits notification only once when its set at
        # the end
        children = list(self.scene_model.children)
        # Clean list to redraw all models in correct order
        del self.scene_model.children[:]
        # Remove model
        children.remove(model)
        # Insert model at beginning
        children.insert(0, model)
        self.scene_model.children.extend(children)

        scene_obj = self._scene_obj_cache.get(model)
        if scene_obj is not None:
            # In case of layouts or widgets
            send_object_to_back(scene_obj)

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
                remove_object_from_layout(obj, self.layout,
                                          self._scene_obj_cache)
        for model in event.added:
            create_object_from_model(self.layout, model, self.inner,
                                     self._scene_obj_cache)
