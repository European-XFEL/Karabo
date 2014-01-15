#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 21, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a QGraphicsView."""

__all__ = ["GraphicsView"]
    
from customxmlreader import CustomXmlReader
from customxmlwriter import CustomXmlWriter

from displaycomponent import DisplayComponent

from enums import NavigationItemTypes
from enums import ConfigChangeTypes
from enums import CompositionMode

from layoutcomponents.arrow import Arrow
from editableapplylatercomponent import EditableApplyLaterComponent
from editablenoapplycomponent import EditableNoApplyComponent
from layoutcomponents.graphicscustomitem import GraphicsCustomItem
from layoutcomponents.graphicsproxywidget import GraphicsProxyWidget
from layoutcomponents.graphicsproxywidgetcontainer import GraphicsProxyWidgetContainer
from layoutcomponents.line import Line
from layoutcomponents.link import Link
from layoutcomponents.linkbase import LinkBase
from layoutcomponents.nodebase import NodeBase
from layoutcomponents.rectangle import Rectangle
from layoutcomponents.text import Text
from layoutcomponents.textdialog import TextDialog

from manager import Manager

from widget import DisplayWidget, EditableWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtSvg import QSvgWidget

from xml.etree import ElementTree
from functools import partial

ns_svg = "{http://www.w3.org/2000/svg}"
ns_karabo = "{http://karabo.eu/scene}"
ElementTree.register_namespace("svg", ns_svg[1:-1])
ElementTree.register_namespace("krb", ns_karabo[1:-1])

class MetaAction(type):
    def __init__(self, name, bases, dict):
        super(MetaAction, self).__init__(name, bases, dict)
        if "text" in dict:
            Action.actions.append(self)
        if "xmltag" in dict:
            Shape.xmltags[self.xmltag] = self

class Action(object):
    __metaclass__ = MetaAction
    actions = [ ]

    @classmethod
    def add_action(cls, source, parent):
        action = QAction(QIcon(cls.icon), cls.text, source)
        action.setStatusTip(cls.text)
        action.setToolTip(cls.text)
        cls.action = action
        return action


class CheckableAction(Action):
    @classmethod
    def add_action(cls, source, parent):
        action = super(CheckableAction, cls).add_action(source, parent)
        action.setCheckable(True)
        action.triggered.connect(partial(parent.set_current_action, cls))
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


class Select(CheckableAction):
    """ This is the default action. It has no icon nor text since
    it is selected if nothing else is selected. """


    def __init__(self):
        self.selection_start = self.moving_item = None


    def mousePressEvent(self, parent, event):
        item = parent.layout.itemAtPosition(event.pos())
        if item is None:
            for s in parent.layout.shapes:
                if s.contains(event.pos()):
                    item = s
                    break
        if item is None:
            self.selection_stop = self.selection_start = event.pos()
            parent.update()
        else:
            self.moving_item = item
            self.moving_pos = (
                event.pos() - self.moving_item.geometry().topLeft())
            if event.modifiers() & Qt.ShiftModifier:
                item.selected = not item.selected
            else:
                parent.clear_selection()
                item.selected = True
            parent.update()
            event.accept()

    def mouseMoveEvent(self, parent, event):
        if self.moving_item is not None:
            self.moving_item.set_position(event.pos() - self.moving_pos)
            event.accept()
        elif self.selection_start is not None:
            self.selection_stop = event.pos()
            event.accept()
        parent.update()


    def mouseReleaseEvent(self, parent, event):
        self.moving_item = None
        if self.selection_start is not None:
            rect = QRect(self.selection_start, self.selection_stop)
            if not event.modifiers() & Qt.ShiftModifier:
                parent.clear_selection()
            for c in parent.layout:
                if rect.contains(c.geometry()):
                    c.selected = True
            self.selection_start = None
            event.accept()
            parent.update()


    ready = True # selections can always be drawn


    def draw(self, painter):
        if self.selection_start is not None:
            painter.setPen(Qt.black)
            painter.drawRect(QRect(self.selection_start, self.selection_stop))


