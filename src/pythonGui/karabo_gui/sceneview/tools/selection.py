from PyQt4.QtCore import Qt, QRect, QPoint
from traits.api import Any

from .bases import BaseSceneTool
from karabo_gui.sceneview.utils import save_painter_state


class SceneSelectionTool(BaseSceneTool):
    """ A tool for selecting things in the SceneView
    """
    # Traits so we don't need an __init__ method
    selection_start = Any
    selection_stop = Any
    resize = Any
    moving_item = Any

    def draw(self, painter):
        """ Draw the selection. """
        if self.selection_start is not None:
            rect = QRect(self.selection_start, self.selection_stop)
            with save_painter_state(painter):
                painter.setPen(Qt.black)
                painter.drawRect(rect)

    def mouse_down(self, scene_view, event):
        """ A callback which is fired whenever the user clicks in the
        SceneView.
        """
        if self.resize:
            return
        item = scene_view.item_at_position(event.pos())
        if item is None:
            self.selection_stop = self.selection_start = event.pos()
        else:
            if event.modifiers() & Qt.ShiftModifier:
                item.selected = not item.selected
            else:
                scene_view.clear_selection()
                item.selected = True
        event.accept()

    def mouse_move(self, scene_view, event):
        """ A callback which is fired whenever the user moves the mouse
        in the SceneView.
        """
        if not event.buttons():
            self._hover_move(scene_view, event)
        else:
            if self.resize == 'm':
                if event.pos().x() < 0 or event.pos().y() < 0:
                    return
                for c in scene_view.selection_model:
                    trans = event.pos() - self.moving_pos
                    c.translate(trans)
                self.moving_pos = event.pos()
                event.accept()
                scene_view.setModified()
            elif self.selection_start is not None:
                self.selection_stop = event.pos()
                event.accept()
            elif self.resize:
                og = self.resize_item.geometry()
                g = QRect(og)
                posX, posY = event.pos().x(), event.pos().y()
                trans = QPoint()
                if "t" in self.resize:
                    trans.setY((posY - g.top()) / 2)
                    g.setTop(posY)
                elif "b" in self.resize:
                    trans.setY((posY - g.bottom()) / 2)
                    g.setBottom(posY)
                if "l" in self.resize:
                    trans.setX((posX - g.left()) / 2)
                    g.setLeft(posX)
                elif "r" in self.resize:
                    trans.setX((posX - g.right()) / 2)
                    g.setRight(posX)
                min = self.resize_item.minimumSize()
                max = self.resize_item.maximumSize()
                if (not min.width() <= g.size().width() <= max.width() or
                    not min.height() <= g.size().height() <= max.height() and
                    (min.width() <= og.size().width() <= max.width() and
                     min.height() <= og.size().height() <= max.height())):
                    return

                self.resize_item.set_geometry(g)
                scene_view.ilayout.update()
                scene_view.setModified()
            scene_view.update()

    def mouse_up(self, scene_view, event):
        """ A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
        self.resize = ""
        self.resize_item = None
        if self.selection_start is not None:
            rect = QRect(self.selection_start, self.selection_stop)
            if not event.modifiers() & Qt.ShiftModifier:
                scene_view.clear_selection()
            for c in scene_view.ilayout:
                if rect.contains(c.geometry()):
                    c.selected = True
            for s in scene_view.ilayout.shapes:
                if rect.contains(s.geometry()):
                    s.selected = True
            self.selection_start = None
            event.accept()
            scene_view.update()

    def _hover_move(self, scene_view, event):
        """ Handles mouse moves when no buttons are pressed.
        """
        mouse_pos = event.pos()
        item = scene_view.item_at_position(mouse_pos)
        cursor = 'none'
        if item is not None:
            rect = item.geometry()
            top = abs(rect.top() - mouse_pos.y()) < 5
            bottom = abs(rect.bottom() - mouse_pos.y()) < 5
            left = abs(rect.left() - mouse_pos.x()) < 5
            right = abs(rect.right() - mouse_pos.x()) < 5
            on_sides = left or right
            on_ends = top or bottom
            if on_ends and not on_sides:
                cursor = 'resize-vertical'
            elif on_sides and not on_ends:
                cursor = 'resize-horizontal'
            elif (top and left) or (bottom and right):
                cursor = 'resize-diagonal-tlbr'
            elif (top and right) or (bottom and left):
                cursor = 'resize-diagonal-trbl'
            if cursor == 'none':
                cursor = 'open-hand'
        scene_view.set_cursor(cursor)
