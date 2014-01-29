#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 21, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a QGraphicsView."""

__all__ = ["GraphicsView"]
    

from displaycomponent import DisplayComponent

from enums import NavigationItemTypes
from enums import ConfigChangeTypes

from layoutcomponents.arrow import Arrow
from editableapplylatercomponent import EditableApplyLaterComponent
from editablenoapplycomponent import EditableNoApplyComponent
from layoutcomponents.graphicscustomitem import GraphicsCustomItem
from layoutcomponents.graphicsproxywidgetcontainer import GraphicsProxyWidgetContainer
from layoutcomponents.link import Link
from layoutcomponents.linkbase import LinkBase
from layoutcomponents.nodebase import NodeBase
from layoutcomponents.text import Text
from layoutcomponents.textdialog import TextDialog

from registry import Loadable, Registry, ns_karabo, ns_svg
from manager import Manager

from widget import DisplayWidget, EditableWidget

from PyQt4.QtCore import (Qt, QByteArray, QDir, QEvent, QSize, QRect, QLine,
    QFileInfo, QBuffer, QIODevice, pyqtSlot, QMimeData)
from PyQt4.QtGui import (QAction, QApplication, QBoxLayout, QBrush, QColor,
                         QGridLayout, QFileDialog, QIcon, QLabel, QLayout,
                         QKeySequence, QMenu, QPainter, QPen,
                         QStackedWidget, QStackedLayout, QWidget)
from PyQt4.QtSvg import QSvgWidget

from xml.etree import ElementTree
from functools import partial
import os.path
from bisect import bisect

class Action(Registry):
    actions = [ ]


    @classmethod
    def register(cls, name, dict):
        super(Action, cls).register(name, dict)
        if "text" in dict:
            cls.actions.append(cls)


    @classmethod
    def add_action(cls, source, parent):
        action = QAction(QIcon(cls.icon), cls.text, source)
        action.setStatusTip(cls.text)
        action.setToolTip(cls.text)
        if hasattr(cls, "shortcut"):
            action.setShortcut(cls.shortcut)
            action.setShortcutContext(Qt.WidgetWithChildrenShortcut)
        return action


class Separator(Action):
    def __init__(self):
        Action.actions.append(self)


    @classmethod
    def add_action(cls, source, parent):
        action = QAction(source)
        action.setSeparator(True)
        return action


class SimpleAction(Action):
    def __init__(self, parent):
        self.parent = parent


    @classmethod
    def add_action(cls, source, parent):
        action = super(SimpleAction, cls).add_action(source, parent)
        o = cls(parent)
        parent.simple_actions.append(o)
        action.triggered.connect(o.run)
        return action


class ActionGroup(Action):
    actions = [ ]


    @classmethod
    def register(cls, name, dict):
        if ActionGroup in cls.__bases__:
            Action.actions.append(cls)
        else:
            super(ActionGroup, cls).register(name, dict)

    @classmethod
    def add_action(cls, source, parent):
        action = super(ActionGroup, cls).add_action(source, parent)
        menu = QMenu(parent)
        for a in cls.actions:
            menu.addAction(super(ActionGroup, a).add_action(source, parent))
        action.setMenu(menu)
        return action


class ShapeAction(Action):
    def __init__(self):
        super(ShapeAction, self).__init__()


    @classmethod
    def add_action(cls, source, parent):
        action = super(ShapeAction, cls).add_action(source, parent)
        action.setCheckable(True)
        obj = ShapeAction()
        obj.action = action
        obj.Shape = cls
        action.triggered.connect(partial(parent.set_current_action, obj))
        return action


    def mousePressEvent(self, parent, event):
        self.start_pos = event.pos()
        self.shape = self.Shape()
        self.shape.set_points(self.start_pos, self.start_pos)
        event.accept()


    def mouseMoveEvent(self, parent, event):
        if hasattr(self, 'shape'):
            self.shape.set_points(self.start_pos, event.pos())
            event.accept()
            parent.update()


    def mouseReleaseEvent(self, parent, event):
        if hasattr(self, 'shape'):
            parent.set_current_action(None)
            parent.ilayout.shapes.append(self.shape)


    def draw(self, painter):
        if hasattr(self, 'shape'):
            self.shape.draw(painter)


