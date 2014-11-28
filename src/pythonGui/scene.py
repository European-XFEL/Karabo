#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 21, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

"""This module contains a class which represents a QSvgWidget."""



from components import (DisplayComponent, EditableApplyLaterComponent)

from dialogs.dialogs import PenDialog, TextDialog
from dialogs.devicedialogs import DeviceGroupDialog
from enums import NavigationItemTypes
from layouts import FixedLayout, GridLayout, BoxLayout, ProxyWidget, Layout
from sceneitems.workflowitems import (Item, WorkflowConnection, WorkflowItem, 
                                      WorkflowGroupItem)

from registry import Loadable, Registry
from const import ns_karabo, ns_svg
import pathparser
import icons
import manager

from PyQt4.QtCore import (pyqtSignal, Qt, QByteArray, QEvent, QSize, QRect, QLine,
                          QFileInfo, QBuffer, QIODevice, QMimeData, QRectF,
                          QPoint)
from PyQt4.QtGui import (QAction, QApplication, QBoxLayout, QBrush, QColor,
                         QDialog, QFrame, QLabel, QLayout, QKeySequence, QMenu,
                         QMessageBox, QPalette, QPainter, QPen, QStackedLayout,
                         QWidget)
from PyQt4.QtSvg import QSvgWidget

from xml.etree import ElementTree
from functools import partial

from itertools import chain
from io import BytesIO
import copy


__all__ = ["Scene"]


class Action(Registry):
    actions = [ ]


    @classmethod
    def register(cls, name, dict):
        super(Action, cls).register(name, dict)
        if "text" in dict:
            cls.actions.append(cls)


    @classmethod
    def add_action(cls, source, parent):
        action = QAction(cls.icon, cls.text, source)
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


    def mouseMoveEvent(self, parent, event):
        if event.buttons() and hasattr(self, 'shape'):
            self.shape.set_points(self.start_pos, event.pos())
            parent.update()


    def mouseReleaseEvent(self, parent, event):
        if hasattr(self, 'shape'):
            parent.set_current_action(None)
            parent.ilayout.shapes.append(self.shape)
            parent.setModified()


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
        if d.get("stroke", "none") == "none" or d.get("stroke-width") == "0":
            pen.setStyle(Qt.NoPen)
        else:
            c = QColor(d["stroke"])
            c.setAlphaF(float(d.get("stroke-opacity", 1)))
            pen.setColor(c)
            pen.setCapStyle(dict(
                butt=Qt.FlatCap, square=Qt.SquareCap, round=Qt.RoundCap)
                [d.get("stroke-linecap", "butt")])
            pen.setWidthF(ununit(d.get("stroke-width", "1")))
            pen.setDashOffset(ununit(d.get("stroke-dashoffset", "0")))
            s = d.get("stroke-dasharray", "none")
            if s != "none":
                v = s.split(",") if "," in s else s.split()
                pen.setDashPattern([ununit(s) / pen.width() for s in v])
            
            style = d.get("stroke-style", Qt.SolidLine)
            pen.setStyle(int(style))
            
            pen.setJoinStyle(dict(
                miter=Qt.SvgMiterJoin, round=Qt.RoundJoin, bevel=Qt.BevelJoin)
                [d.get("stroke-linejoin", "miter")])
            pen.setMiterLimit(float(d.get("stroke-miterlimit", 4)))
        self.pen = pen
        s = d.get("fill", "black")
        if s == "none":
            self.brush = QBrush()
        else:
            c = QColor(s)
            c.setAlphaF(float(d.get("fill-opacity", 1)))
            self.brush = QBrush(c)


    def savepen(self, e):
        d = e.attrib
        if self.pen.style() == Qt.NoPen:
            d["stroke"] = "none"
        else:
            d["stroke"] = "#{:06x}".format(self.pen.color().rgb() & 0xffffff)
            d["stroke-opacity"] = str(self.pen.color().alphaF())
            d["stroke-linecap"] = PenDialog.linecaps[self.pen.capStyle()]
            d["stroke-dashoffset"] = str(self.pen.dashOffset())
            d["stroke-width"] = str(self.pen.widthF())
            d["stroke-dasharray"] = " ".join(str(x * self.pen.width())
                                             for x in self.pen.dashPattern())
            d["stroke-style"] = str(self.pen.style())
            d["stroke-linejoin"] = PenDialog.linejoins[self.pen.joinStyle()]
            d["stroke-miterlimit"] = str(self.pen.miterLimit())
        if self.brush.style() == Qt.SolidPattern:
            d["fill"] = "#{:06x}".format(self.brush.color().rgb() & 0xffffff)
            d["fill-opacity"] = str(self.brush.color().alphaF())
        else:
            d["fill"] = "none"


    def draw(self, painter):
        if self.selected:
            black = QPen(Qt.black)
            black.setStyle(Qt.DashLine)
            white = QPen(Qt.white)
            painter.setPen(white)
            painter.drawRect(self.geometry())
            painter.setPen(black)
            painter.drawRect(self.geometry())


    def minimumSize(self):
        return QSize(0, 0)


    def maximumSize(self):
        return QSize(10000, 10000)


