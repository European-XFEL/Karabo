#############################################################################
# Author: <martin.teichmann@xfel.eu>
# Created on February 27, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


import karabo_gui.icons as icons
from karabo_gui.registry import Loadable
from karabo_gui.const import ns_svg, ns_karabo
import karabo_gui.sceneitems as sceneitems
from karabo_gui.topology import getDeviceBox

from PyQt4.QtCore import pyqtSlot, QPoint, QRect, QSize, Qt
from PyQt4.QtGui import (QAction, QBoxLayout, QGridLayout, QLabel,
                         QLayout, QMenu, QStackedLayout, QWidget)

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


    def __init__(self, *args, **kwargs):
        super(Layout, self).__init__(*args, **kwargs)
        self.shapes = [ ]
        self.shape_geometry = None
        self.fixed_geometry = None
        self.selected = False


    def __len__(self):
        return self.count()


    def __getitem__(self, i):
        if isinstance(i, slice):
            r = (self.itemAt(j) for j in range(*i.indices(len(self))))
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
            for j in range(*i.indices(len(self))):
                self.takeAt(j)
        else:
            self.takeAt(i)


    def load_element(self, element):
        """try to load all sub-elements from *element*

        Sub-Elements that we were able to load are deleted from *element*,
        those that we couldn't load are kept for others to show
        them (Qt that is). """
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
                                {k: str(v) for k, v in d.items()})
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
        "only to be used by Qt, don't use directly!"
        super(Layout, self).setGeometry(rect)
        if self.shape_geometry is None:
            if self.fixed_geometry is None:
                self.shape_geometry = QRect(rect)
            else:
                self.shape_geometry = QRect(self.fixed_geometry)
        for s in self.shapes:
            s.translate(rect.topLeft() - self.shape_geometry.topLeft())
        self.shape_geometry = QRect(rect)


    def edit(self):
        pass


class FixedLayout(Layout, QLayout):
    xmltag = ns_svg + "g"

    def __init__(self):
        super(FixedLayout, self).__init__()
        self._children = [ ] # contains only QLayoutItems
        self.entire = None
        self.__item = None

    def __setitem__(self, key, value):
        if not isinstance(key, slice):
            key = slice(key, key + 1)
            value = [value]
        values = [ ]
        for v in value:
            if isinstance(v, SceneWidget):
                # Qt creates a QLayoutItem and calls addItem.
                # that sets self.__item
                self.addWidget(v)
                assert self.__item is not None
                values.append(self.__item)
                self.__item = None
            elif isinstance(v, Layout):
                self.addChildLayout(v)
                values.append(v)
        self._children[key] = values
        self.update()

    def addItem(self, item):
        """only to be used by Qt, don't use directly!"""
        # The job of addItem should be to insert the item into our list
        # unfortunately, we don't know *where* to insert it, so we secretly
        # store the item and let the caller (__setitem__ that is) deal with it.
        assert self.__item is None
        self.__item = item

    def add_item(self, item):
        """add ProxyWidgets or Layouts"""
        self[len(self):len(self)] = [item]


    def remove_item(self, item):
        """
        The given \item is removed from the layout.
        """
        i = 0
        while i < len(self):
            c = self[i]
            if c == item:
                self.clear_layout(c)
                break
            i += 1


    def remove_selected(self):
        """
        All selected items are removed from the layout.
        """
        i = 0
        while i < len(self):
            if self[i].selected:
                self.clear_layout(self[i])
            else:
                i += 1
        self.shapes = [s for s in self.shapes if not s.selected]


    def clear_layout(self, layoutItem):
        """
        The given \layoutItem is removed.
        """
        stack = [layoutItem]
        while stack:
            p = stack.pop()
            if isinstance(p, Layout):
                stack.extend(p)
            elif isinstance(p, SceneWidget):
                if p.scene.tabVisible:
                    if isinstance(p, ProxyWidget):
                        if p.component is not None:
                            for b in p.component.boxes:
                                b.removeVisible()
                    else:
                        if isinstance(p.widget, sceneitems.workflowitems.Item):
                            p.widget.getDevice().removeVisible()
            # the following line also deletes the item from
            # this layout. This is why we don't need to increase i
            p.setParent(None)


    def loadPosition(self, element, item):
        if element.get(ns_karabo + 'entire') is not None:
            self.entire = item
        try:
            item.fixed_geometry = _parse_rect(element)
        except TypeError:
            pass
        self.add_item(item)


    def add_children(self, e, selected=False):
        """save the children of this layout into the element *e*"""
        for c in self:
            if not selected or c.selected:
                ee = c.element()
                if ee is None:
                    # Could be none, in case no save function is implemented
                    continue
                if self.entire == c:
                    ee.set('entire', 'True')
                e.append(ee)
        e.extend(s.element() for s in self.shapes if not selected or s.selected)


    def itemAtPosition(self, pos):
        for item in self._children[::-1]:
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


    def geometry(self):
        if self.entire is not None:
            return self.entire.geometry()
        if self.fixed_geometry is None:
            return self.parentWidget().geometry()
        else:
            return self.fixed_geometry


    def translate(self, pos):
        for c in self:
            c.fixed_geometry.translate(pos)
        super(FixedLayout, self).translate(pos)


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
        if ret.fixed_geometry is None:
            rect = QRect()
            for c in ret:
                rect.united(c.geometry())
            for s in ret.shapes:
                rect = rect.united(s.geometry())
            ret.fixed_geometry = rect
        return ret

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
            l = ret.layout()
            if l is not None and l.parent() is self:
                l.setParent(None)
            return ret
        except IndexError:
            return


    def count(self):
        "only to be used by Qt, don't use directly!"
        return len(self._children)


    def setGeometry(self, rect):
        "only to be used by Qt, don't use directly!"
        super(FixedLayout, self).setGeometry(rect)
        if self.entire is not None:
            self.entire.fixed_geometry = rect
        if self.fixed_geometry is None:
            translation = QPoint(0, 0)
        else:
            translation = rect.topLeft() - self.fixed_geometry.topLeft()
        for c in self:
            c.fixed_geometry.translate(translation)
            c.setGeometry(c.fixed_geometry)
        self.fixed_geometry = QRect(rect)


    def sizeHint(self):
        if self.entire is not None:
            return self.entire.sizeHint()
        elif self.fixed_geometry is not None:
            return self.fixed_geometry.size()
        return QSize(10, 10)


