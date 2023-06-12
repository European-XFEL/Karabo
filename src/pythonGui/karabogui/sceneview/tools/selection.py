# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from qtpy.QtCore import QPoint, QRect, Qt
from qtpy.QtGui import QColor, QPen
from traits.api import Any, Enum, HasStrictTraits, Instance, String

from karabogui.binding.api import DeviceProxy
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.sceneview.bases import BaseSceneTool
from karabogui.sceneview.const import HOVER_COLOR
from karabogui.sceneview.layout.layouts import GroupLayout
from karabogui.sceneview.utils import (
    round_down_to_grid, round_up_to_grid, save_painter_state)
from karabogui.sceneview.widget.unknown import UnknownSvgWidget

NONRESIZABLE_OBJECTS = (GroupLayout,)
NONSELECTABLE_OBJECTS = (UnknownSvgWidget,)


class ProxySelectionTool(BaseSceneTool):
    """A tool for the selection of widgets in the SceneView which are derived
    from ``ControllerContainer`` and bound to proxies.
    """

    def mouse_down(self, scene_view, event):
        # Only controller might have proxies
        controller = scene_view.controller_at_position(event.pos())
        if controller is not None:
            # Use the main proxy of the controller
            proxy = controller.widget_controller.proxy
            device = proxy.root_proxy
            if isinstance(device, DeviceProxy):  # ignore DeviceClassProxy
                if not device.online:
                    return  # ignore offline devices
                broadcast_event(KaraboEvent.ShowConfiguration,
                                {'proxy': device})
            return

    def mouse_move(self, scene_view, event):
        pass

    def mouse_up(self, scene_view, event):
        pass


class _SelectionRect(HasStrictTraits):
    selection_start = Instance(QPoint, args=())
    selection_stop = Instance(QPoint, args=())

    def start_drag(self, pos):
        self.selection_start = self.selection_stop = pos

    def rect(self):
        return QRect(self.selection_start, self.selection_stop)

    def draw(self, painter):
        """Draw the selection.
        """
        with save_painter_state(painter):
            painter.setPen(Qt.black)
            painter.drawRect(self.rect())


HOVER_TOLERANCE = 7
HOVER_QCOLOR = QColor(*(HOVER_COLOR + (128,)))


