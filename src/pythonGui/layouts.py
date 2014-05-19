#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created on February 27, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


from dialogs.dialogs import TextDialog
import manager
from registry import Loadable
from const import ns_svg, ns_karabo

from PyQt4.QtCore import pyqtSlot, QRect, QSize
from PyQt4.QtGui import (QAction, QBoxLayout, QFrame, QGridLayout, QLabel,
                         QLayout, QPalette, QMenu, QStackedLayout, QWidget)

from bisect import bisect
from functools import partial
from xml.etree import ElementTree


def _parse_rect(elem):
    if elem.get(ns_karabo + "x") is not None:
        ns = ns_karabo
    else:
        ns = ""
    return QRect(float(elem.get(ns + "x")), float(elem.get(ns + "y")),
                 float(elem.get(ns + "width")), float(elem.get(ns + "height")))


class Layout(Loadable):
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
        return r.widget() if r.widget() is not None else r


    def iterWidgets(self, selected=False):
        """iterates over all widgets contained in this layout,
        or only selected ones if selected is True."""
        for wl in self:
            if not selected or wl.selected:
                if isinstance(wl, Layout):
                    for r in wl.iterWidgets():
                        yield r
                else:
                    yield wl


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
            if r is None or len(element[i]):
                i += 1
            else:
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


    def set_geometry(self, rect):
        self.fixed_geometry = rect


    def setGeometry(self, rect):
        super(Layout, self).setGeometry(rect)
        if self.shape_geometry is None:
            self.shape_geometry = QRect(self.fixed_geometry)
        for s in self.shapes:
            s.translate(rect.topLeft() - self.shape_geometry.topLeft())
        self.shape_geometry = QRect(rect)


    def edit(self):
        pass


class FixedLayout(Layout, QLayout):
    xmltag = ns_svg + "g"

    def __init__(self):
        QLayout.__init__(self)
        Layout.__init__(self)
        self._children = [ ] # contains only QLayoutItems
        self.entire = None


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


    def loadPosition(self, element, item):
        self.add_item(item)
        if element.get(ns_karabo + 'entire') is not None:
            self.entire = item
        try:
            item.fixed_geometry = _parse_rect(element)
        except TypeError:
            pass


    def add_children(self, e, selected=False):
        for c in self:
            if not selected or c.selected:
                ee = c.element()
                if self.entire == c:
                    ee.set('entire', 'True')
                e.append(ee)
        e.extend(s.element() for s in self.shapes if not selected or s.selected)


    def itemAtPosition(self, pos):
        for item in self._children:
            if item.geometry().contains(pos):
                if item.widget() is not None:
                    return item.widget()
                else:
                    return item
        for s in self.shapes[::-1]:
            if s.contains(pos) or s.selected and s.geometry().contains(pos):
                return s


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
                        if p.component is not None:
                            for b in p.component.boxes:
                                b.configuration.removeVisible()
                    # the following line also deletes the item from
                    # this layout. This is why we don't need to increase i
                    p.setParent(None)
            else:
                i += 1
        self.shapes = [s for s in self.shapes if not s.selected]


    def geometry(self):
        if self.entire is not None:
            return self.entire.geometry()
        try:
            return self.fixed_geometry
        except AttributeError:
            return self.parentWidget().geometry()


    def translate(self, pos):
        for c in self:
            c.fixed_geometry.translate(pos)
        for s in self.shapes:
            s.translate(pos)
        Layout.translate(self, pos)


    def save(self):
        return { }


    @staticmethod
    def load(elem, layout=None, widget=None):
        ret = FixedLayout()
        if widget is None:
            layout.loadPosition(elem, ret)
        else:
            widget.setLayout(ret)
        ret.load_element(elem)
        if not hasattr(ret, 'fixed_geometry'):
            rect = QRect()
            for c in ret:
                rect.united(c.geometry())
            for s in ret.shapes:
                rect = rect.united(s.geometry())
            ret.fixed_geometry = rect
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
        if self.entire is not None:
            self.entire.fixed_geometry = geometry
        for item in self._children:
            i = item.widget() if item.widget() is not None else item
            i.setGeometry(i.fixed_geometry)


    def sizeHint(self):
        if self.entire is not None:
            return self.entire.sizeHint()
        return QSize(10, 10)