class Group(SimpleAction):
    "Group several items into one group"

    text = "Group"
    icon = ":line"

    def run(self):
        i = 0
        g = BoxLayout(BoxLayout.TopToBottom)
        rect = None
        while i < len(self.parent.layout):
            c = self.parent.layout[i]
            if c.selected:
                c.selected = False
                if isinstance(c, ProxyWidget):
                    g.addWidget(c)
                else:
                    g.addItem(c)
                    del self.parent.layout[i]
                if rect is None:
                    rect = c.geometry()
                else:
                    rect = rect.united(c.geometry())
            else:
                i += 1
        if rect is None:
            return
        shapes = self.parent.layout.shapes
        g.shapes = [s for s in shapes if s.selected]
        self.parent.layout.shapes = [s for s in shapes if not s.selected]
        for s in g.shapes:
            s.selected = False
        g.fixed_geometry = QRect(rect.topLeft(), g.sizeHint())
        self.parent.layout.add_item(g)


class Ungroup(SimpleAction):
    "Ungroup items"

    text = "Ungroup"
    icon = ":line"

    def run(self):
        i = 0
        while i < len(self.parent.layout):
            child = self.parent.layout[i]
            if child.selected and isinstance(child, Layout):
                cl = list(child)
                for c in cl:
                    c.fixed_geometry = c.geometry()
                self.parent.layout[i:i + 1] = cl
                self.parent.layout.shapes.extend(child.shapes)
                i += len(cl)
            else:
                i += 1


class Shape(CheckableAction):
    xmltags = { }
    fuzzy = 3

    def __init__(self):
        Action.__init__(self)
        self.selected = False
        self.pen = QPen()
        self.brush = QBrush()

    def mousePressEvent(self, parent, event):
        self.start_pos = event.pos()
        self.set_points(self.start_pos, self.start_pos)
        event.accept()

    def mouseMoveEvent(self, parent, event):
        if self.ready:
            self.set_points(self.start_pos, event.pos())
            event.accept()
            parent.update()

    def mouseReleaseEvent(self, parent, event):
        if self.ready:
            parent.set_current_action(None)
            parent.layout.shapes.append(self)


    @property
    def ready(self):
        "returns whether this shape is already well-defined"
        return hasattr(self, "start_pos")


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

    @staticmethod
    def load(element):
        try:
            return Shape.xmltags[element.tag].load(element)
        except KeyError:
            return

    def draw(self, painter):
        if self.selected:
            painter.setPen(Qt.DashLine)
            painter.drawRect(self.geometry())

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

    def set_position(self, p):
        self.line.translate(p - self.geometry().topLeft())

    def element(self):
        ret = ElementTree.Element(
            ns_svg + "line", x1=unicode(self.line.x1()),
            x2=unicode(self.line.x2()), y1=unicode(self.line.y1()),
            y2=unicode(self.line.y2()))
        self.savepen(ret)
        return ret

    @staticmethod
    def load(e):
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

    def set_position(self, p):
        self.rect.moveTo(p)

    @staticmethod
    def load(e):
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

class Label(object):
    """ Fake class to make a QLabel loadable """
    @staticmethod
    def load(elem, parent):
        label = QLabel(elem.get(ns_karabo + "text"), parent)
        ret = ProxyWidget(parent)
        ret.set_child(label, None)
        ret.show()
        return ret