class Select(Action):
    """ This is the default action. It has no icon nor text since
    it is selected if nothing else is selected. """


    cursors = {'l': Qt.SizeHorCursor, 'r': Qt.SizeHorCursor,
               't': Qt.SizeVerCursor, 'b': Qt.SizeVerCursor,
               'lt': Qt.SizeFDiagCursor, 'lb': Qt.SizeBDiagCursor,
               'rt': Qt.SizeBDiagCursor, 'rb': Qt.SizeFDiagCursor,
               '': Qt.ArrowCursor, 'm': Qt.OpenHandCursor}


    def __init__(self):
        self.selection_start = self.moving_item = None
        self.resize = ''


    def mousePressEvent(self, parent, event):
        if self.resize:
            return
        item = parent.ilayout.itemAtPosition(event.pos())
        if item is None:
            self.selection_stop = self.selection_start = event.pos()
            parent.update()
        else:
            if event.modifiers() & Qt.ShiftModifier:
                item.selected = not item.selected
            else:
                parent.clear_selection()
                item.selected = True
            parent.update()
            event.accept()


    def mouseMoveEvent(self, parent, event):
        if not event.buttons():
            item = parent.ilayout.itemAtPosition(event.pos())
            self.resize = ""
            if item is not None and item.selected:
                g = item.geometry()
                p = event.pos()
                if p.x() - g.left() < 5:
                    self.resize += 'l'
                elif g.right() - p.x() < 5:
                    self.resize += 'r'
                if p.y() - g.top() < 5:
                    self.resize += 't'
                elif g.bottom() - p.y() < 5:
                    self.resize += 'b'
                if not self.resize:
                    self.resize = 'm'
                self.resize_item = item
                self.moving_pos = event.pos()
            parent.setCursor(self.cursors[self.resize])
        else:
            if self.resize == 'm':
                for c in chain(parent.ilayout, parent.ilayout.shapes):
                    if c.selected:
                        c.translate(event.pos() - self.moving_pos)
                self.moving_pos = event.pos()
                event.accept()
                parent.setModified()
            elif self.selection_start is not None:
                self.selection_stop = event.pos()
                event.accept()
            elif self.resize:
                og = self.resize_item.geometry()
                g = QRect(og)
                if "t" in self.resize:
                    g.setTop(event.pos().y())
                elif "b" in self.resize:
                    g.setBottom(event.pos().y())
                if "l" in self.resize:
                    g.setLeft(event.pos().x())
                elif "r" in self.resize:
                    g.setRight(event.pos().x())
                min = self.resize_item.minimumSize()
                max = self.resize_item.maximumSize()
                if (not min.width() <= g.size().width() <= max.width() or
                    not min.height() <= g.size().height() <= max.height()) and (
                    min.width() <= og.size().width() <= max.width() and
                    min.height() <= og.size().height() <= max.height()):
                    return
                self.resize_item.set_geometry(g)
                parent.ilayout.update()
                parent.setModified()
            parent.update()


    def mouseReleaseEvent(self, parent, event):
        self.resize = ""
        self.resize_item = None
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


