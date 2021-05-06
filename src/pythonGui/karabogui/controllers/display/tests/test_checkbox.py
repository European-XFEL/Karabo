from xml.etree.ElementTree import parse

from karabo.native import Bool, Configurable
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from ..checkbox import CHECKED, UNCHECKED, DisplayCheckBox


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
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = DisplayCheckBox(proxy=self.proxy)
        self.controller.create(None)

    def tearDown(self):
        super(TestDisplayCheckBox, self).tearDown()
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        set_proxy_value(self.proxy, 'prop', True)
        _check_renderer_against_svg(self.controller.widget.renderer(),
                                    CHECKED)

        set_proxy_value(self.proxy, 'prop', False)
        _check_renderer_against_svg(self.controller.widget.renderer(),
                                    UNCHECKED)