class Shape(ShapeAction, Loadable):
    fuzzy = 3


    def __init__(self):
        super(Shape, self).__init__()
        self.selected = False
        self.pen = QPen()
        self.pen.setWidth(1)
        self.brush = QBrush()


    @classmethod
    def add_action(cls, source, parent):
        return super(Shape, cls).add_action(source, parent)


    def loadpen(self, e):
        def ununit(x):
            try:
                f = {"px": 1, "pt": 1.25, "pc": 15, "mm": 3.543307,
                     "cm": 35.43307, "in": 90}[x[-2:]]
                return f * float(x[:-2])
            except KeyError:
                return float(x)

        d = e.attrib.copy()
        if "style" in d:
            d.update(s.split(":") for s in d["style"].split(";"))
        pen = QPen()
        c = QColor(d.get("stroke", "black"))
        c.setAlphaF(float(d.get("stroke-opacity", 1)))
        pen.setColor(c)
        pen.setCapStyle(dict(
            butt=Qt.FlatCap, square=Qt.SquareCap, round=Qt.RoundCap)
            [d.get("stroke-linecap", "butt")])
        s = d.get("fill", "black")
        if s == "none":
            self.brush = QBrush()
        else:
            c = QColor(s)
            c.setAlphaF(float(d.get("fill-opacity", 1)))
            self.brush = QBrush(c)
        pen.setDashOffset(ununit(d.get("stroke-dashoffset", "0")))
        s = d.get("stroke-dasharray", "none")
        if s != "none":
            v = s.split(",") if "," in s else s.split()
            pen.setDashPattern([ununit(s) for s in v])
        pen.setJoinStyle(dict(
            miter=Qt.SvgMiterJoin, round=Qt.RoundJoin, bevel=Qt.BevelJoin)
            [d.get("storke-linejoin", "miter")])
        pen.setMiterLimit(float(d.get("stroke-miterlimit", 4)))
        pen.setWidthF(ununit(d.get("stroke-width", "1")))
        self.pen = pen


    def savepen(self, e):
        d = e.attrib
        d["stroke"] = "#{:06x}".format(self.pen.color().rgb() & 0xffffff)
        d["stroke-opacity"] = unicode(self.pen.color().alphaF())
        d["stroke-linecap"] = {Qt.FlatCap: "butt", Qt.SquareCap: "square",
                               Qt.RoundCap: "round"}[self.pen.capStyle()]
        if self.brush.style() == Qt.SolidPattern:
            d["fill"] = "#{:06x}".format(self.brush.color().rgb() & 0xffffff)
            d["fill-opacity"] = unicode(self.brush.color().alphaF())
        else:
            d["fill"] = "none"
        d["stroke-dashoffset"] = unicode(self.pen.dashOffset())
        d["stroke-dasharray"] = " ".join(unicode(x)
                                         for x in self.pen.dashPattern())
        d["stroke-linejoin"] = {
            Qt.SvgMiterJoin: "miter", Qt.MiterJoin: "miter",
            Qt.BevelJoin: "bevel", Qt.RoundJoin: "round"}[self.pen.joinStyle()]
        d["stroke-miterlimit"] = unicode(self.pen.miterLimit())
        d["stroke-width"] = unicode(self.pen.widthF())


    def draw(self, painter):
        if self.selected:
            black = QPen(Qt.black)
            black.setStyle(Qt.DashLine)
            white = QPen(Qt.white)
            painter.setPen(white)
            painter.drawRect(self.geometry())
            painter.setPen(black)
            painter.drawRect(self.geometry())

class Select(Action):
    """ This is the default action. It has no icon nor text since
    it is selected if nothing else is selected. """

    def __init__(self):
        self.selection_start = self.moving_item = None

    def mousePressEvent(self, parent, event):
        item = parent.ilayout.itemAtPosition(event.pos())
        if item is None:
            for s in parent.ilayout.shapes:
                if s.contains(event.pos()):
                    item = s
                    break
        elif item.selected:
            g = item.geometry()
            p = event.pos()
            if p.x() - g.left() < 10:
                self.resize = "left"
            elif g.right() - p.x() < 10:
                self.resize = "right"
            elif p.y() - g.top() < 10:
                self.resize = "top"
            elif g.bottom() - p.y() < 10:
                self.resize = "bottom"
            else:
                self.resize = None
            if self.resize is not None:
                self.resize_item = item
                return
        if item is None:
            self.selection_stop = self.selection_start = event.pos()
            parent.update()
        else:
            self.moving_item = item
            self.moving_pos = event.pos()
            if event.modifiers() & Qt.ShiftModifier:
                item.selected = not item.selected
            else:
                parent.clear_selection()
                item.selected = True
            parent.update()
            event.accept()

    def mouseMoveEvent(self, parent, event):
        if self.moving_item is not None:
            self.moving_item.translate(event.pos() - self.moving_pos)
            self.moving_pos = event.pos()
            event.accept()
        elif self.selection_start is not None:
            self.selection_stop = event.pos()
            event.accept()
        elif self.resize is not None:
            g = QRect(self.resize_item.fixed_geometry)
            if self.resize == "top":
                g.setTop(event.pos().y())
            elif self.resize == "bottom":
                g.setBottom(event.pos().y())
            elif self.resize == "left":
                g.setLeft(event.pos().x())
            elif self.resize == "right":
                g.setRight(event.pos().x())
            min = self.resize_item.minimumSize()
            max = self.resize_item.maximumSize()
            if (not min.width() < g.size().width() < max.width() or
                not min.height() < g.size().height() < max.height()):
                return
            self.resize_item.fixed_geometry = g
            parent.ilayout.update()
        parent.update()

    def mouseReleaseEvent(self, parent, event):
        self.moving_item = None
        self.resize = self.resize_item = None
        if self.selection_start is not None:
            rect = QRect(self.selection_start, self.selection_stop)
            if not event.modifiers() & Qt.ShiftModifier:
                parent.clear_selection()
            for c in parent.ilayout:
                if rect.contains(c.geometry()):
                    c.selected = True
            self.selection_start = None
            event.accept()
            parent.update()

    def draw(self, painter):
        if self.selection_start is not None:
            painter.setPen(Qt.black)
            painter.drawRect(QRect(self.selection_start, self.selection_stop))


class Line(Shape):
    xmltag = ns_svg + "line"
    text = "Add Line"
    icon = ":line"

    def set_points(self, start, end):
        self.line = QLine(start, end)

    def draw(self, painter):
        painter.setPen(self.pen)
        painter.drawLine(self.line)
        Shape.draw(self, painter)

    def contains(self, p):
        x1, x2 = self.line.x1(), self.line.x2()
        y1, y2 = self.line.y1(), self.line.y2()
        return (min(x1, x2) - self.fuzzy < p.x() < max(x1, x2) + self.fuzzy and
                min(y1, y2) - self.fuzzy < p.y() < max(y1, y2) + self.fuzzy and
                ((x2 - x1) * (p.y() - y1) - (y2 - y1) * (p.x() - x1)) ** 2 <
                self.fuzzy ** 2 * ((x1 - x2) ** 2 + (y1 - y2) ** 2))

    def geometry(self):
        return QRect(self.line.p1(), self.line.p2())

    def translate(self, p):
        self.line.translate(p)

    def element(self):
        ret = ElementTree.Element(
            ns_svg + "line", x1=unicode(self.line.x1()),
            x2=unicode(self.line.x2()), y1=unicode(self.line.y1()),
            y2=unicode(self.line.y2()))
        self.savepen(ret)
        return ret

    @staticmethod
    def load(e, layout):
        ret = Line()
        ret.line = QLine(float(e.get("x1")), float(e.get("y1")),
                         float(e.get("x2")), float(e.get("y2")))
        ret.loadpen(e)
        return ret


