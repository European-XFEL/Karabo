from const import ns_karabo
from widget import DisplayWidget

from karabo.hashtypes import String

from PyQt4.QtCore import pyqtSlot, QBuffer
from PyQt4.QtGui import QAction, QFileDialog, QInputDialog, QMessageBox
from PyQt4.QtSvg import QSvgWidget

from xml.etree import ElementTree
import re
import os.path
import urllib.request

ns_inkscape = "{http://www.inkscape.org/namespaces/inkscape}"
ElementTree.register_namespace("inkscape", ns_inkscape[1:-1])


class Element(ElementTree.Element):
    def __init__(self, tag, attrib={}, **extra):
        super(Element, self).__init__(tag, attrib, **extra)
        label = self.get(ns_inkscape + 'label')
        if label is not None:
            self.set(">label", re.compile(label))

    def __iter__(self):
        for i in range(len(self)):
            e = self[i]
            l = e.get(">label")
            filter = self.get(">filter")
            if l is None or l.match(filter):
                yield e

    def items(self):
        for k, v in super().items():
            if k[0] != ">":
                yield k, v


class DisplayIconset(DisplayWidget):
    category = String
    alias = "Iconset"

    def __init__(self, box, parent):
        super(DisplayIconset, self).__init__(box)

        self.widget = QSvgWidget(parent)
        action = QAction("Iconset from file...", self.widget)
        action.triggered.connect(self.onChangeIcons)
        self.widget.addAction(action)
        action = QAction("Iconset from URL...", self.widget)
        action.triggered.connect(self.onChangeURL)
        self.widget.addAction(action)
        self.toProject = QAction("Copy iconset to project", self.widget)
        self.toProject.triggered.connect(self.onToProject)
        self.widget.addAction(action)
        self.setURL("file://" + urllib.request.pathname2url(
            os.path.join(os.path.dirname(__file__), "empty.svg")))

    def save(self, e):
        if self.url is not None:
            e.set(ns_karabo + "url", self.url)

    def load(self, e):
        url = e.get(ns_karabo + "url")
        if url is None:
            name = e.get(ns_karabo + "filename")
            if name is not None:
                url = urllib.request.pathname2url(name)
        self.valueChanged(None, "")
        if url is None:
            self.url = None
        else:
            try:
                self.setURL(url)
            except KeyError:
                QMessageBox.warning(None, "Resource not found",
                                    'could not find iconset for "{}"'.
                                    format(self.boxes[0].key()))
                self.url = None

    @pyqtSlot()
    def onChangeIcons(self):
        fn = QFileDialog.getOpenFileName(self.widget, "Open Iconset",
                                         filter="*.svg")
        if not fn:
            return
        self.setURL("file://" + urllib.request.pathname2url(fn))

    @pyqtSlot()
    def onChangeURL(self):
        url, ok = QInputDialog.getText(self.widget, "Set URL",
                                       "New iconset URL:", text=self.url)
        if ok:
            self.setURL(url)

    @pyqtSlot()
    def onToProject(self):
        self.setURL(self.project.addResource("iconset",
                                             self.project.getURL(self.url)))

    def setURL(self, url):
        if url.startswith("file:"):
            url = self.project.addResource("iconset", self.project.getURL(url))
        self.url = url
        parser = ElementTree.XMLParser(target=ElementTree.TreeBuilder(
            element_factory=Element))
        parser.feed(self.project.getURL(self.url))
        self.xml = ElementTree.ElementTree(parser.close())
        self.valueChanged(None, self.boxes[0].value if self.boxes[0].hasValue()
                          else "")
        self.toProject.setEnabled(not url.startswith("project:"))

    def valueChanged(self, box, value, timestamp=None):
        self.xml.getroot().set(">filter", value)
        buffer = QBuffer()
        buffer.open(QBuffer.WriteOnly)
        self.xml.write(buffer)
        buffer.close()
        self.widget.load(buffer.buffer())
