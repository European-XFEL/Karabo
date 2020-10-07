import random
from unittest import mock

from PyQt5.QtGui import QFont, QFontDatabase

from karabo.common.api import ProxyStatus
from karabo.common.scenemodel.api import (
    DeviceSceneLinkModel, LabelModel, SceneLinkModel, SceneModel,
    StickerModel, WebLinkModel)
from karabo.native import Configurable, VectorString

from karabogui.binding.api import build_binding, DeviceProxy, PropertyProxy
from karabogui.fonts import FONT_FAMILIES
from karabogui.testing import GuiTestCase
from ..widget.container import ControllerContainer
from ..view import SceneView

NUM_TESTED_FONTS = 20
DEVICE_NAME = "Device"
PROPERTY_NAME = "availableScenes"
PROPERTY_PATH = f"{DEVICE_NAME}.{PROPERTY_NAME}"

GET_PROXY_PATH = "karabogui.sceneview.widget.container.get_proxy"


class Object(Configurable):
    availableScenes = VectorString()


class TestSceneFonts(GuiTestCase):
    def setUp(self):
        super(TestSceneFonts, self).setUp()
        self.view = SceneView()

        # Prepare a property proxy to create the controller widget successfully
        schema = Object.getClassSchema()
        binding = build_binding(schema)
        device = DeviceProxy(binding=binding,
                             server_id='Fake',
                             device_id=DEVICE_NAME,
                             status=ProxyStatus.OFFLINE)
        self.property_proxy = PropertyProxy(root_proxy=device,
                                            path=PROPERTY_NAME)
        self.property_proxy.value = ['bob', 'frank']

        # Prepare the fonts
        font_families = QFontDatabase().families()
        if len(font_families) > NUM_TESTED_FONTS:
            font_families = random.sample(font_families, NUM_TESTED_FONTS)
        self._font_families = font_families

    def tearDown(self):
        self.view.destroy()
        self.view = None

    def test_text_widgets(self):
        self._assert_model(LabelModel)
        self._assert_model(SceneLinkModel)
        self._assert_model(DeviceSceneLinkModel)
        self._assert_model(StickerModel)
        self._assert_model(WebLinkModel)

    def _assert_model(self, klass):
        models = [
            klass(text=font, font=QFont(font).toString(), keys=[PROPERTY_PATH])
            for font in self._font_families]
        self._set_models_to_scene(models)

        for model in models:
            widget = self.view._scene_obj_cache[model]
            if isinstance(widget, ControllerContainer):
                # Get the internal widget from the controller container
                widget = widget.widget_controller.widget
            self.assertTrue(widget.font().family() in FONT_FAMILIES)

    def _set_models_to_scene(self, models):
        with mock.patch(GET_PROXY_PATH, return_value=self.property_proxy):
            self.view.update_model(SceneModel(children=models))