class Rectangle(Shape):
    xmltag = ns_svg + "rect"
    text = "Add rectangle"
    icon = ":rect"

    def set_points(self, start, end):
        self.rect = QRect(start, end).normalized()

    def draw(self, painter):
        painter.setPen(self.pen)
        painter.setBrush(self.brush)
        painter.drawRect(self.rect)
        Shape.draw(self, painter)

    def element(self):
        ret = ElementTree.Element(
            ns_svg + "rect", x=unicode(self.rect.x()),
            y=unicode(self.rect.y()), width=unicode(self.rect.width()),
            height=unicode(self.rect.height()))
        self.savepen(ret)
        return ret

    def contains(self, p):
        l, r = self.rect.left(), self.rect.right()
        t, b = self.rect.top(), self.rect.bottom()
        x, y = p.x(), p.y()
        return (l < x < r and min(abs(t - y), abs(b - y)) < self.fuzzy or
                t < y < b and min(abs(l - x), abs(r - x)) < self.fuzzy)

    def geometry(self):
        return self.rect

    def translate(self, p):
        self.rect.translate(p)

    @staticmethod
    def load(e, layout):
        ret = Rectangle()
        ret.rect = QRect(float(e.get("x")), float(e.get("y")),
                         float(e.get("width")), float(e.get("height")))
        ret.loadpen(e)
        return ret

def _parse_rect(elem):
    if elem.get(ns_karabo + "x") is not None:
        ns = ns_karabo
    else:
        ns = ""
    return QRect(float(elem.get(ns + "x")), float(elem.get(ns + "y")),
                 float(elem.get(ns + "width")), float(elem.get(ns + "height")))


Separator()

class GroupActions(ActionGroup):
    icon = ":group"
    text = "Group"

class GroupAction(SimpleAction):
    "Group several items into one group"

    def gather_widgets(self):
        i = 0
        rect = None
        l = [ ]
        while i < len(self.parent.ilayout):
            c = self.parent.ilayout[i]
            if c.selected:
                c.selected = False
                l.append(c)
                if isinstance(c, Layout):
                    del self.parent.ilayout[i]
                    i -= 1
                if rect is None:
                    rect = c.geometry()
                else:
                    rect = rect.united(c.geometry())
            i += 1
        return rect, l


    def gather_shapes(self):
        shapes = self.parent.ilayout.shapes
        self.parent.ilayout.shapes = [s for s in shapes if not s.selected]
        shapes = [s for s in shapes if s.selected]
        for s in shapes:
            s.selected = False
        return shapes


class BoxGroup(GroupAction):
    def doit(self, group, cmp):
        rect, widgets = self.gather_widgets()
        if rect is None:
            return
        widgets.sort(cmp)
        for w in widgets:
            if isinstance(w, Layout):
                group.addItem(w)
            else:
                group.addWidget(w)
        group.shapes = self.gather_shapes()
        group.fixed_geometry = QRect(rect.topLeft(), group.sizeHint())
        self.parent.ilayout.add_item(group)


class VerticalGroup(GroupActions, BoxGroup):
    text = "Group Vertically"
    icon = "icons/group-vertical.svg"


    def run(self):
        self.doit(BoxLayout(BoxLayout.TopToBottom),
                  lambda x, y: x.geometry().y() - y.geometry().y())


class HorizontalGroup(GroupActions, BoxGroup):
    text = "Group Horizontally"
    icon = "icons/group-horizontal.svg"


    def run(self):
        self.doit(BoxLayout(BoxLayout.LeftToRight),
                  lambda x, y: x.geometry().x() - y.geometry().x())


class GridGroup(GroupActions, BoxGroup):
    text = "Group in a Grid"
    icon = "icons/group-grid.svg"


    def run(self):
        rect, widgets = self.gather_widgets()
        if rect is None:
            return
        group = GridLayout()
        group.set_children(widgets)
        group.shapes = self.gather_shapes()
        group.fixed_geometry = QRect(rect.topLeft(), group.sizeHint())
        self.parent.ilayout.add_item(group)


class Ungroup(GroupActions, SimpleAction):
    "Ungroup items"
    text = "Ungroup"
    icon = "icons/ungroup.svg"


    def run(self):
        i = 0
        while i < len(self.parent.ilayout):
            child = self.parent.ilayout[i]
            if child.selected and isinstance(child, Layout):
                cl = list(child)
                for c in cl:
                    if isinstance(c, QLayout):
                        c.setParent(None)
                    c.fixed_geometry = c.geometry()
                self.parent.ilayout[i:i + 1] = cl
                self.parent.ilayout.shapes.extend(child.shapes)
                i += len(cl)
            else:
                i += 1


Separator()


class Cut(SimpleAction):
    text = "Cut"
    icon = ":edit-cut"
    shortcut = QKeySequence.Cut


    def run(self):
        QApplication.clipboard().setMimeData(self.parent.mimeData())
        self.parent.ilayout.delete_selected()
        self.parent.update()


