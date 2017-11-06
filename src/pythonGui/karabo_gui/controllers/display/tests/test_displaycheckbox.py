from xml.etree.ElementTree import parse

from karabo.common.api import DeviceStatus
from karabo.middlelayer import Configurable, Bool
from karabo_gui.binding.api import (
    DeviceClassProxy, PropertyProxy, build_binding
)
from karabo_gui.testing import GuiTestCase
from ..displaycheckbox import DisplayCheckBox, CHECKED, UNCHECKED


class Object(Configurable):
    prop = Bool(defaultValue=True)


def _check_renderer_against_svg(renderer, svgfile):
    """Yeah... Check an SVG for elements which we suspect it should have.
    """
    def _get_path_ids(filename):
        tree = parse(filename)
        ids = []
        for elem in tree.iter('{http://www.w3.org/2000/svg}path'):
            ids.append(elem.get('id'))
        return ids

    for name in _get_path_ids(svgfile):
        assert renderer.elementExists(name)


class TestDisplayCheckBox(GuiTestCase):
    def setUp(self):
        super(TestDisplayCheckBox, self).setUp()

        schema = Object.getClassSchema()
        binding = build_binding(schema)
        device = DeviceClassProxy(binding=binding, server_id='Fake',
                                  status=DeviceStatus.OFFLINE)
        self.proxy = PropertyProxy(root_proxy=device, path='prop')
        self.controller = DisplayCheckBox(proxy=self.proxy)
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        self.proxy.value = True
        _check_renderer_against_svg(self.controller.widget.renderer(),
                                    CHECKED)

        self.proxy.value = False
        _check_renderer_against_svg(self.controller.widget.renderer(),
                                    UNCHECKED)