class Layout(object):
    def __init__(self):
        self.shapes = [ ]
        self.shape_geometry = None


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


    def load(self, element):
        i = 0
        while i < len(element):
            elem = element[i]
            cls = elem.get(ns_karabo + "class")
            if cls is None:
                i += 1
            else:
                obj = globals()[cls].load(elem, self.parentWidget())
                obj.fixed_geometry = _parse_rect(elem)
                self.add_item(obj)
                del root[i]

        shapes = [ ]
        for e in element[::-1]:
            s = Shape.load(e)
            if s is None:
                break
            else:
                shapes.append(s)
        if len(shapes) > 0:
            del root[-len(shapes):]
        self.shapes = shapes[::-1]


    def draw(self, painter):
        for s in self.shapes:
            painter.save()
            s.draw(painter)
            painter.restore()
        for i in range(self.count()):
            item = self.itemAt(i)
            if isinstance(item, Layout):
                item.draw(painter)


    def element(self):
        g = self.geometry()
        d = { ns_karabo + "x": g.x(), ns_karabo + "y": g.y(),
              ns_karabo + "width": g.width(), ns_karabo + "height": g.height(),
              ns_karabo + "class": self.__class__.__name__ }
        d.update(self.save())

        e = ElementTree.Element(ns_svg + "g",
                                {k: unicode(v) for k, v in d.iteritems()})
        e.extend(l.widget().element() if isinstance(l, QWidgetItem) else
                 l.element() for l in (self.itemAt(i)
                                       for i in range(self.count())))
        return e


    def set_position(self, pos):
        self.fixed_geometry.moveTo(pos)
        self.update()


    def update_shapes(self, rect):
        if self.shape_geometry is None:
            self.shape_geometry = QRect(self.fixed_geometry)
        for s in self.shapes:
            s.set_position(s.geometry().topLeft() + rect.topLeft() -
                           self.shape_geometry.topLeft())
        self.shape_geometry = QRect(rect)


class FixedLayout(Layout, QLayout):
    def __init__(self, parent):
        QLayout.__init__(self, parent)
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


    def addItem(self, item):
        "only to be used by Qt, don't use directly!"
        self._item = item


    def add_item(self, item):
        """add ProxyWidgets or Layouts"""
        self[len(self):len(self)] = [item]


    def itemAt(self, index):
        "only to be used by Qt, don't use directly!"
        try:
            return self._children[index]
        except IndexError:
            return


    def takeAt(self, index):
        "only to be used by Qt, don't use directly!"
        try:
            ret = self._children.pop(index)
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


    def itemAtPosition(self, pos):
        for item in self._children:
            if item.geometry().contains(pos):
                if item.widget():
                    return item.widget()
                else:
                    return item
            

    def relayout(self, widget):
        for c in self.children:
            ll = [c]
            ret = None
            while ll:
                l = ll.pop()
                if isinstance(l, QLayout):
                    for i in range(l.count()):
                        ll.append(l.itemAt(i))
                else:
                    if l.widget() is widget:
                        ret = l
                        break
            if ret is not None:
                break
        if ret is None:
            return
        c.setGeormetry(QRect(self.positions[c], c.sizeHint()))


    def save(self):
        return { }


class BoxLayout(QBoxLayout, Layout):
    def __init__(self, dir):
        QBoxLayout.__init__(self, dir)
        Layout.__init__(self)
        self.selected = False
        self.setContentsMargins(5, 5, 5, 5)

    @staticmethod
    def load(elem, parent):
        ret = BoxLayout(int(elem.get(ns_karabo + "Direction")))
        for i in range(len(elem)):
            cls = elem[i].get(ns_karabo + "class")
            item = globals()[cls].load(elem[i], parent)
            if isinstance(item, QWidget):
                ret.addWidget(item)
            else:
                ret.addLayout(item)
        return ret

    def save(self):
        return {ns_karabo + "Direction": self.direction()}


    def setGeometry(self, rect):
        QBoxLayout.setGeometry(self, rect)
        self.update_shapes(rect)


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
            widgetFactory = self.component.widgetFactory
            d[ns_karabo + "class"] = "ProxyWidget"
            d[ns_karabo + "componentType"] = self.component.__class__.__name__
            d[ns_karabo + "widgetFactory"] = "DisplayWidget"
            d[ns_karabo + "classAlias"] = self.component.classAlias
            if self.component.classAlias == "Command":
                d[ns_karabo + "commandText"] = widgetFactory.widget.text()
                d[ns_karabo + "commandEnabled"] = "{}".format(
                    widgetFactory.widget.isEnabled())
                d[ns_karabo + "allowedStates"] = ",".join(
                    widgetFactory.allowedStates)
                d[ns_karabo + "command"] = widgetFactory.command
            d[ns_karabo + "key"] = ",".join(self.component.keys)
        return ElementTree.Element(ns_svg + "rect",
                                   {k: unicode(v) for k, v in d.iteritems()})

    @staticmethod
    def load(elem, parent):
        ks = "classAlias", "key", "widgetFactory"
        if elem.get(ns_karabo + "classAlias") == "Command":
            ks += "command", "allowedStates", "commandText"
        d = {k: elem.get(ns_karabo + k) for k in ks}
        d["commandEnabled"] = elem.get(ns_karabo + "commandEnabled") == "True"
        component = globals()[elem.get(ns_karabo + "componentType")](**d)
        component.widget.setAttribute(Qt.WA_NoSystemBackground, True)
        ret = ProxyWidget(parent)
        ret.set_child(component.widget, component)
        ret.show()
        return ret

    def set_position(self, pos):
        self.fixed_geometry.moveTo(pos)
        self.parent().layout().update()