class BoxLayout(Layout, QBoxLayout):
    def __init__(self, dir):
        QBoxLayout.__init__(self, dir)
        Layout.__init__(self)
        self.setContentsMargins(5, 5, 5, 5)


    def save(self):
        return {ns_karabo + "direction": self.direction()}


    @staticmethod
    def load(elem, layout):
        ret = BoxLayout(int(elem.get(ns_karabo + "direction")))
        layout.loadPosition(elem, ret)
        ret.load_element(elem)
        return ret


    def loadPosition(self, element, item):
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


class GridLayout(Layout, QGridLayout):
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


    def loadPosition(self, element, item):
        p = (int(element.get(ns_karabo + s)) for s in
             ("row", "col", "rowspan", "colspan"))
        if isinstance(item, ProxyWidget):
            self.addWidget(item, *p)
        else:
            self.addLayout(item, *p)


    @staticmethod
    def load(elem, layout):
        ret = GridLayout()
        layout.loadPosition(elem, ret)
        ret.load_element(elem)
        return ret


class ProxyWidget(QWidget):
    def __init__(self, parent):
        QWidget.__init__(self, parent)
        QStackedLayout(self).setStackingMode(QStackedLayout.StackAll)
        self.selected = False
        self.component = None
        self.marker = QLabel("", self)
        self.layout().addWidget(self.marker)
        self.widget = None


    def setComponent(self, component):
        self.component = component

        box = self.component.boxes[0]
        self.setToolTip(box.key())
        box.configuration.statusChanged.connect(self.showStatus)
        self.showStatus(None, box.configuration.status)

        for text, factory in component.factories.iteritems():
            aliases = factory.getAliasesViaCategory(
                component.widgetCategory)
            if component.boxes[0].path == ('state',):
                aliases = aliases + factory.getAliasesViaCategory("State")
            if aliases:
                aa = QAction(text, self)
                menu = QMenu(self)
                for a in aliases:
                    menu.addAction(a).triggered.connect(
                        partial(self.on_changeWidget, factory, a))
                aa.setMenu(menu)
                self.addAction(aa)


    def showStatus(self, configuration, status):
        if status == "alive":
            self.marker.hide()
        else:
            self.marker.setText(status)
            self.marker.show()


    def setWidget(self, widget):
        if self.layout().count() > 1:
            self.layout().takeAt(0)
        self.layout().insertWidget(0, widget)
        self.widget = widget


    @pyqtSlot()
    def on_changeWidget(self, factory, alias):
        self.component.changeWidget(factory, alias)
        self.parent().layout().relayout(self)
        self.adjustSize()

    def contextMenuEvent(self, event):
        if not self.parent().parent().designMode:
            return
        QMenu.exec_(self.widget.actions() + self.actions(),
                    event.globalPos(), None, self)

    def element(self):
        g = self.geometry()
        d = { "x": g.x(), "y": g.y(), "width": g.width(), "height": g.height() }
        if self.component is None:
            w = self.widget
            d[ns_karabo + "class"] = "Label"
            d[ns_karabo + "text"] = w.text()
            d[ns_karabo + "font"] = w.font().toString()
            d[ns_karabo + "foreground"] = "#{:06x}".format(
                w.palette().color(QPalette.Foreground).rgb() & 0xffffff)
            if w.autoFillBackground():
                d[ns_karabo + "background"] = "#{:06x}".format(
                    w.palette().color(QPalette.Background).rgb() & 0xffffff)
            if w.frameShape() == QFrame.Box:
                d[ns_karabo + 'frameWidth'] = unicode(w.lineWidth())
        ret = ElementTree.Element(ns_svg + "rect",
                                  {k: unicode(v) for k, v in d.iteritems()})
        if self.component is not None:
            self.component.save(ret)
        return ret


    def translate(self, pos):
        self.fixed_geometry.translate(pos)
        self.parent().layout().update()


    def set_geometry(self, rect):
        self.fixed_geometry = rect


    def edit(self):
        if isinstance(self.widget, QLabel):
            dialog = TextDialog(self.widget)
            dialog.exec_()


    def dropEvent(self, event):
        source = event.source()
        if source is None:
            return
        for item in source.selectedItems():
            if (self.component is not None and
                    self.component.addBox(item.internalKey)):
                item.internalKey.configuration.addVisible()
                event.accept()