class SceneSelectionTool(BaseSceneTool):
    """A tool for selecting things in the SceneView
    """
    visible = True

    state = Enum('normal', 'draw', 'move', 'resize')

    _hover_item = Any
    _moving_pos = Instance(QPoint, args=())
    _resize_type = String
    _selection_rect = Instance(_SelectionRect, args=())

    def draw(self, scene_view, painter):
        """Draw the tool
        """
        selection_model = scene_view.selection_model
        self._draw_hovered_item(selection_model, painter)

        if self.state == 'draw':
            self._selection_rect.draw(painter)

    def mouse_down(self, scene_view, event):
        """A callback which is fired whenever the user clicks in the
        SceneView.
        """
        selection_model = scene_view.selection_model

        if self._hover_item is not None and len(self._resize_type) > 0:
            # We select the hover item
            self.state = 'resize'
            selection_model.clear_selection()
            selection_model.select_object(self._hover_item)
            event.accept()
            return

        mouse_pos = event.pos()

        if selection_model.has_multiple_selection():
            # If there is a multiple selection, we start moving it
            if selection_model.get_selection_bounds().contains(mouse_pos):
                self.state = 'move'
                self._moving_pos = mouse_pos
                event.accept()
                return

        item = scene_view.item_at_position(mouse_pos)
        if item is None:
            # No items are selected. We draw a selection rectangle instead.
            self.state = 'draw'
            self._selection_rect.start_drag(mouse_pos)
        else:
            # There is a selected or hovered item, which has preference.
            # We select and start moving it.
            item = self._hover_item or item
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
        """A callback which is fired whenever the user moves the mouse
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
        """A callback which is fired whenever the user ends a mouse click
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
        """Handle mouse moves when drawing the selection rect.
        """
        self._selection_rect.selection_stop = event.pos()
        event.accept()

    def _hover_move(self, scene_view, event):
        """Handles mouse moves when no buttons are pressed.
        """
        mouse_pos = event.pos()
        selection_model = scene_view.selection_model
        cursor = 'none'
        self._resize_type = ''

        if selection_model.has_multiple_selection():
            # Multiple selections. Does not support resizing.
            # Always in move mode.
            if selection_model.get_selection_bounds().contains(mouse_pos):
                cursor = 'open-hand'
        else:
            # Single selection. We check for move or resize mode
            self._hover_item = hover_item = scene_view.item_at_hover(mouse_pos)
            if hover_item is not None:
                # The following objects do not support resizing.
                # Always in move mode
                cursor = 'open-hand'
                if is_resizable(hover_item):
                    cursor = self._get_cursor(mouse_pos)

        scene_view.set_cursor(cursor)

    def _get_cursor(self, mouse_pos):
        rect = self._hover_item.geometry()
        top = abs(rect.top() - mouse_pos.y()) < HOVER_TOLERANCE
        bottom = abs(rect.bottom() - mouse_pos.y()) < HOVER_TOLERANCE
        left = abs(rect.left() - mouse_pos.x()) < HOVER_TOLERANCE
        right = abs(rect.right() - mouse_pos.x()) < HOVER_TOLERANCE
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
        return cursor

    def _move_move(self, scene_view, event):
        """Handles mouse moves when a scene object is being moved.
        """
        mouse_pos = event.pos()
        # Create a rect with scene view size and origin (0, 0)!
        og = QRect(QPoint(0, 0), scene_view.size())
        if not og.contains(mouse_pos):
            return
        trans = mouse_pos - self._moving_pos

        # Calculate snap translation
        if scene_view.snap_to_grid:
            origin = scene_view.selection_model.get_item_rect().topLeft()
            trans = _get_snap_offset(origin, offset=trans)

        # Translate only if there is an offset (can be zero from snap calc)
        if trans:
            for c in scene_view.selection_model:
                c.translate(trans)
            self._moving_pos += trans
        event.accept()

    def _resize_move(self, scene_view, event):
        """Handles mouse moves when a scene object is being resized.
        """
        mouse_pos = event.pos()
        og = self._hover_item.geometry()
        g = QRect(og)
        posX, posY = mouse_pos.x(), mouse_pos.y()

        # Get snap position if scene is in grid mode
        if scene_view.snap_to_grid:
            posX, posY = round_down_to_grid(posX), round_down_to_grid(posY)

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

    def _draw_hovered_item(self, selection_model, painter):
        hovered_item = self._hover_item

        # Don't draw an indicator if there's no hovered item
        if hovered_item is None:
            return

        # Also don't draw if the single selection is the hovered item
        selected_item = selection_model.get_single_selection()
        if hovered_item is selected_item:
            return

        pen = QPen(HOVER_QCOLOR)
        pen.setWidth(3)

        with save_painter_state(painter):
            painter.setPen(pen)
            painter.drawRect(hovered_item.geometry())


def _get_snap_offset(qpoint, offset=QPoint()):
    """Calculate snap offset from on the grid step and mouse offset"""
    trans_point = qpoint + offset

    # Get rounding function
    x_round = _round_func(offset.x())
    y_round = _round_func(offset.y())

    snapped_point = QPoint(x_round(trans_point.x()), y_round(trans_point.y()))

    return snapped_point - qpoint


def _round_func(direction):
    func = round_down_to_grid
    if direction <= 0:
        func = round_up_to_grid
    return func


def is_resizable(gui_obj):
    return not isinstance(gui_obj, NONRESIZABLE_OBJECTS)


def is_selectable(gui_obj):
    return not isinstance(gui_obj, NONSELECTABLE_OBJECTS)
