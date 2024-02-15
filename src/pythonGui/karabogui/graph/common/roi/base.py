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
import numpy as np
from pyqtgraph import ROI, Point, TextItem, functions as fn
from qtpy.QtCore import QPoint, Qt, Slot
from qtpy.QtGui import QTransform
from qtpy.QtWidgets import QAction, QInputDialog, QLabel, QMenu, QWidgetAction

from karabogui import icons
from karabogui.graph.common.utils import float_to_string

from .utils import set_roi_html


class KaraboROI(ROI):

    def __init__(self, pos=(0, 0), size=Point(1, 1), name='',
                 scaleSnap=False, translateSnap=False, pen=None,
                 parent=None):
        super().__init__(pos, size,
                         scaleSnap=scaleSnap,
                         translateSnap=translateSnap,
                         pen=pen, removable=True, parent=parent)
        self.name = name
        self.setZValue(100)
        self._selected = False
        self._scaling = np.array([1, 1])
        self._translation = np.array([0, 0])

        self._add_handles()

        self.textItem = TextItem(
            html=set_roi_html(name=self.name, center=(0, 0)),
            fill=(0, 0, 0, 50))
        self.textItem.setZValue(99)
        self._change_roi_text_item_details()

        self.sigRegionChanged.connect(self._change_roi_text_item_details)
        self.setAcceptedMouseButtons(Qt.RightButton | Qt.LeftButton)
        self.set_visible(False)

    def _add_handles(self):
        pass

    def mouseClickEvent(self, event):
        super().mouseClickEvent(event)

    # ---------------------------------------------------------------------
    # Public methods

    def update_geometry_from_transform(self, scaling, translation,
                                       update=True):
        self.setPos(self._absolute_position * scaling + translation,
                    update=False)
        self.setSize(self._absolute_size * scaling, update=update)
        self.stateChanged(finish=True)

        self._scaling = scaling
        self._translation = translation
        self._change_roi_text_item_details()

    def set_visible(self, visible=True):
        self.setVisible(visible)
        if self.textItem:
            self.textItem.setVisible(self.selected and visible)

    def redraw(self):
        self.update()

    def _set_handle_visible(self, visible):
        for handle in self.handles:
            handle["item"].setVisible(visible)

    # ---------------------------------------------------------------------
    # PyQt slots

    @Slot()
    def _change_roi_text_item_details(self):
        if not self.textItem:
            return

        direction = np.array([0, 1])  # lower left
        self.textItem.setPos(*(self.pos() + (self.size() * direction)))
        center = [float_to_string(coord) for coord in self.center]
        size = [float_to_string(s) for s in self.size()]
        self.textItem.setHtml(set_roi_html(name=self.name,
                                           center=center,
                                           size=size))

    # ---------------------------------------------------------------------
    # Properties

    @property
    def selected(self):
        return self._selected

    @selected.setter
    def selected(self, selected):
        self._selected = selected

        # Set the visual changes to reflect the change
        width = 2 if selected else 1
        self.pen.setWidthF(width)
        self.currentPen.setWidthF(width)
        self._set_handle_visible(selected)

        # Redraw ROI item
        self.redraw()

        # Then toggle the textItem to show only when ROI selected
        if self.textItem:
            self.textItem.setVisible(selected)

    @property
    def _absolute_position(self):
        return np.array((self.pos() - self._translation) / self._scaling)

    @property
    def _absolute_size(self):
        return np.array(self.size()) / self._scaling

    @property
    def center(self):
        return self.pos() + (self.size() / 2)

    @property
    def coords(self):
        return tuple(self._absolute_position) + tuple(self._absolute_size)

    # ---------------------------------------------------------------------
    # PyQtGraph methods

    def _makePen(self):
        # Generate the pen color for this ROI based on its current state.
        if not self.mouseHovering:
            return self.pen

        pen = fn.mkPen(255, 255, 0)
        pen.setWidthF(3)
        return pen

    def raiseContextMenu(self, ev):
        menu = self.getMenu()
        pos = ev.screenPos()
        menu.popup(QPoint(int(pos.x()), int(pos.y())))

    def getMenu(self):
        if self.menu is not None:
            self.menu.destroy()

        parent = self.parent()
        self.menu = menu = QMenu(parent)
        menu.setTitle("ROI")
        # Label action
        label = QLabel(self.name or "Region of Interest")
        label.setMargin(2)
        label.setAlignment(Qt.AlignCenter)
        label_action = QWidgetAction(menu)
        label_action.setDefaultWidget(label)
        menu.addAction(label_action)
        menu.addSeparator()
        # Remove action
        remove_action = QAction("Remove", menu)
        remove_action.triggered.connect(self.removeClicked)
        remove_action.setIcon(icons.delete)
        menu.addAction(remove_action)
        # Configure action
        configure_action = QAction("Configure", menu)
        configure_action.triggered.connect(self._configure_roi)
        configure_action.setIcon(icons.edit)
        menu.addAction(configure_action)

        return menu

    def mouseDragEvent(self, ev):
        if ev.isStart():
            if ev.button() == Qt.LeftButton:
                # NOTE: We disable the autorange on ROI movement to make sure
                # it does not get out of bounds!
                self.getViewBox().disableAutoRange()
                self.setSelected(True)
                self.setCursor(Qt.ClosedHandCursor)
                self.sigRegionChangeStarted.emit(self)
                if self.translatable:
                    self.isMoving = True
                    self.preMoveState = self.getState()
                    self.cursorOffset = self.pos() - self.mapToParent(
                        ev.buttonDownPos())
                    ev.accept()
                else:
                    ev.ignore()

        elif ev.isFinish():
            if self.translatable:
                if self.isMoving:
                    self.stateChangeFinished()
                self.isMoving = False
            self.setCursor(Qt.OpenHandCursor)
            return

        if (self.translatable and self.isMoving
                and ev.buttons() == Qt.LeftButton):
            snap = True if (ev.modifiers() & Qt.ControlModifier) else None
            newPos = self.mapToParent(ev.pos()) + self.cursorOffset
            self.translate(newPos - self.pos(), snap=snap, finish=False)

    def movePoint(self, handle, pos, modifiers=None, finish=True,
                  coords='parent'):
        # pos is the new position of the handle in scene coords,
        # as requested by the handle.
        if modifiers is None:
            modifiers = Qt.NoModifier
        newState = self.stateCopy()
        index = self.indexOfHandle(handle)
        h = self.handles[index]
        p0 = self.mapToParent(h['pos'] * self.state['size'])
        p1 = Point(pos)

        if coords == 'parent':
            pass
        elif coords == 'scene':
            p1 = self.mapSceneToParent(p1)
        else:
            raise Exception(
                "New point location must be given in either 'parent' or"
                " 'scene' coordinates.")

        if 'center' in h:
            c = h['center']
            cs = c * self.state['size']
            lp0 = self.mapFromParent(p0) - cs
            lp1 = self.mapFromParent(p1) - cs

        if h['type'] == 't':
            snap = True if (modifiers & Qt.ControlModifier) else None
            self.translate(p1 - p0, snap=snap, update=False)

        elif h['type'] == 'f':
            newPos = self.mapFromParent(p1)
            h['item'].setPos(newPos)
            h['pos'] = newPos
            self.freeHandleMoved = True

        elif h['type'] == 's':
            # If a handle and its center have the same x or y value,
            # we can't scale across that axis.
            if h['center'][0] == h['pos'][0]:
                lp1[0] = 0
            if h['center'][1] == h['pos'][1]:
                lp1[1] = 0

            # ---- Patch ----
            # snap
            if self.scaleSnap or (modifiers & Qt.ControlModifier):
                lp1[0] = np.floor(lp1[0] / self._scaling[0]) * self._scaling[0]
                lp1[1] = np.floor(lp1[1] / self._scaling[1]) * self._scaling[1]

            # ---- Patch End ----

            # preserve aspect ratio (this can override snapping)
            if h['lockAspect'] or (modifiers & Qt.AltModifier):
                # arv = Point(self.preMoveState['size']) -
                lp1 = lp1.proj(lp0)

            # determine scale factors and new size of ROI
            hs = h['pos'] - c
            if hs[0] == 0:
                hs[0] = 1
            if hs[1] == 0:
                hs[1] = 1
            newSize = lp1 / hs

            # Perform some corrections and limit checks
            if newSize[0] == 0:
                newSize[0] = newState['size'][0]
            if newSize[1] == 0:
                newSize[1] = newState['size'][1]
            if not self.invertible:
                if newSize[0] < 0:
                    newSize[0] = newState['size'][0]
                if newSize[1] < 0:
                    newSize[1] = newState['size'][1]
            if self.aspectLocked:
                newSize[0] = newSize[1]

            # Move ROI so the center point occupies the same scene location
            # after the scale
            s0 = c * self.state['size']
            s1 = c * newSize
            cc = self.mapToParent(s0 - s1) - self.mapToParent(Point(0, 0))

            # update state, do more boundary checks
            newState['size'] = newSize
            newState['pos'] = newState['pos'] + cc
            if self.maxBounds is not None:
                r = self.stateRect(newState)
                if not self.maxBounds.contains(r):
                    return

            self.setPos(newState['pos'], update=False)
            self.setSize(newState['size'], update=False)

        elif h['type'] in ['r', 'rf']:
            if h['type'] == 'rf':
                self.freeHandleMoved = True

            if not self.rotatable:
                return
            # If the handle is directly over its center point, we can't
            # compute an angle.
            try:
                if lp1.length() == 0 or lp0.length() == 0:
                    return
            except OverflowError:
                return

            # determine new rotation angle, constrained if necessary
            ang = newState['angle'] - lp0.angle(lp1)
            if ang is None:  # this should never happen..
                return
            if self.rotateSnap or (modifiers & Qt.ControlModifier):
                ang = round(ang / self.rotateSnapAngle) * self.rotateSnapAngle

            # create rotation transform
            tr = QTransform()
            tr.rotate(ang)

            # move ROI so that center point remains stationary after rotate
            cc = self.mapToParent(cs) - (tr.map(cs) + self.state['pos'])
            newState['angle'] = ang
            newState['pos'] = newState['pos'] + cc

            # check boundaries, update
            if self.maxBounds is not None:
                r = self.stateRect(newState)
                if not self.maxBounds.contains(r):
                    return
            self.setPos(newState['pos'], update=False)
            self.setAngle(ang, update=False)

            # If this is a free-rotate handle, its distance from the center
            # may change.

            if h['type'] == 'rf':
                h['item'].setPos(self.mapFromScene(
                    p1))  # changes ROI coordinates of handle
                h['pos'] = self.mapFromParent(p1)

        elif h['type'] == 'sr':
            try:
                if lp1.length() == 0 or lp0.length() == 0:
                    return
            except OverflowError:
                return

            ang = newState['angle'] - lp0.angle(lp1)
            if ang is None:
                return
            if self.rotateSnap or (modifiers & Qt.ControlModifier):
                ang = round(ang / self.rotateSnapAngle) * self.rotateSnapAngle

            # --- Patch ---

            if self.aspectLocked or h['center'][0] != h['pos'][0]:
                newState['size'][0] = self.state['size'][
                                          0] * lp1.length() / lp0.length()
                if self.scaleSnap:  # use CTRL only for angular snap here.
                    newState['size'][0] = (
                            np.floor(newState['size'][0]
                                     / self._scaling[0])
                            * self._scaling[0])

            if self.aspectLocked or h['center'][1] != h['pos'][1]:
                newState['size'][1] = self.state['size'][
                                          1] * lp1.length() / lp0.length()
                if self.scaleSnap:  # use CTRL only for angular snap here.
                    newState['size'][1] = (
                            np.floor(newState['size'][1]
                                     / self._scaling[1])
                            * self._scaling[0])

            # --- Patch End ---

            if newState['size'][0] == 0:
                newState['size'][0] = 1
            if newState['size'][1] == 0:
                newState['size'][1] = 1

            c1 = c * newState['size']
            tr = QTransform()
            tr.rotate(ang)

            cc = self.mapToParent(cs) - (tr.map(c1) + self.state['pos'])
            newState['angle'] = ang
            newState['pos'] = newState['pos'] + cc
            if self.maxBounds is not None:
                r = self.stateRect(newState)
                if not self.maxBounds.contains(r):
                    return

            self.setState(newState, update=False)

        self.stateChanged(finish=finish)

    # ---------------------------------------------------------------------
    # Private methods

    @Slot()
    def _configure_roi(self):
        text, ok = QInputDialog.getText(None, 'Configure ROI', 'Name:',
                                        text=self.name)
        if ok:
            self.name = text
            self._change_roi_text_item_details()