class Copy(SimpleAction):
    text = "Copy"
    icon = ":edit-copy"
    shortcut = QKeySequence.Copy


    def run(self):
        QApplication.clipboard().setMimeData(self.parent.mimeData())


class Paste(SimpleAction):
    text = "Paste"
    icon = ":edit-paste"
    shortcut = QKeySequence.Paste


    def run(self):
        root = ElementTree.fromstring(QApplication.clipboard().mimeData().
                                      data("image/svg+xml"))
        self.parent.ilayout.load_element(root)
        self.parent.tree.getroot().extend(root)
        ar = QByteArray()
        buf = QBuffer(ar)
        buf.open(QIODevice.WriteOnly)
        self.parent.tree.write(buf)
        buf.close()
        self.parent.load(ar)
        self.parent.update()


class Layout(Loadable):
    xmltag = ns_svg + "g"
    subclasses = { }


    def __init__(self):
        self.shapes = [ ]
        self.shape_geometry = None
        self.selected = False


    def __len__(self):
        return self.count()


    def __getitem__(self, i):
        if isinstance(i, slice):
            r = (self.itemAt(j) for j in range(i.start, i.stop, i.step))
            return [rr.widget() if rr.widget() else rr for rr in r
                    if rr is not None]
        r = self.itemAt(i)
        if r is None:
            raise IndexError("index out of range")
        return r.widget() if r.widget() else r


    def __delitem__(self, i):
        if isinstance(i, slice):
            for j in range(i.start, i.stop, i.step):
                self.takeAt(j)
        else:
            self.takeAt(i)


    def load_element(self, element):
        i = 0
        while i < len(element):
            elem = element[i]
            r = Loadable.load(elem, self)
            if r is None:
                i += 1
            elif isinstance(r, Shape):
                self.shapes.append(r)
                del element[i]
            else:
                if not isinstance(r, Layout):
                    w = ProxyWidget(self.widget())
                    if isinstance(r, QLabel):
                        w.set_child(r, None)
                    else:
                        w.set_child(r.widget, r)
                    self.load_item(elem, w)
                r.fixed_geometry = _parse_rect(elem)
                del element[i]


    def draw(self, painter):
        for s in self.shapes:
            painter.save()
            s.draw(painter)
            painter.restore()
        for i in range(self.count()):
            item = self.itemAt(i)
            if isinstance(item, Layout):
                item.draw(painter)


    def element(self, selected=False):
        """ save this layout to an element. if selected is True,
        only selected elements are saved, for cut&paste support """
        g = self.geometry()
        d = { ns_karabo + "x": g.x(), ns_karabo + "y": g.y(),
              ns_karabo + "width": g.width(), ns_karabo + "height": g.height(),
              ns_karabo + "class": self.__class__.__name__ }
        d.update(self.save())

        e = ElementTree.Element(ns_svg + "g",
                                {k: unicode(v) for k, v in d.iteritems()})
        if selected:
            self.add_children(e, True)
        else:
            self.add_children(e)
        return e


    def add_children(self, e, selected=False):
        e.extend(e.element() for e in self if not selected or e.selected)
        e.extend(s.element() for s in self.shapes if not selected or s.selected)


    def translate(self, pos):
        self.fixed_geometry.translate(pos)
        self.update()


    def update_shapes(self, rect):
        if self.shape_geometry is None:
            self.shape_geometry = QRect(self.fixed_geometry)
        for s in self.shapes:
            s.translate(rect.topLeft() - self.shape_geometry.topLeft())
        self.shape_geometry = QRect(rect)


class FixedLayout(Layout, QLayout):
    def __init__(self):
        QLayout.__init__(self)
        Layout.__init__(self)
        self._children = [ ] # contains only QLayoutItems


    def __setitem__(self, key, value):
        if not isinstance(key, slice):
            key = slice(key, key + 1)
            value = [value]
        values = [ ]
        for v in value:
            if isinstance(v, ProxyWidget):
                self.addWidget(v)
                values.append(self._item)
            elif isinstance(v, Layout):
                self.addChildLayout(v)
                values.append(v)
        self._children[key] = values
        self.update()


    def add_item(self, item):
        """add ProxyWidgets or Layouts"""
        self[len(self):len(self)] = [item]


    def load_item(self, element, item):
        self.add_item(item)


    def itemAtPosition(self, pos):
        for item in self._children:
            if item.geometry().contains(pos):
                if item.widget():
                    return item.widget()
                else:
                    return item


    def relayout(self, widget):
        for c in self:
            ll = [c]
            ret = None
            while ll and ret is None:
                l = ll.pop()
                if isinstance(l, Layout):
                    ll.extend(l)
                else:
                    if l is widget:
                        ret = l
        if ret is None:
            return
        c.fixed_geometry = QRect(c.fixed_geometry.topLeft(), c.sizeHint())


    def delete_selected(self):
        i = 0
        while i < len(self):
            if self[i].selected:
                stack = [self[i]]
                while stack:
                    p = stack.pop()
                    if isinstance(p, Layout):
                        stack.extend(p)
                    else:
                        p.setParent(None)
                del self[i]
            else:
                i += 1
        self.shapes = [s for s in self.shapes if not s.selected]


    def save(self):
        return { }


    @staticmethod
    def load(elem, layout):
        ret = FixedLayout()
        if layout is not None:
            layout.load_item(elem, ret)
        ret.load_element(elem)
        return ret


    def addItem(self, item):
        "only to be used by Qt, don't use directly!"
        self._item = item


    def itemAt(self, index):
        "only to be used by Qt, don't use directly!"
        try:
            return self._children[index]
        except IndexError:
            return


    def takeAt(self, index):
        "only to be used by Qt, don't use directly!"
        try:
            return self._children.pop(index)
        except IndexError:
            return


    def count(self):
        "only to be used by Qt, don't use directly!"
        return len(self._children)


    def setGeometry(self, geometry):
        "only to be used by Qt, don't use directly!"
        for item in self._children:
            i = item.widget() if item.widget() else item
            i.setGeometry(i.fixed_geometry)


    def sizeHint(self):
        return QSize(10, 10)