class GraphicsView(QSvgWidget):
    def __init__(self, parent):
        super(GraphicsView, self).__init__(parent)
        
        self.inner = QWidget(self)
        self.layout = FixedLayout(self.inner)
        self.inner.setLayout(self.layout)
        layout = QStackedLayout(self)
        layout.addWidget(self.inner)
        
        self.current_action = self.default_action = Select()
        self.current_action.action = QAction(self) # never displayed
        self.simple_actions = [ ]

        self.tree = ElementTree.ElementTree(ElementTree.Element(ns_svg + "svg"))

        # Composition mode is either ON/OFFLINE, once set not changeable
        self.__compositionMode = CompositionMode.UNDEFINED

        self.designMode = True

        # Describes most recent item to be cut or copied inside the application
        self.__copiedItem = QByteArray()

        self.setAcceptDrops(True)

    def add_actions(self, source):
        for v in Action.actions:
            action = v.add_action(source, self)
            yield action

    def set_current_action(self, Action):
        self.current_action.action.setChecked(False)
        if Action is None:
            self.current_action = self.default_action
        else:
            self.current_action = Action()
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
                                               filter="SVG (*.svg)")
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
        self.layout.load(root)

        ar = QByteArray()
        buf = QBuffer(ar)
        buf.open(QIODevice.WriteOnly)
        self.tree.write(buf)
        buf.close()
        self.load(ar)


    # Helper function opens *.xml configuration file
    def openConfiguration(self, filename):
        print "openConfiguration", filename


    # Save active view to file
    def saveSceneLayoutToFile(self):
        filename = QFileDialog.getSaveFileName(None, "Save file as",
                                               filter="SVG (*.svg)")
        if len(filename) < 1:
            return

        fi = QFileInfo(filename)
        if len(fi.suffix()) < 1:
            filename += ".svg"

        root = self.tree.getroot().copy()
        tree = ElementTree.ElementTree(root)
        e = self.layout.element()
        root.extend(s.element() for s in self.shapes)
        root.extend(ee for ee in e)
        tree.write(filename)


    def saveSceneConfigurationsToFile(self):
        dirPath = QFileDialog.getExistingDirectory(self, "Select directory to save configuration files", QDir.tempPath(),
                                                   QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)

        if len(dirPath) < 1:
            return

        # Check, if directory is empty
        self.checkDirectoryBeforeSave(dirPath)

        # Save configurations of navigation related items
        self.saveSceneConfigurations(dirPath)


    # Save active view and configurations to folder/files
    def saveSceneLayoutConfigurationsToFile(self):
        dirPath = QFileDialog.getExistingDirectory(self, "Select directory to save layout and configuration files", QDir.tempPath(),
                                                   QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)

        if len(dirPath) < 1:
            return

        # Check, if directory is empty
        self.checkDirectoryBeforeSave(dirPath)

        # Save layout to directory
        CustomXmlWriter(self.__scene).write(dirPath + "/" + QDir(dirPath).dirName() + ".scene")

        # Save configurations of navigation related items
        self.saveSceneConfigurations(dirPath)


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
        items = self.items()
        for item in items:
            if isinstance(item, GraphicsCustomItem):
                # TODO: Remove dirty hack for scientific computing again!!!
                croppedClassId = item.text().split("-")
                classId = croppedClassId[0]
                Manager().saveAsXml(str(dirPath + "/" + item.text() + ".xml"), str(classId), str(item.internalKey()))


    # A new instance of a text is created and passed to the _setupItem function
    # to position and select
    def addText(self):
        textDialog = TextDialog(self)
        if textDialog.exec_() == QDialog.Rejected:
            return

        textItem = Text(self.__isDesignMode)
        textItem.setText(textDialog.text())
        textItem.setFont(textDialog.font())
        textItem.setTextColor(textDialog.textColor())
        textItem.setBackgroundColor(textDialog.backgroundColor())
        textItem.setOutlineColor(textDialog.outlineColor())

        self._setupItem(textItem)


    # Add a link, if exactely 2 items are selected
    def addLink(self):
        items = self.selectedItemPair()
        if items is None:
            return

        link = Link(items[0], items[1])
        self.__scene.addItem(link)


    # Add an arrow, if exactely 2 items are selected
    def addArrowLink(self):
        items = self.selectedItemPair()
        if items is None:
            return

        arrowLink = Arrow(items[0], items[1])
        self.__scene.addItem(arrowLink)


    # Cut is two-part process: copy selected items and remove item from scene
    def cut(self):
        items = self.selectedItems()
        if len(items) < 1:
            return

        # Copy items
        self.copy()
        # Remove items from scene
        for item in items:
            self.__scene.removeItem(item)
            del item


    # The selected items are stored as binary data inside the application
    def copy(self):
        items = self.selectedItems()
        if len(items) < 1:
            return

        # Copy data into DataStream
        self.__copiedItem.clear()
        stream = QDataStream(self.__copiedItem, QIODevice.WriteOnly)
        for item in items:
            if isinstance(item, Text):
                stream << "Text" << item.text() \
                                          << item.font().toString() \
                                          << item.textColor().name() \
                                          << item.outlineColor().name() \
                                          << item.backgroundColor().name() \
                                          << "\n"
            elif isinstance(item, Link):
                print "Link"
            elif isinstance(item, Arrow):
                print "Arrow"
            elif isinstance(item, Line):
                line = item.line()
                stream << "Line" << "{},{},{},{}".format(
                        line.x1(), line.y1(), line.x2(), line.y2()) \
                    << "{}".format(item.length()) \
                    << "{}".format(item.widthF()) \
                    << "{}".format(item.style()) \
                    << item.color().name() \
                    << "\n"
            elif isinstance(item, Rectangle):
                rect = item.rect()
                topLeft = rect.topLeft()
                stream << "Rectangle" << "{},{},{},{}".format(
                    topLeft.x(), topLeft.y(), rect.width(), rect.height()) \
                    << "\n"
            elif isinstance(item, GraphicsProxyWidgetContainer):
                print "GraphicsProxyWidgetContainer"
            elif isinstance(item, GraphicsProxyWidget):
                print "GraphicsProxyWidget"
                embeddedWidget = item.widget()



    # The copied item data is extracted and the items are instantiated with the
    # refered binary data
    def paste(self):
        if len(self.__copiedItem) < 1:
            return

        stream = QDataStream(self.__copiedItem, QIODevice.ReadOnly)

        itemData = [ ]
        while not stream.atEnd():
            input = stream.readString()

            if input == "\n":
                # Create item
                type = itemData[0]
                if type == "Text" and len(itemData) == 6:
                    textItem = Text(self.__isDesignMode)
                    textItem.setText(itemData[1])
                    font = QFont()
                    font.fromString(itemData[2])
                    textItem.setFont(font)
                    textItem.setTextColor(QColor(itemData[3]))
                    textItem.setOutlineColor(QColor(itemData[4]))
                    textItem.setBackgroundColor(QColor(itemData[5]))
                    self._setupItem(textItem)
                elif type == "Link":
                    print "Link"
                elif type == "Arrow":
                    print "Arrow"
                elif type == "Line" and len(itemData) == 6:
                    lineItem = Line(self.__isDesignMode)
                    # Get line coordinates
                    lineCoords = itemData[1].split(",")
                    if len(lineCoords) == 4:
                        line = QLineF(lineCoords[0].toFloat()[0], lineCoords[1].toFloat()[0], \
                                      lineCoords[2].toFloat()[0], lineCoords[3].toFloat()[0])
                        lineItem.setLine(line)
                    # Get line length
                    length = float(itemData[2])
                    lineItem.setLength(length[0])
                    # Get line width
                    widthF = float(itemData[3])
                    lineItem.setWidthF(widthF[0])
                    # Get line style
                    style = int(itemData[4])
                    lineItem.setStyle(style[0])
                    # Get line color
                    lineItem.setColor(QColor(itemData[5]))

                    self._setupItem(lineItem)
                    lineItem.setTransformOriginPoint(lineItem.boundingRect().center())
                elif type == "Rectangle" and len(itemData) == 2:
                    rectItem = Rectangle(self.__isDesignMode)
                    rectData = itemData[1].split(",")
                    if len(rectData) == 4:
                        rect = QRectF(rectData[0].toFloat()[0], rectData[1].toFloat()[0], \
                                      rectData[2].toFloat()[0], rectData[3].toFloat()[0])
                        rectItem.setRect(rect)
                    self._setupItem(rectItem)
                    rectItem.setTransformOriginPoint(rectItem.boundingRect().center())
                elif type == "GraphicsProxyWidgetContainer":
                    print "GraphicsProxyWidgetContainer"
                elif type == "GraphicsProxyWidget":
                    print "GraphicsProxyWidget"

                itemData = [ ]
                continue

            itemData.append(input)


    # All selected items are removed; when an item (not type Link) is removed its
    # destructor deletes any links that are associated with it
    # To avoid double-deleting links, the Link-items are removed before deleting the other items
    def remove(self):
        items = self.selectedItems()
        if (len(items) and QMessageBox.question(self, "Remove selected items",
                                                "Remove {0} item{1}?".format(len(items),
                                                "s" if len(items) != 1 else ""),
                                                QMessageBox.Yes|QMessageBox.No) ==
                                                QMessageBox.No):
            return

        self.removeItems(items)


    def removeItems(self, items):
        while items:
            item = items.pop()
            if isinstance(item, Text) or isinstance(item, Rectangle):
                for link in item.links():
                    if link in items:
                        # Remove item from list - prevent double deletion
                        items.remove(link)
            elif isinstance(item, Link) or isinstance(item, Arrow):
                print "Link or Arrow removed"
            elif isinstance(item, Line):
                print "Line removed"
            elif isinstance(item, GraphicsProxyWidgetContainer):
                layout = item.layout()
                while layout.count() > 0:
                    proxyItem = layout.itemAt(layout.count()-1)
                    if not proxyItem:
                        continue
                    
                    keys = proxyItem.keys
                    if keys:
                        for key in keys:
                            Manager().removeVisibleDevice(key)
                    # Destroy and unregister
                    proxyItem.destroy()
                    if proxyItem in items:
                        # Remove item from list - prevent double deletion
                        items.remove(proxyItem)
                    
                    self.__scene.removeItem(proxyItem)
                    layout.removeItem(proxyItem)
                    
                item.destroy()
            elif isinstance(item, GraphicsProxyWidget):
                keys = item.keys
                if keys:
                    for key in keys:
                        Manager().removeVisibleDevice(key)
                # Destroy and unregister
                item.destroy()
            elif isinstance(item, GraphicsCustomItem):
                for inputItem in item.inputChannelItems():
                    if inputItem in items:
                        # Remove item from list - prevent double deletion
                        items.remove(inputItem)
                    self.__scene.removeItem(inputItem)
                for outputItem in item.outputChannelItems():
                    if outputItem in items:
                        # Remove item from list - prevent double deletion
                        items.remove(outputItem)
                    self.__scene.removeItem(outputItem)
                Manager().removeVisibleDevice(item.internalKey())
                Manager().unregisterEditableComponent(item.deviceIdKey, item)

            self.__scene.removeItem(item)
            del item


    def groupItems(self):
        items = self.selectedItems()
        if len(items) < 1:
            return

        # Unselect all selected items
        for item in items:
            item.setSelected(False)

        itemGroup = self.__scene.createItemGroup(items)
        itemGroup.setFlags(QGraphicsItem.ItemIsMovable | QGraphicsItem.ItemIsSelectable)
        itemGroup.setSelected(True)
        self.bringToFront()


    def horizontalLayout(self):
        items = self.selectedItems()
        if len(items) < 1:
            return
        self.createGraphicsItemContainer(Qt.Horizontal, items)


    def verticalLayout(self):
        items = self.selectedItems()
        if len(items) < 1:
            return
        self.createGraphicsItemContainer(Qt.Vertical, items)


    def breakLayout(self):
        items = self.selectedItems()
        if len(items) < 1:
            return
        for item in items:
            self.__scene.breakLayout(item)


    def unGroupItems(self):
        items = self.selectedItems()
        if len(items) < 1:
            return

        childItems = []
        for item in items:
            if isinstance(item, QGraphicsItemGroup):
                childItems = item.childItems()
                self.__scene.destroyItemGroup(item)
        # Select all items again
		for childItem in childItems:
		    childItem.setSelected(True)



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
        self.layout.add_item(layout)
        layout.selected = True
        return layout

    def clear_selection(self):
        for s in self.layout.shapes:
            s.selected = False
        for c in self.layout:
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


