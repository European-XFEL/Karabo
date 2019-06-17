import numpy as np
from PyQt4.QtCore import Qt, QRectF, pyqtSlot, QPoint
from PyQt4.QtGui import QMenu, QAction, QTransform
from pyqtgraph import ROI, TextItem, functions as fn, Point

from karabogui.graph.common.utils import float_to_string

from .utils import ROI_CENTER_HTML, ROI_CENTER_SIZE_HTML


class KaraboROI(ROI):

    def __init__(self, pos, size=Point(1, 1),
                 scale_snap=False, translate_snap=False, pen=None):
        super(KaraboROI, self).__init__(pos, size,
                                        scaleSnap=scale_snap,
                                        translateSnap=translate_snap,
                                        pen=pen,
                                        removable=True)
        self.setZValue(100)
        self._scaling = np.array([1, 1])
        self._translation = np.array([0, 0])

        self._add_handles()

        self.textItem = TextItem(html=ROI_CENTER_HTML.format(0, 0),
                                 fill=(0, 0, 0, 50))
        self.textItem.setZValue(99)
        self._change_roi_text_item_details()

        self.sigRegionChanged.connect(self._change_roi_text_item_details)
        self.setAcceptedMouseButtons(Qt.RightButton | Qt.LeftButton)
        self.set_visible(False)

    def _add_handles(self):
        pass

    def mouseClickEvent(self, event):
        super(KaraboROI, self).mouseClickEvent(event)

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
            self.textItem.setVisible(visible)

    def select(self, update=True):
        self.pen.setWidthF(2)
        self.currentPen.setWidthF(2)
        self._set_handle_visible(True)
        if update:
            self.update()

    def unselect(self, update=True):
        self.pen.setWidthF(1)
        self.currentPen.setWidthF(1)
        self._set_handle_visible(False)
        if update:
            self.update()

    def _set_handle_visible(self, visible):
        for handle in self.handles:
            handle["item"].setVisible(visible)

    # ---------------------------------------------------------------------
    # PyQt slots

    @pyqtSlot()
    def _change_roi_text_item_details(self):
        if not self.textItem:
            return

        direction = np.array([0, 1])  # lower left
        self.textItem.setPos(*(self.pos() + (self.size() * direction)))
        center = [float_to_string(coord) for coord in self.center]
        size = [float_to_string(s) for s in self.size()]
        self.textItem.setHtml(ROI_CENTER_SIZE_HTML.format(*center, *size))

    # ---------------------------------------------------------------------
    # Properties

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
        return (*self._absolute_position, *self._absolute_size)

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
        menu.popup(QPoint(pos.x(), pos.y()))

    def getMenu(self):
        if self.menu is None:
            menu = QMenu()
            menu.setTitle("ROI")
            remove_action = QAction("Remove ROI", menu)
            remove_action.triggered.connect(self.removeClicked)
            menu.addAction(remove_action)
            self.menu = menu
        return self.menu

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

    def viewRangeChanged(self):
        super(KaraboROI, self).viewRangeChanged()
        self.maxBounds = self.getViewBox().viewRect()

    def movePoint(self, handle, pos, modifiers=Qt.KeyboardModifier(),
                  finish=True, coords='parent'):
        """Patching to support snapping based on scaling"""
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
            raise Exception("New point location must be given in either "
                            "'parent' or 'scene' coordinates.")

        # Handles with a 'center' need to know their local position
        # relative to the center point (lp0, lp1)
        if 'center' in h:
            c = h['center']
            cs = c * self.state['size']
            lp0 = self.mapFromParent(p0) - cs
            lp1 = self.mapFromParent(p1) - cs

        if h['type'] == 't':
            snap = True if (modifiers & Qt.ControlModifier) else None
            # if self.translateSnap or ():
            # snap = Point(self.snapSize, self.snapSize)
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

            # snap
            if self.scaleSnap or (modifiers & Qt.ControlModifier):
                lp1[0] = np.floor(lp1[0] / self._scaling[0]) * self._scaling[0]
                lp1[1] = np.floor(lp1[1] / self._scaling[1]) * self._scaling[1]

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

        elif h['type'] == 'sr':
            if h['center'][0] == h['pos'][0]:
                scaleAxis = 1
                nonScaleAxis = 0
            else:
                scaleAxis = 0
                nonScaleAxis = 1

            try:
                if lp1.length() == 0 or lp0.length() == 0:
                    return
            except OverflowError:
                return

            ang = newState['angle'] - lp0.angle(lp1)
            if ang is None:
                return
            if self.rotateSnap or (modifiers & Qt.ControlModifier):
                # ang = round(ang / (np.pi/12.)) * (np.pi/12.)
                ang = round(ang / 15.) * 15.

            hs = abs(h['pos'][scaleAxis] - c[scaleAxis])
            newState['size'][scaleAxis] = lp1.length() / hs

            # if self.scaleSnap or (modifiers & Qt.ControlModifier):
            if self.scaleSnap:  # use CTRL only for angular snap here.
                newState['size'][scaleAxis] = (
                        np.floor(newState['size'][scaleAxis]
                                 / self._scaling[scaleAxis])
                        * self._scaling[scaleAxis])
            if newState['size'][scaleAxis] == 0:
                newState['size'][scaleAxis] = 1
            if self.aspectLocked:
                newState['size'][nonScaleAxis] = newState['size'][scaleAxis]

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

    def getArraySlice(self, data, img, axes=(0, 1), returnSlice=True):
        """ Patching to fix bug in rounded off bounds """

        # Determine shape of array along ROI axes
        dShape = (data.shape[axes[0]], data.shape[axes[1]])

        # Determine transform that maps ROI bounding box to image coordinates
        try:
            tr = (self.sceneTransform()
                  * fn.invertQTransform(img.sceneTransform()))
        except np.linalg.linalg.LinAlgError:
            return None

        # Modify transform to scale from image coords to data coords
        axisOrder = img.axisOrder
        if axisOrder == 'row-major':
            tr.scale(float(dShape[1]) / img.width(),
                     float(dShape[0]) / img.height())
        else:
            tr.scale(float(dShape[0]) / img.width(),
                     float(dShape[1]) / img.height())

        # Transform ROI bounds into data bounds
        dataBounds = tr.mapRect(self.boundingRect())

        # Intersect transformed ROI bounds with data bounds
        if axisOrder == 'row-major':
            intBounds = dataBounds.intersected(
                QRectF(0, 0, dShape[1], dShape[0]))
        else:
            intBounds = dataBounds.intersected(
                QRectF(0, 0, dShape[0], dShape[1]))

        # Determine index values to use when referencing the array.
        bounds = (
            (round(min(intBounds.left(), intBounds.right())),
             round(1 + max(intBounds.left(), intBounds.right()))),
            (round(min(intBounds.bottom(), intBounds.top())),
             round(1 + max(intBounds.bottom(), intBounds.top())))
        )
        if axisOrder == 'row-major':
            bounds = bounds[::-1]

        if returnSlice:
            # Create slice objects
            sl = [slice(None)] * data.ndim
            sl[axes[0]] = slice(*bounds[0])
            sl[axes[1]] = slice(*bounds[1])
            return tuple(sl), tr
        else:
            return bounds, tr