class BoxLayout(QBoxLayout, Layout):
    def __init__(self, dir):
        QBoxLayout.__init__(self, dir)
        Layout.__init__(self)
        self.setContentsMargins(5, 5, 5, 5)


    def save(self):
        return {ns_karabo + "Direction": self.direction()}


    def setGeometry(self, rect):
        QBoxLayout.setGeometry(self, rect)
        self.update_shapes(rect)


    @staticmethod
    def load(elem, layout):
        ret = BoxLayout(int(elem.get(ns_karabo + "Direction")))
        layout.load_item(elem, ret)
        ret.load_element(elem)
        return ret


    def load_item(self, element, item):
        if isinstance(item, ProxyWidget):
            self.addWidget(item)
        else:
            self.addLayout(item)


def _reduce(xs):
    xs.sort()
    i = 1
    while i < len(xs):
        if xs[i] - xs[i - 1] > 10:
            i += 1
        else:
            del xs[i]


class GridLayout(QGridLayout, Layout):
    def __init__(self):
        QGridLayout.__init__(self)
        Layout.__init__(self)

    def set_children(self, children):
        xs = [c.geometry().x() for c in children]
        ys = [c.geometry().y() for c in children]
        _reduce(xs)
        _reduce(ys)
        for c in children:
            col = bisect(xs, c.geometry().x())
            row = bisect(ys, c.geometry().y())
            colspan = bisect(xs, c.geometry().right()) - col + 1
            rowspan = bisect(ys, c.geometry().bottom()) - row + 1
            if isinstance(c, ProxyWidget):
                self.addWidget(c, row, col, rowspan, colspan)
            else:
                self.addLayout(c, row, col, rowspan, colspan)


    def save(self):
        return { }


    def add_children(self, e):
        e.extend(s.element() for s in self.shapes)
        for i in range(len(self)):
            c = self[i].element()
            for n, v in zip(("row", "col", "rowspan", "colspan"),
                            self.getItemPosition(i)):
                c.set(ns_karabo + n, unicode(v))
            e.append(c)


    def load_item(self, element, item):
        p = (int(element.get(ns_karabo + s)) for s in
             ("row", "col", "rowspan", "colspan"))
        if isinstance(item, ProxyWidget):
            self.addWidget(item, *p)
        else:
            self.addLayout(item, *p)


    @staticmethod
    def load(elem, layout):
        ret = GridLayout()
        layout.load_item(elem, ret)
        ret.load_element(elem)
        return ret


    def setGeometry(self, rect):
        QGridLayout.setGeometry(self, rect)
        self.update_shapes(rect)


class Label(Loadable):
    """ Fake class to make a QLabel loadable """
    @staticmethod
    def load(elem, layout):
        return QLabel(elem.get(ns_karabo + "text"))


class ProxyWidget(QStackedWidget):
    def __init__(self, parent):
        QStackedWidget.__init__(self, parent)
        #self.setContextMenuPolicy(Qt.ActionsContextMenu)
        self.selected = False

    def set_child(self, child, component):
        if self.count() > 0:
            self.removeWidget(self.widget(0))
        self.addWidget(child)
        self.component = component
        if component is None:
            return

        component.setParent(self)

        if isinstance(component, DisplayComponent):
            Widget = DisplayWidget
        else:
            Widget = EditableWidget

        for text, factory in Widget.factories.iteritems():
            aliases = factory.getAliasesViaCategory(
                component.widgetCategory)
            keys = component.keys[0].split('.configuration.')
            if keys[1] == "state":
                aliases += factory.getAliasesViaCategory("State")
            if aliases:
                aa = QAction(text, self)
                menu = QMenu(self)
                for a in aliases:
                    menu.addAction(a).triggered.connect(
                        partial(self.on_changeWidget, factory, a))
                aa.setMenu(menu)
                self.addAction(aa)

    @pyqtSlot()
    def on_changeWidget(self, factory, alias):
        self.component.changeWidget(factory, alias)
        self.parent().layout().relayout(self)
        self.adjustSize()
        
    def contextMenuEvent(self, event):
        if not self.parent().parent().designMode:
            return
        QMenu.exec_(self.actions(), event.globalPos(), None, self)

    def element(self):
        g = self.geometry()
        d = { "x": g.x(), "y": g.y(), "width": g.width(), "height": g.height() }
        if self.component is None:
            d[ns_karabo + "class"] = "Label"
            d[ns_karabo + "text"] = self.widget(0).text()
        else:
            d.update(self.component.attributes())
        return ElementTree.Element(ns_svg + "rect",
                                   {k: unicode(v) for k, v in d.iteritems()})


    @staticmethod
    def load(elem, layout):
        if elem.get(ns_karabo + "class") == "Label":
            ret = ProxyWidget(layout.widget())
            ret.set_child(QLabel(elem.get(ns_karabo + "text"), ret), None)
            ret.show()
            return ret
        ks = "classAlias", "key", "widgetFactory"
        if elem.get(ns_karabo + "classAlias") == "Command":
            ks += "command", "allowedStates", "commandText"
        d = {k: elem.get(ns_karabo + k) for k in ks}
        d["commandEnabled"] = elem.get(ns_karabo + "commandEnabled") == "True"
        component = globals()[elem.get(ns_karabo + "componentType")](**d)
        component.widget.setAttribute(Qt.WA_NoSystemBackground, True)
        ret = ProxyWidget(layout.widget())
        ret.set_child(component.widget, component)
        ret.show()
        return ret


    def translate(self, pos):
        self.fixed_geometry.translate(pos)
        self.parent().layout().update()