class BoxLayout(Layout, QBoxLayout):
    def __init__(self, direction):
        super(BoxLayout, self).__init__(direction)
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
        if isinstance(item, SceneWidget):
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
            if isinstance(c, SceneWidget):
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
                c.set(ns_karabo + n, str(v))
            e.append(c)


    def loadPosition(self, element, item):
        p = (int(element.get(ns_karabo + s)) for s in
             ("row", "col", "rowspan", "colspan"))
        if isinstance(item, SceneWidget):
            self.addWidget(item, *p)
        else:
            self.addLayout(item, *p)


    @staticmethod
    def load(elem, layout):
        ret = GridLayout()
        layout.loadPosition(elem, ret)
        ret.load_element(elem)
        return ret


class SceneWidget(QWidget):
    def __init__(self, parent):
        super(SceneWidget, self).__init__(parent)
        QStackedLayout(self).setStackingMode(QStackedLayout.StackAll)
        
        self.selected = False
        self.marker = QLabel("", self)
        self.marker.setAttribute(Qt.WA_TransparentForMouseEvents)
        self.layout().addWidget(self.marker)
        
        self.widget = None

    @pyqtSlot(object, str, bool)
    def showStatus(self, configuration, status, error):
        if status == "monitoring" and not error:
            self.marker.hide()
        else:
            if status != "offline" and error:
                icon = icons.device_error
            else:
                icon = dict(requested=icons.device_requested,
                            schema=icons.device_schema,
                            dead=icons.device_dead,
                            noserver=icons.deviceOfflineNoServer,
                            noplugin=icons.deviceOfflineNoPlugin,
                            incompatible=icons.deviceIncompatible,
                            offline=icons.deviceOffline,
                            alive=icons.deviceAlive,
                            missing=icons.propertyMissing).get(status)
            if icon is not None:
                self.marker.setPixmap(icon.pixmap(16))
                self.marker.setText("")
            else:
                self.marker.setText(status)
            self.marker.show()

    def setWidget(self, widget):
        if self.layout().count() > 1:
            self.layout().takeAt(0)
        self.layout().insertWidget(0, widget)
        self.widget = widget

    def createElement(self):
        g = self.geometry()
        d = dict(x=g.x(), y=g.y(), width=g.width(), height=g.height())
        if g.x() == 0 and g.y() == 0 and g.width() == 100 and g.height() == 30:
            raise RuntimeError("lost geometry data")
        ret = ElementTree.Element(ns_svg + "rect",
                                  {k: str(v) for k, v in d.items()})
        
        return ret

    def element(self):
        if not hasattr(self.widget, "save"):
            return None
            
        ret = self.createElement()
        self.widget.save(ret)
        
        return ret

    def translate(self, pos):
        self.fixed_geometry.translate(pos)
        self.parent().layout().update()

    def set_geometry(self, rect):
        self.fixed_geometry = rect
        
    @property
    def scene(self):
        return self.parent().parent()


class ProxyWidget(SceneWidget):
    def __init__(self, parent):
        super(ProxyWidget, self).__init__(parent)
        self.component = None

    def setComponent(self, component):
        self.component = component

        box = self.component.boxes[0]
        self.setToolTip(box.key())
        box.configuration.signalStatusChanged.connect(self.showStatus)
        self.showStatus(None, box.configuration.status, box.configuration.error)
        box.signalNewDescriptor.connect(self.setDescriptor)
        if box.descriptor is not None:
            self.setDescriptor(box)

    def setDescriptor(self, box):
        classes = self.component.Widget.getClasses(box)
        menus = {}
        for c in classes:
            menus.setdefault(c.menu, []).append(c)
        for text, classes in menus.items():
            if classes:
                aa = QAction(text, self)
                menu = QMenu(self)
                for c in classes:
                    menu.addAction(c.alias).triggered.connect(
                        partial(self.on_changeWidget, c))
                aa.setMenu(menu)
                self.addAction(aa)


    @pyqtSlot(object, str, bool)
    def showStatus(self, configuration, status, error):
        if status == "monitoring" and not error:
            if self.component is not None:
                for b in self.component.boxes:
                    if not b.hasValue():
                        status = "missing"
        super(ProxyWidget, self).showStatus(configuration, status, error)

    @pyqtSlot()
    def on_changeWidget(self, factory):
        self.component.changeWidget(factory)
        self.parent().layout().relayout(self)
        self.adjustSize()

    def contextMenuEvent(self, event):
        if not self.scene.designMode:
            return
        QMenu.exec_(self.widget.actions() + self.actions(),
                    event.globalPos(), None, self)

    def element(self):
        if self.component is None:
            return None
        
        ret = self.createElement()
        self.component.save(ret)

        return ret

    def dropEvent(self, event):
        source = event.source()
        if source is None:
            return
        for item in source.selectedItems():
            if (self.component is not None and
                    self.component.addBox(getDeviceBox(item.box))):
                if self.scene.tabVisible:
                    item.box.addVisible()
                event.accept()

