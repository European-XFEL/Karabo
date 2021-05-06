import os.path as op
import re
import urllib.request
from xml.etree.ElementTree import (
    Element, ElementTree, TreeBuilder, XMLParser, register_namespace)

from qtpy.QtCore import QBuffer
from qtpy.QtSvg import QSvgWidget
from qtpy.QtWidgets import QAction, QInputDialog
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import DisplayIconsetModel
from karabogui.binding.api import StringBinding, get_binding_value
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.util import getOpenFileName

DEFAULT_ICON_PATH = op.join(op.dirname(__file__), 'empty.svg')
DEFAULT_ICON_URL = 'file://' + urllib.request.pathname2url(DEFAULT_ICON_PATH)
NS_INKSCAPE = "{http://www.inkscape.org/namespaces/inkscape}"
register_namespace('inkscape', NS_INKSCAPE[1:-1])


class _MyElement(Element):
    def __init__(self, tag, attrib={}, **extra):
        super(_MyElement, self).__init__(tag, attrib, **extra)
        label = self.get(NS_INKSCAPE + 'label')
        if label is not None:
            self.set('>label', re.compile(label))

    def __iter__(self):
        for i in range(len(self)):
            elem = self[i]
            label = elem.get('>label')
            filt = self.get('>filter')
            if label is None or label.match(filt):
                yield elem

    def items(self):
        for k, v in super().items():
            if k[0] != '>':
                yield k, v


def _read_xml_data(data):
    target = TreeBuilder(element_factory=_MyElement)
    parser = XMLParser(target=target)
    parser.feed(data)

    return ElementTree(parser.close())


@register_binding_controller(ui_name='Iconset', klassname='DisplayIconset',
                             binding_type=StringBinding)
class DisplayIconset(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(DisplayIconsetModel, args=())
    # XMLParser object to get associated layer for property value
    xml = Instance(ElementTree, allow_none=True)

    def create_widget(self, parent):
        widget = QSvgWidget(parent)
        qaction = QAction('Iconset from file...', widget)
        qaction.triggered.connect(self._change_iconset_file)
        widget.addAction(qaction)
        qaction = QAction('Iconset from URL...', widget)
        qaction.triggered.connect(self._change_iconset_url)
        widget.addAction(qaction)
        return widget

    def value_update(self, proxy):
        self._xml_changed()

    def _xml_changed(self):
        if self.widget is None:
            return

        # It's possible to get here before the binding is initialized
        value = get_binding_value(self.proxy)
        if value is None:
            return

        self.xml.getroot().set('>filter', value)
        buffer = QBuffer()
        buffer.open(QBuffer.WriteOnly)
        self.xml.write(buffer)
        buffer.close()
        self.widget.load(buffer.buffer())

    @on_trait_change('model.image')
    def _icon_url_update(self):
        if self.model.image:
            self.model.data = urllib.request.urlopen(self.model.image).read()

    @on_trait_change('model.data')
    def _icon_data_update(self):
        if self.model.data:
            self.xml = _read_xml_data(self.model.data)

    def _widget_changed(self):
        """Called after the widget is done being created."""
        # Make sure _something_ is being shown
        if self.model.data:
            self._icon_data_update()
        elif self.model.image == '':
            self.model.image = DEFAULT_ICON_URL

    def _change_iconset_file(self):
        fn = getOpenFileName(parent=self.widget,
                             caption='Open Iconset',
                             filter='*.svg')
        if fn:
            self.model.image = 'file://' + urllib.request.pathname2url(fn)

    def _change_iconset_url(self):
        url = self.model.image
        url, ok = QInputDialog.getText(self.widget, 'Set URL',
                                       'New iconset URL:', text=url)
        if ok:
            self.model.image = url