class GraphicsView(QSvgWidget):
    def __init__(self, parent):
        super(GraphicsView, self).__init__(parent)
        
        self.inner = QWidget(self)
        self.ilayout = FixedLayout()
        self.inner.setLayout(self.ilayout)
        layout = QStackedLayout(self)
        layout.addWidget(self.inner)
        
        self.current_action = self.default_action = Select()
        self.current_action.action = QAction(self) # never displayed
        self.simple_actions = [ ]

        self.tree = ElementTree.ElementTree(ElementTree.Element(ns_svg + "svg"))

        self.designMode = True

        # Describes most recent item to be cut or copied inside the application
        self.__copiedItem = QByteArray()

        self.setFocusPolicy(Qt.StrongFocus)
        self.setAcceptDrops(True)

    def add_actions(self, source):
        for v in Action.actions:
            action = v.add_action(source, self)
            yield action

    def set_current_action(self, action):
        self.current_action.action.setChecked(False)
        if Action is None:
            self.current_action = self.default_action
        else:
            self.current_action = action
        if self.current_action is None:
            self.current_action = self.default_action
        self.current_action.action.setChecked(True)


    @property
    def designMode(self):
        return self.inner.testAttribute(Qt.WA_TransparentForMouseEvents)


    @designMode.setter
    def designMode(self, value):
        self.inner.setAttribute(Qt.WA_TransparentForMouseEvents, value)

    # Returns true, when items has been copied; otherwise false
    def hasCopy(self):
        return (len(self.__copiedItem) > 0)


    # Open saved view from file
    def openSceneLayoutFromFile(self):
        filename = QFileDialog.getOpenFileName(None, "Open saved view",
                                               QDir.tempPath(), "SVG (*.svg)")
        if len(filename) < 1:
            return

        self.openScene(filename)


    def openSceneConfigurationsFromFile(self):
        dirPath = QFileDialog.getExistingDirectory(self, "Select directory to open configuration files", QDir.tempPath(),
                                                   QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)

        if len(dirPath) < 1:
            return

        dir = QDir(dirPath)
        fileInfos = dir.entryInfoList(QDir.NoDotAndDotDot | QDir.Files | QDir.Hidden | QDir.System)
        for fileInfo in fileInfos:
            print fileInfo


    def openSceneLayoutConfigurationsFromFile(self):
        dirPath = QFileDialog.getExistingDirectory(self, "Select directory to open configuration files", QDir.tempPath(),
                                                   QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)

        if len(dirPath) < 1:
            return

        dir = QDir(dirPath)
        fileInfos = dir.entryInfoList(QDir.NoDotAndDotDot | QDir.Files | QDir.Hidden | QDir.System)

        internalKeyTextTuples = []
        for fileInfo in fileInfos:
            if fileInfo.suffix() == "scene":
                # Layout file
                internalKeyTextTuples = self.openScene(str(fileInfo.absoluteFilePath()))
            #elif fileInfo.suffix() == "xml":
                # Configuration file
                #print "XML file:", fileInfo.absoluteFilePath()

        for internalKeyText in internalKeyTextTuples:
            internalKey = str(internalKeyText[0])
            text = str(internalKeyText[1])
            filename = str(dirPath + "/" + text + ".xml")
            # openAsXml(self, filename, internalKey, configChange=ConfigChangeTypes.DEVICE_CLASS_CONFIG_CHANGED, classId=str())

            # TODO: Remove dirty hack for scientific computing again!!!
            croppedClassId = text.split("-")
            classId = croppedClassId[0]
            Manager().openAsXml(filename, internalKey, ConfigChangeTypes.DEVICE_CLASS_CONFIG_CHANGED, classId)


    # Helper function opens *.scene file
    # Returns list of tuples containing (internalKey, text) of GraphicsItem of scene
    def openScene(self, filename):
        self.tree = ElementTree.parse(filename)
        root = self.tree.getroot()
        self.inner.setParent(None)
        self.inner = QWidget(self)
        self.ilayout = FixedLayout.load(root, None)
        self.inner.setLayout(self.ilayout)
        self.layout().addWidget(self.inner)
        self.designMode = True

        ar = QByteArray()
        buf = QBuffer(ar)
        buf.open(QIODevice.WriteOnly)
        self.tree.write(buf)
        buf.close()
        self.load(ar)


    # Helper function opens *.xml configuration file
    def openConfiguration(self, filename):
        print "openConfiguration", filename


    def saveSceneLayoutToFile(self):
        """ Save active view to file """
        filename = QFileDialog.getSaveFileName(None, "Save file as",
                                               QDir.tempPath(), "SVG (*.svg)")
        if len(filename) < 1:
            return
        self.saveScene(filename)

    def saveScene(self, filename):
        fi = QFileInfo(filename)
        if len(fi.suffix()) < 1:
            filename += ".svg"

        root = self.tree.getroot().copy()
        tree = ElementTree.ElementTree(root)
        e = self.ilayout.element()
        root.extend(ee for ee in e)
        tree.write(filename)


    def mimeData(self):
        e = self.ilayout.element(selected=True)
        e.tag = ns_svg + "svg"
        tree = ElementTree.ElementTree(e)
        ar = QByteArray()
        buf = QBuffer(ar)
        buf.open(QIODevice.WriteOnly)
        tree.write(buf)
        buf.close()
        mime = QMimeData()
        mime.setData("image/svg+xml", ar)
        return mime


    def saveSceneConfigurationsToFile(self):
        dirPath = QFileDialog.getExistingDirectory(self, "Select directory to save configuration files", QDir.tempPath(),
                                                   QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)

        if len(dirPath) < 1:
            return

        # Check, if directory is empty
        self.checkDirectoryBeforeSave(dirPath)

        # Save configurations of navigation related items
        self.saveSceneConfigurations(dirPath)


    def saveSceneLayoutConfigurationsToFile(self):
        """ Save active view and configurations to folder/files """
        dir = QFileDialog.getExistingDirectory(
            self, "Select directory to save layout and configuration files",
            options=QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)
        if not dir:
            return
        self.checkDirectoryBeforeSave(dir)
        self.saveScene(os.path.join(dir, os.path.basename(dir) + ".svg"))
        self.saveSceneConfigurations(dir)


    # Helper function checks whether the directory to save to is empty
    # If the directory is not empty the user has to select what happens with the existing files
    def checkDirectoryBeforeSave(self, dirPath):
        dir = QDir(dirPath)
        files = dir.entryList(QDir.NoDotAndDotDot | QDir.Files | QDir.Hidden | QDir.System)
        if len(files) > 0:
            reply = QMessageBox.question(self, 'Selected directory is not empty',
                "The selected directory already contains files.<br>These files will be overwritten or removed.<br><br>" \
                + "Do you want to continue?", QMessageBox.Yes |
                QMessageBox.No, QMessageBox.No)

            if reply == QMessageBox.No:
                return

        for file in files:
            dir.remove(file)


    # Helper function to save all configurations for scene items
    def saveSceneConfigurations(self, dirPath):
        for item in self.ilayout.shapes:
            if isinstance(item, GraphicsCustomItem):
                # TODO: Remove dirty hack for scientific computing again!!!
                croppedClassId = item.text().split("-")
                classId = croppedClassId[0]
                Manager().saveAsXml(str(dirPath + "/" + item.text() + ".xml"), str(classId), str(item.internalKey()))


    # Positions a newly added or pasted item in the scene
    # The sequence number ensures that new items are added in different positions
    # rather than on top of each other
    def _setupItem(self, item):
        item.setPos(QPointF(80 + (100 * (self.__seqNumber % 5)), 80 + (50 * ((self.__seqNumber / 5) % 7))))
        self.__seqNumber += 1
        self._addItem(item)


    # Creates and returns container item
    def createGraphicsItemContainer(self, orientation, items, pos):
        # Initialize layout
        layout = BoxLayout(QBoxLayout.LeftToRight if
                           orientation == Qt.Horizontal else
                           QBoxLayout.TopToBottom)

        for item, component in items:
            proxy = ProxyWidget(self.inner)
            proxy.set_child(item, component)
            layout.addWidget(proxy)
            proxy.show()

        layout.fixed_geometry = QRect(pos, layout.sizeHint())
        self.ilayout.add_item(layout)
        layout.selected = True
        return layout


    def clear_selection(self):
        for s in self.ilayout.shapes:
            s.selected = False
        for c in self.ilayout:
            c.selected = False


    def mousePressEvent(self, event):
        if not self.designMode:
            return
        if event.button() == Qt.LeftButton:
            self.current_action.mousePressEvent(self, event)
        else:
            child = self.inner.childAt(event.pos())
            if child is not None:
                while not isinstance(child, ProxyWidget):
                    child = child.parent()
                child.mousePressEvent(event)

        QWidget.mousePressEvent(self, event)


    def contextMenuEvent(self, event):
        if not self.designMode:
            return
        child = self.inner.childAt(event.pos())
        if child is not None:
            while not isinstance(child, ProxyWidget):
                child = child.parent()
            child.event(event)


    def mouseMoveEvent(self, event):
        self.current_action.mouseMoveEvent(self, event)
        QWidget.mouseMoveEvent(self, event)


    def mouseReleaseEvent(self, event):
        self.current_action.mouseReleaseEvent(self, event)
        QWidget.mouseReleaseEvent(self, event)


    def dragEnterEvent(self, event):
        source = event.source()
        if source is not None and source is not self and self.designMode:
            event.accept()
        QWidget.dragEnterEvent(self, event)


    def dropEvent(self, event):
        #print "GraphicsView.dropEvent"

        source = event.source()
        if source is not None:
            customItem = None
            mimeData = event.mimeData()
            # Source type
            sourceType = mimeData.data("sourceType")
            # Drop from NavigationTreeView or ParameterTreeWidget?
            if sourceType == "NavigationTreeView":
                # Navigation item type
                navItemType = int(mimeData.data("navigationItemType"))
                # Device server instance id
                serverId = mimeData.data("serverId").data()
                # Internal key
                #key = mimeData.data("key").data()
                # Display name
                displayName = mimeData.data("displayName").data()
                
                # Get schema
                schema = None
                path = str("server." + serverId + ".classes." + displayName + ".description")
                if Manager().hash.has(path):
                    schema = Manager().hash.get(path)
                
                configKey, configCount = Manager().createNewConfigKeyAndCount(displayName)
                
                # Create graphical item
                customItem = GraphicsCustomItem(
                    configKey, self.designMode, displayName, schema,
                    (navItemType == NavigationItemTypes.CLASS))
                tooltipText = "<html><b>Associated key: </b>%s</html>" % configKey
                customItem.setToolTip(tooltipText)
                customItem.position = event.pos()
                self.ilayout.shapes.append(customItem)
                self.update()

                # Register as visible device - TODO?
                #Manager().newVisibleDevice(configKey)

                if navItemType and (navItemType == NavigationItemTypes.CLASS):
                    Manager().createNewProjectConfig(customItem, configKey, configCount, displayName, schema)

                    # Connect customItem signal to Manager, DEVICE_CLASS
                    customItem.signalValueChanged.connect(Manager().onDeviceClassValueChanged)
                    # Register for value changes of deviceId
                    Manager().registerEditableComponent(customItem.deviceIdKey, customItem)

            elif sourceType == "ParameterTreeWidget":                
                # Internal key
                internalKey = mimeData.data("internalKey").data()
                # Display name
                displayName = mimeData.data("displayName").data()
                # Display component?
                hasDisplayComponent = mimeData.data(
                    "hasDisplayComponent") == "True"
                # Editable component?
                hasEditableComponent = mimeData.data(
                    "hasEditableComponent") == "True"
                
                # TODO: HACK to get apply button disabled
                currentValue = None
                if hasEditableComponent:
                    currentValue = str(mimeData.data("currentValue"))
                
                metricPrefixSymbol = mimeData.data("metricPrefixSymbol").data()
                unitSymbol = mimeData.data("unitSymbol").data()
                
                enumeration = mimeData.data("enumeration").data()
                if enumeration:
                    enumeration = enumeration.split(",")
                # Navigation item type
                navItemType = int(mimeData.data("navigationItemType"))
                # Class alias
                classAlias = mimeData.data("classAlias").data()

                # List stored all items for layout
                items = []

                displayNameProxyWidget = self._createDisplayNameProxyWidget(displayName)
                if displayNameProxyWidget:
                    # Add item to itemlist
                    items.append((displayNameProxyWidget, None))

                # Does key concern state of device?
                keys = str(internalKey).split('.configuration.')
                isStateToDisplay = keys[1] == "state"

                # Display widget
                if hasDisplayComponent:
                    # Special treatment for command
                    if classAlias == "Command":
                        allowedStates = []
                        displayText = str()
                        commandEnabled = False
                        command = str()
                        parameterItem = source.getParameterTreeWidgetItemByKey(internalKey)
                        if parameterItem:
                            allowedStates = parameterItem.allowedStates
                            displayText = parameterItem.displayText
                            commandEnabled = parameterItem.enabled
                            command = parameterItem.command
                        displayComponent = DisplayComponent(classAlias, key=internalKey, \
                                                            allowedStates=allowedStates, \
                                                            commandText=displayText, \
                                                            commandEnabled=commandEnabled, \
                                                            command=command)
                    else:
                        displayComponent = DisplayComponent(classAlias, key=internalKey, \
                                                            enumeration = enumeration, \
                                                            metricPrefixSymbol=metricPrefixSymbol, \
                                                            unitSymbol=unitSymbol)
                    displayComponent.widget.setToolTip(internalKey)
                    
                    items.append((displayComponent.widget, displayComponent))
                    
                    # Add proxyWidget for unit label, if available
                    unitProxyWidget = self._createUnitProxyWidget(metricPrefixSymbol, unitSymbol)
                    if unitProxyWidget:
                        items.append((unitProxyWidget, None))

                    # Register as visible device
                    Manager().newVisibleDevice(internalKey)

                # Editable widget
                if hasEditableComponent:
                    if navItemType is NavigationItemTypes.CLASS:
                        editableComponent = EditableNoApplyComponent(classAlias, key=internalKey, \
                                                                     enumeration = enumeration, \
                                                                     metricPrefixSymbol=metricPrefixSymbol, \
                                                                     unitSymbol=unitSymbol)
                    elif navItemType is NavigationItemTypes.DEVICE:
                        editableComponent = EditableApplyLaterComponent(classAlias, key=internalKey, \
                                                                        enumeration = enumeration, \
                                                                        metricPrefixSymbol=metricPrefixSymbol, \
                                                                        unitSymbol=unitSymbol)
                        editableComponent.isEditableValueInit = False

                    items.append((editableComponent.widget, editableComponent))

                    # Register as visible device
                    Manager().newVisibleDevice(internalKey)                   

                customItem = self.createGraphicsItemContainer(
                    Qt.Horizontal, items, event.pos())

            if customItem is None:
                return

        event.accept()

        QWidget.dropEvent(self, event)


    def event(self, event):
        ret = QWidget.event(self, event)
        if event.type() == QEvent.ToolTip:
            item = self.inner.childAt(event.pos())
            if item is not None:
                item.event(event)
                return True
        return ret


    def _getWidgetCenterPosition(self, pos, centerX, centerY):
        # QPointF pos, int centerX, int centerY
        pos.setX(pos.x()-centerX)
        pos.setY(pos.y()-centerY)
        return pos


    def _createDisplayNameProxyWidget(self, displayName):
        if len(displayName) < 1:
            return None

        # Label only, if there is something to show
        return QLabel(displayName)


    def _createUnitProxyWidget(self, metricPrefixSymbol, unitSymbol):
        # If metricPrefix &| unitSymbo are set a QLabel is returned,
        # otherwise None
        unitLabel = str()
        
        if len(metricPrefixSymbol) > 0:
            unitLabel += metricPrefixSymbol
        if len(unitSymbol) > 0:
            unitLabel += unitSymbol
        
        if len(unitLabel) > 0:
            laUnit = QLabel(unitLabel)
            return laUnit
        
        return None


    def paintEvent(self, event):
        painter = QPainter(self)
        try:
            self.renderer().render(painter, self.renderer().viewBoxF())
            self.ilayout.draw(painter)
            painter.save()
            if self.designMode:
                painter.setPen(Qt.DashLine)
                for item in self.ilayout:
                    if item.selected:
                        painter.drawRect(item.geometry())
            painter.restore()
            self.current_action.draw(painter)
        finally:
            painter.end()