class Label(QLabel, Loadable):
    hasBackground = False


    def __init__(self, text="", parent=None):
        QLabel.__init__(self, text, parent)
        Loadable.__init__(self)
        
        self.setFrameShape(QFrame.Box)
        self.setLineWidth(0)


    def save(self, ele):
        ele.set(ns_karabo + "class", "Label")
        ele.set(ns_karabo + "text", self.text())
        ele.set(ns_karabo + "font", self.font().toString())
        ele.set(ns_karabo + "foreground",
                self.palette().color(QPalette.Foreground).name())
        if self.hasBackground:
            ele.set(ns_karabo + "background",
                    self.palette().color(QPalette.Background).name())
        if self.frameShape() == QFrame.Box:
            ele.set(ns_karabo + 'frameWidth', "{}".format(self.lineWidth()))


    @staticmethod
    def load(elem, layout):
        proxy = ProxyWidget(layout.parentWidget())
        label = Label(elem.get(ns_karabo + "text"), proxy)
        proxy.setWidget(label)
        layout.loadPosition(elem, proxy)
        ss = [ ]
        ss.append('qproperty-font: "{}";'.format(elem.get(ns_karabo + "font")))
        ss.append("color: {};".format(
                    elem.get(ns_karabo + "foreground", "black")))
        bg = elem.get(ns_karabo + 'background')
        if bg is not None:
            ss.append("background-color: {};".format(bg))
            label.hasBackground = True
        fw = elem.get(ns_karabo + "frameWidth")
        if fw is not None:
            label.setLineWidth(int(fw))
        label.setStyleSheet("".join(ss))
        return proxy


class LabelAction(Action):
    text = "Add text"
    icon = icons.text


    @classmethod
    def add_action(cls, source, parent):
        action = super(LabelAction, cls).add_action(source, parent)
        c = LabelAction()
        c.action = action
        action.triggered.connect(partial(parent.set_current_action, c))
        return action


    def mousePressEvent(self, parent, event):
        label = Label()
        dialog = TextDialog(label)
        if dialog.exec_() == QDialog.Rejected:
            return
        
        p = ProxyWidget(parent.inner)
        label.setParent(p)
        p.setWidget(label)
        p.fixed_geometry = QRect(event.pos(), p.sizeHint())
        parent.ilayout.add_item(p)
        parent.set_current_action(None)
        parent.setModified()


    def mouseReleaseEvent(self, *args):
        pass


    mouseMoveEvent = mouseReleaseEvent
    draw = mouseReleaseEvent


class Line(Shape):
    xmltag = ns_svg + "line"
    text = "Add line"
    icon = icons.line


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


    def set_geometry(self, rect):
        self.line = QLine(rect.topLeft(), rect.bottomRight())


    def translate(self, p):
        self.line.translate(p)


    def element(self):
        ret = ElementTree.Element(
            ns_svg + "line", x1=str(self.line.x1()),
            x2=str(self.line.x2()), y1=str(self.line.y1()),
            y2=str(self.line.y2()))
        self.savepen(ret)
        return ret


    @staticmethod
    def load(e, layout):
        ret = Line()
        ret.line = QLine(float(e.get("x1")), float(e.get("y1")),
                         float(e.get("x2")), float(e.get("y2")))
        ret.loadpen(e)
        layout.shapes.append(ret)
        return ret


    def edit(self, parent):
        pendialog = PenDialog(self.pen)
        pendialog.exec_()
        parent.setModified()


