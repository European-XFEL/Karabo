# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import random
from platform import system
from unittest import mock, skipIf

from qtpy.QtCore import QPoint, QSize
from qtpy.QtGui import QFont, QFontDatabase
from qtpy.QtWidgets import QWidget

from karabo.common.scenemodel.api import (
    SCENE_DEFAULT_DPI, SCENE_FONT_SIZE, SCENE_MAC_DPI, DeviceSceneLinkModel,
    LabelModel, SceneLinkModel, SceneModel, StickerModel, WebLinkModel)
from karabo.common.scenemodel.tests.utils import single_model_round_trip
from karabo.native import Configurable, VectorString
from karabogui.binding.api import (
    DeviceProxy, PropertyProxy, ProxyStatus, build_binding)
from karabogui.fonts import FONT_FAMILIES, get_font_size_from_dpi, get_qfont
from karabogui.testing import GuiTestCase

from ..view import SceneView
from ..widget.container import ControllerContainer

NUM_TESTED_FONTS = 20
DEVICE_NAME = "Device"
PROPERTY_NAME = "availableScenes"
PROPERTY_PATH = f"{DEVICE_NAME}.{PROPERTY_NAME}"

GET_PROXY_PATH = "karabogui.sceneview.widget.container.get_proxy"

MAC_DPI_FACTOR = SCENE_DEFAULT_DPI / SCENE_MAC_DPI


class Object(Configurable):
    availableScenes = VectorString()


class BaseSceneFontTest(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.view = SceneView()

        # Prepare a property proxy to create the controller widget
        # successfully
        schema = Object.getClassSchema()
        binding = build_binding(schema)
        device = DeviceProxy(binding=binding,
                             server_id='Fake',
                             device_id=DEVICE_NAME,
                             status=ProxyStatus.OFFLINE)
        self.property_proxy = PropertyProxy(root_proxy=device,
                                            path=PROPERTY_NAME)
        self.property_proxy.value = ['bob', 'frank']

    def tearDown(self):
        # The destroy will clear the cache
        self.view.destroy()
        self.view = None
        super().tearDown()

    def set_models_to_scene(self, models):
        with mock.patch(GET_PROXY_PATH, return_value=self.property_proxy):
            self.view.update_model(SceneModel(children=models))

    def get_widget(self, model):
        return self.view._scene_obj_cache.get(model)


class BaseWidgetFontTest(BaseSceneFontTest):

    def test_text_widgets(self):
        """Test the basic widgets fonts sizes"""
        self.assert_model(LabelModel)
        self.assert_model(SceneLinkModel)
        self.assert_model(DeviceSceneLinkModel)
        self.assert_model(StickerModel)
        self.assert_model(WebLinkModel)

    def assert_model(self, klass):
        # Trigger size hint calculation by not specifying model geometry
        model = klass(text="I am a long text qweqweqwe",
                      keys=[PROPERTY_PATH])
        font_size = get_font_size_from_dpi(SCENE_FONT_SIZE)
        # Add to scene
        self.set_models_to_scene([model])
        widget = self.get_widget(model)
        self._assert_geometry(model, size=widget.size())
        self._assert_font_size(widget, expected=font_size)
        self._assert_font_size(model, expected=SCENE_FONT_SIZE)

        # Reload the model
        read_model = single_model_round_trip(model)
        self._assert_font_size(read_model, SCENE_FONT_SIZE)

        # Add the reloaded model to scene
        self.set_models_to_scene([model])
        widget = self.get_widget(model)
        self._assert_geometry(read_model, pos=(model.x, model.y),
                              size=(model.width, model.height))
        self._assert_font_size(widget, expected=font_size)
        self._assert_font_size(read_model, expected=SCENE_FONT_SIZE)

    def _assert_geometry(self, model, pos=(0, 0), size=(0, 0)):
        if isinstance(pos, QPoint):
            pos = (pos.x(), pos.y())
        if isinstance(size, QSize):
            size = (size.width(), size.height())

        self.assertEqual(model.x, pos[0])
        self.assertEqual(model.y, pos[1])
        self.assertEqual(model.width, size[0])
        self.assertEqual(model.height, size[1])

    def _assert_font_size(self, obj, expected=SCENE_FONT_SIZE):
        if isinstance(obj, ControllerContainer):
            qfont = obj.widget_controller.widget.font()
        elif isinstance(obj, QWidget):
            qfont = obj.font()
        else:
            # We want to compare the absolve value of the font size
            qfont = get_qfont(obj.font, adjust_size=False)
        self.assertEqual(qfont.pointSize(), expected)


class TestSceneFonts(BaseSceneFontTest):

    def setUp(self):
        super().setUp()
        # Prepare the fonts
        font_families = QFontDatabase().families()
        if len(font_families) > NUM_TESTED_FONTS:
            font_families = random.sample(font_families, NUM_TESTED_FONTS)
        self._font_families = font_families

    def test_text_widgets(self):
        """Test the scene font families replacement"""
        self.assert_model(LabelModel)
        self.assert_model(SceneLinkModel)
        self.assert_model(DeviceSceneLinkModel)
        self.assert_model(StickerModel)
        self.assert_model(WebLinkModel)

    def assert_model(self, klass):
        models = [
            klass(text=font, font=QFont(font).toString(), keys=[PROPERTY_PATH])
            for font in self._font_families]
        self.set_models_to_scene(models)

        for model in models:
            widget = self.view._scene_obj_cache[model]
            if isinstance(widget, ControllerContainer):
                # Get the internal widget from the controller container
                widget = widget.widget_controller.widget
            self.assertTrue(widget.font().family() in FONT_FAMILIES)


@skipIf(system() == "Darwin",
        reason="This test is mocking OSX from another OS.")
class TestWidgetFontsOnMacOSX(BaseWidgetFontTest):

    def setUp(self):
        # Create a QApplication with a mocked Mac OSX font size
        font_size = self._get_font_size(SCENE_FONT_SIZE)
        with mock.patch("karabogui.programs.base.SCENE_FONT_SIZE",
                        new=font_size):
            super().setUp()

    @mock.patch("karabogui.fonts.GUI_DPI_FACTOR", new=MAC_DPI_FACTOR)
    def test_text_widgets(self):
        """Test the macOS mock of the fonts"""
        self.assert_model(LabelModel)
        self.assert_model(SceneLinkModel)
        self.assert_model(DeviceSceneLinkModel)
        self.assert_model(StickerModel)
        self.assert_model(WebLinkModel)

    def set_models_to_scene(self, models):
        with mock.patch("karabogui.fonts.GUI_DPI_FACTOR", new=MAC_DPI_FACTOR):
            super().set_models_to_scene(models)

    def _get_font_size(self, size):
        with mock.patch("karabogui.fonts.GUI_DPI_FACTOR", new=MAC_DPI_FACTOR):
            return get_font_size_from_dpi(size)
