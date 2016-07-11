from PyQt4.QtCore import Qt, QRect, QPoint
from traits.api import HasStrictTraits, Any, Enum, Instance, String

from karabo_gui.sceneview.bases import BaseSceneTool
from karabo_gui.sceneview.utils import save_painter_state


class _SelectionRect(HasStrictTraits):
    selection_start = Instance(QPoint, args=())
    selection_stop = Instance(QPoint, args=())

    def start_drag(self, pos):
        self.selection_start = self.selection_stop = pos

    def rect(self):
        return QRect(self.selection_start, self.selection_stop)

    def draw(self, painter):
        """ Draw the selection. """
        with save_painter_state(painter):
            painter.setPen(Qt.black)
            painter.drawRect(self.rect())


class SceneSelectionTool(BaseSceneTool):
    """ A tool for selecting things in the SceneView
    """
    visible = True

    state = Enum('normal', 'draw', 'move', 'resize')

    _hover_item = Any
    _moving_pos = Instance(QPoint, args=())
    _resize_type = String
    _selection_rect = Instance(_SelectionRect, args=())

    def draw(self, painter):
        """ Draw the tool. """
        if self.state == 'draw':
            self._selection_rect.draw(painter)

    def mouse_down(self, scene_view, event):
        """ A callback which is fired whenever the user clicks in the
        SceneView.
        """
        selection_model = scene_view.selection_model

        if self._hover_item is not None and len(self._resize_type) > 0:
            self.state = 'resize'
            selection_model.clear_selection()
            selection_model.select_object(self._hover_item)
            event.accept()
            return

        mouse_pos = event.pos()
        item = scene_view.item_at_position(mouse_pos)
        if item is None:
            if selection_model.get_selection_bounds().contains(mouse_pos):
                self.state = 'move'
                self._moving_pos = mouse_pos
            else:
                self.state = 'draw'
                self._selection_rect.start_drag(mouse_pos)
        else:
            self.state = 'move'
            self._moving_pos = mouse_pos
            if event.modifiers() & Qt.ShiftModifier:
                if item in selection_model:
                    selection_model.deselect_object(item)
                else:
                    selection_model.select_object(item)
            elif item not in selection_model:
                selection_model.clear_selection()
                selection_model.select_object(item)
        event.accept()

    def mouse_move(self, scene_view, event):
        """ A callback which is fired whenever the user moves the mouse
        in the SceneView.
        """
        if not event.buttons():
            self._hover_move(scene_view, event)
        else:
            if self.state == 'move':
                self._move_move(scene_view, event)
            elif self.state == 'draw':
                self._draw_move(scene_view, event)
            elif self.state == 'resize':
                self._resize_move(scene_view, event)

    def mouse_up(self, scene_view, event):
        """ A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
        if self.state == 'draw':
            selection_model = scene_view.selection_model
            if not event.modifiers() & Qt.ShiftModifier:
                selection_model.clear_selection()
            items = scene_view.items_in_rect(self._selection_rect.rect())
            for item in items:
                selection_model.select_object(item)
            event.accept()

        self._hover_item = None
        self._resize_type = ''
        self.state = 'normal'

    def _draw_move(self, scene_view, event):
        """ Handle mouse moves when drawing the selection rect.
        """
        self._selection_rect.selection_stop = event.pos()
        event.accept()

    def _hover_move(self, scene_view, event):
        """ Handles mouse moves when no buttons are pressed.
        """
        mouse_pos = event.pos()
        selection_model = scene_view.selection_model
        self._hover_item = scene_view.item_at_position(mouse_pos)
        cursor = 'none'
        if self._hover_item is not None:
            rect = self._hover_item.geometry()
            top = abs(rect.top() - mouse_pos.y()) < 5
            bottom = abs(rect.bottom() - mouse_pos.y()) < 5
            left = abs(rect.left() - mouse_pos.x()) < 5
            right = abs(rect.right() - mouse_pos.x()) < 5
            on_sides = left or right
            on_ends = top or bottom
            if on_ends and not on_sides:
                cursor = 'resize-vertical'
                self._resize_type = 't' if top else 'b'
            elif on_sides and not on_ends:
                cursor = 'resize-horizontal'
                self._resize_type = 'l' if left else 'r'
            elif (top and left) or (bottom and right):
                cursor = 'resize-diagonal-tlbr'
                self._resize_type = 'tl' if top else 'br'
            elif (top and right) or (bottom and left):
                cursor = 'resize-diagonal-trbl'
                self._resize_type = 'tr' if top else 'bl'
            else:
                cursor = 'open-hand'
                self._resize_type = ''
        elif selection_model.get_selection_bounds().contains(mouse_pos):
            cursor = 'open-hand'
            self._resize_type = ''
        scene_view.set_cursor(cursor)

    def _move_move(self, scene_view, event):
        """ Handles mouse moves when a scene object is being moved.
        """
        mouse_pos = event.pos()
        if mouse_pos.x() < 0 or mouse_pos.y() < 0:
            return
        trans = mouse_pos - self._moving_pos
        self._moving_pos = mouse_pos
        for c in scene_view.selection_model:
            c.translate(trans)
        event.accept()

    def _resize_move(self, scene_view, event):
        """ Handles mouse moves when a scene object is being resized.
        """
        mouse_pos = event.pos()
        og = self._hover_item.geometry()
        g = QRect(og)
        posX, posY = mouse_pos.x(), mouse_pos.y()
        if "t" in self._resize_type:
            g.setTop(posY)
        elif "b" in self._resize_type:
            g.setBottom(posY)
        if "l" in self._resize_type:
            g.setLeft(posX)
        elif "r" in self._resize_type:
            g.setRight(posX)

        min_size = self._hover_item.minimumSize()
        max_size = self._hover_item.maximumSize()
        if (not min_size.width() <= g.size().width() <= max_size.width() or
            not min_size.height() <= g.size().height() <= max_size.height() and
            (min_size.width() <= og.size().width() <= max_size.width() and
                min_size.height() <= og.size().height() <= max_size.height())):
            return

        self._hover_item.set_geometry(g)
