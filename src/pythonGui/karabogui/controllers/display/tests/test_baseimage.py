from unittest.mock import patch

from PyQt4.QtCore import QPoint, QRect
from PyQt4.QtGui import QWidget

from karabo.common.scenemodel.api import WebcamImageModel
from karabogui.binding.api import (
    apply_configuration, build_binding, DeviceProxy, PropertyProxy)
from karabogui.testing import GuiTestCase
from .image import PipelineData, get_image_hash
from ..baseimage import WebcamImageDisplay


# XXX: ScientificImageDisplay is not tested because it is identical to
# WebcamImageDisplay (from widget to model)

class MockQMenu(QWidget):
    actions = []

    def addAction(self, action):
        self.actions.append(action)

    def exec(self, pos):
        for ac in self.actions:
            ac.setChecked(True)


class TestWebcamImageDisplay(GuiTestCase):
    def setUp(self):
        super(TestWebcamImageDisplay, self).setUp()

        schema = PipelineData.getClassSchema()
        binding = build_binding(schema)
        root_proxy = DeviceProxy(binding=binding)
        self.model = WebcamImageModel()

        self.ouput_proxy = PropertyProxy(root_proxy=root_proxy,
                                         path='output.data')
        img_proxy = PropertyProxy(root_proxy=root_proxy,
                                  path='output.data.image')
        self.controller = WebcamImageDisplay(proxy=img_proxy,
                                             model=self.model)
        self.controller.create(None)
        # give qwt sometime to construct
        self.process_qt_events()

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_basics(self):
        assert self.controller.widget is not None

        self.model.show_tool_bar = False
        self.model.show_color_bar = False
        self.model.show_axes = False
        sym = "karabogui.controllers.display.baseimage.QMenu"
        with patch(sym, new=MockQMenu):
            self.controller.show_context_menu(QPoint(0, 0))
            assert self.model.show_tool_bar
            assert self.model.show_color_bar
            assert self.model.show_axes

        self.controller.zoom_reset()
        assert self.controller._zoom_rect == QRect()

        # walk code path
        for ac in self.controller._toolbar.actions():
            ac.toggled.emit(True)
        self.controller.axis_changed()

    def test_2dimage(self):
        # display image, nothing to check
        apply_configuration(get_image_hash(), self.ouput_proxy.binding)
        # display a new image
        apply_configuration(get_image_hash(val=1), self.ouput_proxy.binding)