class Rectangle(Shape):
    xmltag = ns_svg + "rect"
    text = "Add rectangle"
    icon = icons.rect


    def set_points(self, start, end):
        self.rect = QRect(start, end).normalized()


    def draw(self, painter):
        painter.setPen(self.pen)
        painter.setBrush(self.brush)
        painter.drawRect(self.rect)
        Shape.draw(self, painter)


    def element(self):
        ret = ElementTree.Element(
            ns_svg + "rect", x=str(self.rect.x()),
            y=str(self.rect.y()), width=str(self.rect.width()),
            height=str(self.rect.height()))
        self.savepen(ret)
        return ret

    def contains(self, p):
        l, r = self.rect.left(), self.rect.right()
        t, b = self.rect.top(), self.rect.bottom()
        x, y = p.x(), p.y()
        if self.brush.style() == Qt.SolidPattern:
            return l < x < r and t < y < b
        else:
            return (l < x < r and min(abs(t - y), abs(b - y)) < self.fuzzy or
                    t < y < b and min(abs(l - x), abs(r - x)) < self.fuzzy)

    def geometry(self):
        return self.rect

    def set_geometry(self, rect):
        self.rect = rect


    def translate(self, p):
        self.rect.translate(p)

    @staticmethod
    def load(e, layout):
        ret = Rectangle()
        ret.rect = QRect(float(e.get("x")), float(e.get("y")),
                         float(e.get("width")), float(e.get("height")))
        ret.loadpen(e)
        layout.shapes.append(ret)
        return ret


    def edit(self, parent):
        pendialog = PenDialog(self.pen, self.brush)
        pendialog.exec_()
        parent.setModified()


class Path(Shape):
    xmltag = ns_svg + "path"


    def contains(self, p):
        r = QRectF(p - QPoint(3, 3), p + QPoint(3, 3))
        return self.path.intersects(r)


    def geometry(self):
        return self.path.boundingRect().toRect()


    def translate(self, p):
        self.path.translate(p)


    @staticmethod
    def load(e, layout):
        ret = Path()
        ret.svg = e.get('d')
        parser = pathparser.Parser(ret.svg)
        ret.path = parser.parse()
        ret.loadpen(e)
        layout.shapes.append(ret)
        return ret


    def draw(self, painter):
        painter.setPen(self.pen)
        painter.setBrush(self.brush)
        painter.drawPath(self.path)
        Shape.draw(self, painter)


    def edit(self, parent):
        pendialog = PenDialog(self.pen, self.brush)
        pendialog.exec_()
        parent.setModified()


    def element(self):
        ret = ElementTree.Element(ns_svg + "path", d=self.svg)
        self.savepen(ret)
        return ret


Separator()

class GroupActions(ActionGroup):
    icon = icons.group
    text = "Group"

class GroupAction(SimpleAction):
    "Group several items into one group"

    def gather_widgets(self):
        i = 0
        rect = QRect()
        l = [ ]
        while i < len(self.parent.ilayout):
            c = self.parent.ilayout[i]
            if c.selected:
                c.selected = False
                l.append(c)
                if isinstance(c, Layout):
                    del self.parent.ilayout[i]
                    i -= 1
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


class FixedGroup(GroupActions, GroupAction):
    text = "Group without layout"
    icon = icons.groupGrid


    def run(self):
        rect, widgets = self.gather_widgets()
        group = FixedLayout()
        for w in widgets:
            group.add_item(w)
        group.shapes = self.gather_shapes()
        for s in group.shapes:
            rect = rect.united(s.geometry())
        if rect.isNull():
            return
        group.fixed_geometry = rect
        self.parent.ilayout.add_item(group)
        group.selected = True


class BoxGroup(GroupAction):
    def doit(self, group, cmp):
        rect, widgets = self.gather_widgets()
        if rect.isNull():
            return
        widgets.sort(key=cmp)
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
    icon = icons.groupVertical


    def run(self):
        self.doit(BoxLayout(BoxLayout.TopToBottom), lambda x: x.geometry().y())


class HorizontalGroup(GroupActions, BoxGroup):
    text = "Group Horizontally"
    icon = icons.groupHorizontal


    def run(self):
        self.doit(BoxLayout(BoxLayout.LeftToRight), lambda x: x.geometry().x())


class GridGroup(GroupActions, BoxGroup):
    text = "Group in a Grid"
    icon = icons.groupGrid


    def run(self):
        rect, widgets = self.gather_widgets()
        if rect.isNull():
            return
        group = GridLayout()
        group.set_children(widgets)
        group.shapes = self.gather_shapes()
        group.fixed_geometry = QRect(rect.topLeft(), group.sizeHint())
        self.parent.ilayout.add_item(group)


