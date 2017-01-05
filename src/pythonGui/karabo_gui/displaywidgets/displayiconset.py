import re
import os.path
import urllib.request
from xml.etree import ElementTree

from PyQt4.QtCore import pyqtSlot, QBuffer
from PyQt4.QtGui import QAction, QInputDialog
from PyQt4.QtSvg import QSvgWidget

from karabo.middlelayer import String
from karabo_gui.util import getOpenFileName
from karabo_gui.widget import DisplayWidget

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
        qaction = QAction("Iconset from file...", self.widget)
        qaction.triggered.connect(self.onChangeIcons)
        self.widget.addAction(qaction)
        qaction = QAction("Iconset from URL...", self.widget)
        qaction.triggered.connect(self.onChangeURL)
        self.widget.addAction(qaction)

        # URL of the icon set
        self.url = None
        # XMLParser object to get associated layer for property value
        self.xml = None

    def _readData(self, url, data=None):
        if data is None:
            data = urllib.request.urlopen(url).read()
            self.setData(url, data)
        parser = ElementTree.XMLParser(target=ElementTree.TreeBuilder(
            element_factory=Element))
        parser.feed(data)
        return ElementTree.ElementTree(parser.close())

    def setURL(self, url):
        self.url = url
        self.xml = self._readData(url)
        self.valueChanged(None, self.boxes[0].value if self.boxes[0].hasValue()
                          else "")

    def setData(self, url, data):
        """ The `url` and the actual `data` is passed and needs to be set. """
        if not url and not data:
            # Use default icon set for initialization
            self.setURL("file://" + urllib.request.pathname2url(
                os.path.join(os.path.dirname(__file__), "empty.svg")))
            return
        self.url = url
        self.xml = self._readData(url, data)
        self.valueChanged(None, self.boxes[0].value if self.boxes[0].hasValue()
                          else "")

    @pyqtSlot()
    def onChangeIcons(self):
        fn = getOpenFileName(parent=self.widget,
                             caption="Open Iconset",
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

    def valueChanged(self, box, value, timestamp=None):
        self.xml.getroot().set(">filter", value)
        buffer = QBuffer()
        buffer.open(QBuffer.WriteOnly)
        self.xml.write(buffer)
        buffer.close()
        self.widget.load(buffer.buffer())
