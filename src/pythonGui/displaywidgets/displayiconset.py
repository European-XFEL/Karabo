from const import ns_karabo
from widget import DisplayWidget

from PyQt4.QtCore import pyqtSlot, QBuffer
from PyQt4.QtGui import QAction, QFileDialog
from PyQt4.QtSvg import QSvgWidget

from xml.etree import ElementTree
import re

ns_inkscape = "{http://www.inkscape.org/namespaces/inkscape}"
ElementTree.register_namespace("inkscape", ns_inkscape[1:-1])


class Element(ElementTree.Element):
    def __init__(self, tag, attrib={}, **extra):
        super(Element, self).__init__(tag, attrib, **extra)
        label = self.get(ns_inkscape + 'label')
        if label is not None:
            self.label = re.compile(label)
        else:
            self.label = None
        self.filter = ""


    def __iter__(self):
        for i in range(len(self)):
            if self[i].label is None or self[i].label.match(self.filter):
                yield self[i]


class DisplayIconset(DisplayWidget):
    category = "State"
    alias = "Iconset"

    def __init__(self, box, parent):
        self.widget = QSvgWidget(parent)
        action = QAction("Change Iconset...", self.widget)
        action.triggered.connect(self.onChangeIcons)
        self.widget.addAction(action)
        self.xml = ElementTree.ElementTree(ElementTree.fromstring(
            '<svg xmlns:svg="http://www.w3.org/2000/svg"/>'))
        self.filename = None
        super(DisplayIconset, self).__init__(box)


    def save(self, e):
        if self.filename is not None:
            e.set(ns_karabo + "filename", self.filename)


    def load(self, e):
        name = e.get(ns_karabo + "filename")
        self.valueChanged(None, "")
        if name is None:
            self.filename = None
        else:
            self.setFilename(e.get(ns_karabo + "filename"))


    @pyqtSlot()
    def onChangeIcons(self):
        fn = QFileDialog.getOpenFileName(self.widget, "Open Iconset",
                                         filter="*.svg")
        if not fn:
            return
        self.setFilename(fn)


    def setFilename(self, fn):
        self.filename = fn
        parser = ElementTree.XMLParser(target=ElementTree.TreeBuilder(
            element_factory=Element))
        value = self.xml.getroot().filter
        self.xml = ElementTree.ElementTree()
        self.xml.parse(fn, parser)
        self.valueChanged(None, value)


    def valueChanged(self, box, value, timestamp=None):
        self.xml.getroot().filter = value
        buffer = QBuffer()
        buffer.open(QBuffer.WriteOnly)
        self.xml.write(buffer)
        buffer.close()
        self.widget.load(buffer.buffer())