class Ungroup(GroupActions, SimpleAction):
    "Ungroup items"
    text = "Ungroup"
    icon = icons.ungroup


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


class EntireWindow(GroupActions, SimpleAction):
    'One group should occupy the entire window'
    text = "Entire Window"
    icon = icons.entireWindow


    def run(self):
        if self.parent.ilayout.entire is not None:
            self.parent.ilayout.entire = None
            self.parent.ilayout.update()
            return
        for c in self.parent.ilayout:
            if c.selected:
                self.parent.ilayout.entire = c
                self.parent.ilayout.update()
                return


Separator()


class SelectAll(SimpleAction):
    text = 'Select All'
    icon = icons.selectAll
    shortcut = QKeySequence.SelectAll


    def run(self):
        for c in self.parent.ilayout:
            c.selected = True
        for s in self.parent.ilayout.shapes:
            s.selected = True
        self.parent.update()


Separator()


class Cut(SimpleAction):
    text = "Cut"
    icon = icons.editCut
    shortcut = QKeySequence.Cut


    def run(self):
        QApplication.clipboard().setMimeData(self.parent.mimeData())
        self.parent.ilayout.delete_selected()
        self.parent.update()
        self.parent.setModified()


class Copy(SimpleAction):
    text = "Copy"
    icon = icons.editCopy
    shortcut = QKeySequence.Copy


    def run(self):
        QApplication.clipboard().setMimeData(self.parent.mimeData())


class Paste(SimpleAction):
    text = "Paste"
    icon = icons.editPaste
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
        self.parent.setModified()


class Delete(SimpleAction):
    text = "Delete"
    icon = icons.delete
    shortcut = QKeySequence.Delete


    def run(self):
        selected = [c for c in self.parent.ilayout if c.selected]
        selectedShapes = [s for s in self.parent.ilayout.shapes if s.selected]
        if not selected and not selectedShapes:
            return
        
        if QMessageBox.question(self.parent, "Really delete?",
                                "Do you really want to delete the items?",
                                QMessageBox.Yes | QMessageBox.No
                               ) == QMessageBox.Yes:
            self.parent.ilayout.delete_selected()
            self.parent.update()
            self.parent.setModified()

Separator()


class Raise(SimpleAction):
    text = "Bring to front"
    icon = icons.bringToFront


    def run(self):
        shapes = self.parent.ilayout.shapes
        for i in range(len(shapes)):
            if shapes[i].selected:
                j = len(shapes) - 1
                for j in range(i + 1, len(shapes)):
                    if not shapes[j].geometry().intersects(
                            shapes[i].geometry()):
                        break
                shapes[j], shapes[i:j] = shapes[i], shapes[i + 1:j + 1]
        for w in self.parent.ilayout.iterWidgets(selected=True):
            w.raise_()
        self.parent.update()
        self.parent.setModified()


class Lower(SimpleAction):
    text = "Send to back"
    icon = icons.sendToBack


    def run(self):
        shapes = self.parent.ilayout.shapes
        for i in range(len(shapes) - 1, 0, -1):
            if shapes[i].selected:
                j = 0
                for j in range(i - 1, -1, -1):
                    if not shapes[j].geometry().intersects(
                            shapes[i].geometry()):
                        break
                shapes[j], shapes[j + 1:i + 1] = shapes[i], shapes[j:i]
        for w in self.parent.ilayout.iterWidgets(selected=True):
            w.lower()
        self.parent.update()
        self.parent.setModified()