# Drag & Drop events
    def dragEnterEvent(self, event):
        #print "GraphicsView.dragEnterEvent"

        source = event.source()
        if source is not None and source is not self and self.designMode:
            event.accept()

        QWidget.dragEnterEvent(self, event)


    def dragMoveEvent(self, event):
        #print "GraphicsView.dragMoveEvent"
        event.accept()
        #QGraphicsView.dragMoveEvent(self, event)


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
                self.layout.shapes.append(customItem)
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


### slots ###
    def onRemoveUserCustomFrame(self, userCustomFrame):
        print "onRemoveUserCustomFrame", userCustomFrame
        if userCustomFrame is None:
            return
        userCustomFrame.deleteLater()


    # Called whenever an item of the scene is un-/selected
    def onSceneSelectionChanged(self):
        self.sceneSelectionChanged.emit()

        if (len(self.__scene.selectedItems()) == 1):
            selectedItem = self.__scene.selectedItems()[0]
            if isinstance(selectedItem, GraphicsCustomItem):
                Manager().selectNavigationItemByKey(selectedItem.internalKey())
            elif isinstance(selectedItem, GraphicsProxyWidget):
                keys = selectedItem.keys
                if keys:
                    for key in keys:
                        Manager().selectNavigationItemByKey(key)
            elif isinstance(selectedItem, GraphicsProxyWidgetContainer):
                layout = selectedItem.layout()
                for i in xrange(layout.count()):
                    proxyItem = layout.itemAt(i)
                    keys = proxyItem.keys
                    if keys:
                        for key in keys:
                            Manager().selectNavigationItemByKey(key)

    def paintEvent(self, event):
        painter = QPainter(self)
        try:
            self.renderer().render(painter, self.renderer().viewBoxF())
            self.layout.draw(painter)
            painter.save()
            if self.designMode:
                painter.setPen(Qt.DashLine)
                for item in self.layout:
                    if item.selected:
                        painter.drawRect(item.geometry())
            painter.restore()
            if self.current_action.ready:
                self.current_action.draw(painter)
        finally:
            painter.end()