class Scene(QSvgWidget):
    signalSceneItemSelected = pyqtSignal(object)
    
    def __init__(self, project, name, parent=None, designMode=True):
        super(Scene, self).__init__(parent)

        self.project = project
        # Connect signals to forward selection of scene item
        self.signalSceneItemSelected.connect(self.project.signalSelectObject)
        self.project.signalDeviceSelected.connect(self.onSelectionChanged)
        
        self.filename = name
        fi = QFileInfo(self.filename)
        if len(fi.suffix()) < 1:
            self.filename = "{}.svg".format(self.filename)

        self.inner = QWidget(self)
        self.inner.setLayout(FixedLayout())
        layout = QStackedLayout(self)
        layout.addWidget(self.inner)
        
        self.workflow_connection = None
        
        self.current_action = self.default_action = Select()
        self.current_action.action = QAction(self) # never displayed
        self.simple_actions = [ ]

        self.tree = ElementTree.ElementTree(ElementTree.Element(ns_svg + "svg"))

        self.designMode = designMode

        self.setFocusPolicy(Qt.StrongFocus)
        self.setAcceptDrops(True)
        self.setAttribute(Qt.WA_MouseTracking)
        self.setBackgroundRole(QPalette.Window)
        self.resize(1024, 768)


    def setModified(self):
        """
        This scene was modified and this needs to be broadcasted to the project.
        """
        self.project.setModified(True)


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


    def closeEvent(self, event):
        if len(self.ilayout) == 0 and not self.ilayout.shapes:
            return

        messageBox = QMessageBox(self)
        messageBox.setWindowTitle("Save scene before closing")
        messageBox.setText("Do you want to save your scene before closing?")
        messageBox.setStandardButtons(QMessageBox.Save | QMessageBox.Discard | 
                                      QMessageBox.Cancel)
        messageBox.setDefaultButton(QMessageBox.Save)

        reply = messageBox.exec_()
        if reply == QMessageBox.Cancel:
            event.ignore()
            return

        if reply == QMessageBox.Save:
            self.project.zip()
        
        event.accept()


    @property
    def designMode(self):
        return self.inner.testAttribute(Qt.WA_TransparentForMouseEvents)


    @designMode.setter
    def designMode(self, value):
        self.inner.setAttribute(Qt.WA_TransparentForMouseEvents, value)


    @property
    def ilayout(self):
        return self.inner.layout()


    def reset(self):
        if len(self.ilayout) == 0 and not self.ilayout.shapes:
            return

        reply = QMessageBox.question(
            self, "Save scene before closing",
            "Do you want to save your scene before closing?",
            QMessageBox.Save | QMessageBox.Discard, QMessageBox.Discard)

        if reply == QMessageBox.Save:
            self.project.zip()

        self.clean()
        self.inner.setLayout(FixedLayout())
        self.layout().addWidget(self.inner)


    def clean(self):
        """Remove all child widgets"""
        for c in self.inner.children():
            if isinstance(c, ProxyWidget) and c.component is not None:
                for b in c.component.boxes:
                    b.configuration.removeVisible()
            #c.setParent(None)
        self.inner.setParent(None)
        self.inner = QWidget(self)
        self.layout().addWidget(self.inner)


    def fromXml(self, xmlString):
        """
        Parses the given xmlString which represents the SVG.
        """
        self.tree = ElementTree.parse(BytesIO(xmlString))
        root = self.tree.getroot()
        self.clean()
        FixedLayout.load(root, widget=self.inner)
        self.resize(int(root.get('width', 1024)), int(root.get('height', 768)))
        self.layout().setGeometry(self.geometry())
        self.ilayout.setGeometry(self.inner.geometry())
        self.designMode = True
        
        ar = QByteArray()
        buf = QBuffer(ar)
        buf.open(QIODevice.WriteOnly)
        self.tree.write(buf)
        buf.close()
        self.load(ar)


    def toXml(self):
        """
        Returns the scene as XML string.
        """
        root = copy.copy(self.tree.getroot())
        tree = ElementTree.ElementTree(root)
        e = self.ilayout.element()
        root.extend(ee for ee in e)
        root.set('width', str(self.width()))
        root.set('height', str(self.height()))
        return ElementTree.tostring(root)


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


    def clear_selection(self):
        for s in self.ilayout.shapes:
            s.selected = False
        for c in self.ilayout:
            c.selected = False


    def workflowChannelHit(self, event):
        """
        Returns ProxyWidget and corresponding WorkflowItem, if there are any at
        the given position.
        """
        proxy = self.ilayout.itemAtPosition(event.pos())
        if proxy is not None:
            if isinstance(proxy, ProxyWidget):
                workflowItem = proxy.widget
                # Check for WorkflowItem...
                if isinstance(workflowItem, Item):
                    # Send selection signal to connected project
                    self.signalSceneItemSelected.emit(workflowItem.getDevice())
                    return proxy, workflowItem.mousePressEvent(proxy, event)
        return None, None


    def mousePressEvent(self, event):
        if not self.designMode:
            return
            
        if event.button() == Qt.LeftButton:
            proxy, channel = self.workflowChannelHit(event)
            if channel is not None and channel.allowConnection():
                proxy.selected = False
                # Create workflow connection item in scene - only for allowed
                # connection type (Network)
                self.workflow_connection = WorkflowConnection(self, channel)
                QWidget.mousePressEvent(self, event)
                return
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
        if self.workflow_connection is not None:
            self.workflow_connection.mouseMoveEvent(event)
        self.current_action.mouseMoveEvent(self, event)
        QWidget.mouseMoveEvent(self, event)


    def mouseReleaseEvent(self, event):
        if self.workflow_connection is not None:
            _, channel = self.workflowChannelHit(event)
            if channel is not None and channel.allowConnection():
                self.workflow_connection.mouseReleaseEvent(self, channel)
            else:
                self.update()
            self.workflow_connection = None
        
        self.current_action.mouseReleaseEvent(self, event)
        QWidget.mouseReleaseEvent(self, event)


    def mouseDoubleClickEvent(self, event):
        if not self.designMode:
            return
        w = self.inner.childAt(event.pos())
        if w is not None:
            while not isinstance(w, ProxyWidget):
                w = w.parent()
            if isinstance(w.widget, Label):
                dialog = TextDialog(w.widget)
                dialog.exec_()
                self.setModified()
            return
        item = self.ilayout.itemAtPosition(event.pos())
        if item is None:
            return
        item.edit(self)


    def dragEnterEvent(self, event):
        sourceType = event.mimeData().data("sourceType")
        
        if sourceType == "ParameterTreeWidget":
            source = event.source()
            if (source is not None) and self.designMode \
               and not (source.conf.type == "class"):
                event.accept()
        elif sourceType == "NavigationTreeView":
            source = event.source()
            if (source is not None) and self.designMode \
               and source.indexInfo().get("type") == NavigationItemTypes.CLASS:
                event.accept()
        QWidget.dragEnterEvent(self, event)


    def dropEvent(self, event):
        w = self.inner.childAt(event.pos())
        if w is not None:
            while not isinstance(w, ProxyWidget):
                w = w.parent()
            w.dropEvent(event)
            if event.isAccepted():
                return
        
        mimeData = event.mimeData()
        sourceType = mimeData.data("sourceType")

        source = event.source()
        if sourceType == "ParameterTreeWidget":
            selectedItems = source.selectedItems()

            self.clear_selection()
            for item in selectedItems:
                # Create layout for coming context
                layout = BoxLayout(QBoxLayout.LeftToRight)

                # Create label
                displayName = item.text(0)
                proxy = ProxyWidget(self.inner)
                label = Label(displayName, proxy)
                label.hasBackground = False
                proxy.setWidget(label)
                layout.addWidget(proxy)
                proxy.show()

                # Get Boxes. "box" is in the project, "realbox" the
                # one on the device. They are the same if not from a project
                box = item.box
                if box.configuration.type == "projectClass":
                    realbox = manager.getDevice(box.configuration.id
                                                ).getBox(box.path)
                    if realbox.descriptor is not None:
                        box = realbox
                else:
                    realbox = box

                # Create display component, if available
                configDisplayComponent = item.displayComponent
                if configDisplayComponent is not None:
                    proxy = ProxyWidget(self.inner)
                    displayComponent = DisplayComponent(
                        box.descriptor.classAlias, realbox, proxy)
                    proxy.setComponent(displayComponent)
                    proxy.setWidget(displayComponent.widget)
                    layout.addWidget(proxy)
                    proxy.show()
                    realbox.configuration.addVisible()

                    unit = (box.descriptor.metricPrefixSymbol +
                            box.descriptor.unitSymbol)
                    if len(unit) > 0:
                        proxy = ProxyWidget(self.inner)
                        proxy.setWidget(Label(unit, proxy))
                        layout.addWidget(proxy)
                        proxy.show()

                # Create editable component, if available
                configEditableComponent = item.editableComponent
                if configEditableComponent is not None:
                    proxy = ProxyWidget(self.inner)

                    editableComponent = EditableApplyLaterComponent(
                        box.descriptor.classAlias, realbox, proxy)

                    realbox.configuration.addVisible()
                    proxy.setComponent(editableComponent)
                    proxy.setWidget(editableComponent.widget)
                    layout.addWidget(proxy)
                    proxy.show()

                layout.fixed_geometry = QRect(event.pos(), layout.sizeHint())
                self.ilayout.add_item(layout)
                layout.selected = True
            self.project.setModified(True)
        elif sourceType == "NavigationTreeView":
            itemInfo = source.indexInfo(source.currentIndex())
            serverId  = itemInfo.get("serverId")
            classId = itemInfo.get("classId")
            
            # Restore cursor for dialog input
            QApplication.restoreOverrideCursor()
            # Open dialog to set up new device (group)
            dialog = DeviceGroupDialog(manager.Manager().systemHash, serverId, classId)
            if dialog.exec_() == QDialog.Accepted:
                if not dialog.isDeviceGroup:
                    object = self.project.getDevice(dialog.deviceId)
                    # TODO: overwrite existing device?
                    if object is None:
                        # Create only one device configuration and add to project,
                        # if not exists
                        object = self.project.newDevice(dialog.serverId,
                                                        dialog.classId,
                                                        dialog.deviceId,
                                                        dialog.startupBehaviour)

                    # Create scene item associated with device
                    proxy = ProxyWidget(self.inner)
                    workflowItem = WorkflowItem(object, proxy)
                else:
                    object = self.project.getDevice(dialog.deviceGroupName)
                    # TODO: overwrite existing device group?
                    if object is None:
                        # Create device group and add to project, if not exists
                        object = self.project.newDeviceGroup(dialog.deviceGroupName,
                                                            dialog.serverId,
                                                            dialog.classId,
                                                            dialog.deviceId,
                                                            dialog.startupBehaviour,
                                                            dialog.displayPrefix,
                                                            dialog.startIndex,
                                                            dialog.endIndex)

                    # Create scene item associated with device group
                    proxy = ProxyWidget(self.inner)
                    workflowItem = WorkflowGroupItem(object, proxy)
                
                rect = workflowItem.boundingRect()
                proxy.setWidget(workflowItem)
                proxy.fixed_geometry = QRect(event.pos(), QSize(rect.width(), rect.height()))
                proxy.show()
                
                self.clear_selection()
                self.ilayout.add_item(proxy)
                
                self.project.setModified(True)
                self.project.signalSelectObject.emit(object)
        event.accept()
        QWidget.dropEvent(self, event)


    def event(self, event):
        ret = QWidget.event(self, event)
        if event.type() == QEvent.ToolTip:
            item = self.inner.childAt(event.pos())
            if item is not None:
                while not isinstance(item, ProxyWidget):
                    item = item.parent()
                item.event(event)
                return True
        return ret


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
            if self.workflow_connection is not None:
                self.workflow_connection.draw(painter)
            self.current_action.draw(painter)
        finally:
            painter.end()


    def onSelectionChanged(self, _, object):
        """
        This slot is called whenever an item of the project panel is selected.
        The corresponding scene item is selected as well, if existent.
        """
        if object is None or self.ilayout is None:
            return
        
        for c in self.ilayout:
            if not isinstance(c, ProxyWidget):
                continue
            
            workflowItem = c.widget
            if isinstance(workflowItem, Item) and (workflowItem.getObject() is object):
                self.clear_selection()
                c.selected = True
                self.update()
                return

        # No corresponding object found - clear selection
        self.clear_selection()
        self.update()

