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

import pytest
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


# fixtures
@pytest.fixture
def setup_widget_fonts(gui_app):
    view = SceneView()

    # Prepare a property proxy to create the controller widget
    # successfully
    schema = Object.getClassSchema()
    binding = build_binding(schema)
    device = DeviceProxy(binding=binding,
                         server_id='Fake',
                         device_id=DEVICE_NAME,
                         status=ProxyStatus.OFFLINE)
    property_proxy = PropertyProxy(root_proxy=device,
                                   path=PROPERTY_NAME)
    property_proxy.value = ['bob', 'frank']
    yield view, property_proxy
    # The destroy will clear the cache
    view.destroy()


@pytest.fixture
def setup_scene_fonts(setup_widget_fonts):
    view, property_proxy = setup_widget_fonts
    # Prepare the fonts
    font_families = QFontDatabase().families()
    if len(font_families) > NUM_TESTED_FONTS:
        font_families = random.sample(font_families, NUM_TESTED_FONTS)
    yield view, property_proxy, font_families


@pytest.fixture
def setup_widget_fonts_macos(gui_app, mocker):
    # Create a QApplication with a mocked Mac OSX font size
    mocker.patch("karabogui.fonts.GUI_DPI_FACTOR", new=MAC_DPI_FACTOR)
    font_size = get_font_size_from_dpi(SCENE_FONT_SIZE)

    mocker.patch("karabogui.programs.base.SCENE_FONT_SIZE", new=font_size)
    # same setup as for normal test
    view = SceneView()
    schema = Object.getClassSchema()
    binding = build_binding(schema)
    device = DeviceProxy(binding=binding,
                         server_id='Fake',
                         device_id=DEVICE_NAME,
                         status=ProxyStatus.OFFLINE)
    property_proxy = PropertyProxy(root_proxy=device,
                                   path=PROPERTY_NAME)
    property_proxy.value = ['bob', 'frank']
    yield view, property_proxy
    # The destroy will clear the cache
    view.destroy()


# helper functions
def set_models_to_scene(view, property_proxy, models, mocker):
    mocker.patch(GET_PROXY_PATH, return_value=property_proxy)
    view.update_model(SceneModel(children=models))


def get_widget(view, model):
    return view._scene_obj_cache.get(model)


def assert_geometry(model, pos=(0, 0), size=(0, 0)):
    if isinstance(pos, QPoint):
        pos = (pos.x(), pos.y())
    if isinstance(size, QSize):
        size = (size.width(), size.height())

    assert model.x == pos[0]
    assert model.y == pos[1]
    assert model.width == size[0]
    assert model.height == size[1]


def assert_font_size(obj, expected=SCENE_FONT_SIZE):
    if isinstance(obj, ControllerContainer):
        qfont = obj.widget_controller.widget.font()
    elif isinstance(obj, QWidget):
        qfont = obj.font()
    else:
        # We want to compare the absolve value of the font size
        qfont = get_qfont(obj.font, adjust_size=False)
    assert qfont.pointSize() == expected


def assert_model_text_widgets(klass, view, property_proxy, mocker):
    # Trigger size hint calculation by not specifying model geometry
    model = klass(text="I am a long text qweqweqwe",
                  keys=[PROPERTY_PATH])
    font_size = get_font_size_from_dpi(SCENE_FONT_SIZE)
    # Add to scene
    set_models_to_scene(view, property_proxy, [model], mocker)
    widget = get_widget(view, model)
    assert_geometry(model, size=widget.size())
    assert_font_size(widget, expected=font_size)
    assert_font_size(model, expected=SCENE_FONT_SIZE)

    # Reload the model
    read_model = single_model_round_trip(model)
    assert_font_size(read_model, SCENE_FONT_SIZE)

    # Add the reloaded model to scene
    set_models_to_scene(view, property_proxy, [model], mocker)
    widget = get_widget(view, model)
    assert_geometry(read_model, pos=(model.x, model.y),
                    size=(model.width, model.height))
    assert_font_size(widget, expected=font_size)
    assert_font_size(read_model, expected=SCENE_FONT_SIZE)


def assert_model_scene_widgets(klass, view, property_proxy,
                               font_families, mocker):
    models = [
        klass(text=font, font=QFont(font).toString(), keys=[PROPERTY_PATH])
        for font in font_families]
    set_models_to_scene(view, property_proxy, models, mocker)

    for model in models:
        widget = view._scene_obj_cache[model]
        if isinstance(widget, ControllerContainer):
            # Get the internal widget from the controller container
            widget = widget.widget_controller.widget
        assert widget.font().family() in FONT_FAMILIES


# actual tests
def test_text_widgets(setup_widget_fonts, mocker):
    """Test the basic widgets fonts sizes"""
    view, property_proxy = setup_widget_fonts
    assert_model_text_widgets(LabelModel, view, property_proxy, mocker)
    assert_model_text_widgets(SceneLinkModel, view, property_proxy, mocker)
    assert_model_text_widgets(DeviceSceneLinkModel, view, property_proxy,
                              mocker)
    assert_model_text_widgets(StickerModel, view, property_proxy, mocker)
    assert_model_text_widgets(WebLinkModel, view, property_proxy, mocker)


def test_scene_text_widgets(setup_scene_fonts, mocker):
    """Test the scene font families replacement"""
    view, property_proxy, font_families = setup_scene_fonts
    assert_model_scene_widgets(LabelModel, view, property_proxy,
                               font_families, mocker)
    assert_model_scene_widgets(SceneLinkModel, view, property_proxy,
                               font_families, mocker)
    assert_model_scene_widgets(DeviceSceneLinkModel, view, property_proxy,
                               font_families, mocker)
    assert_model_scene_widgets(StickerModel, view, property_proxy,
                               font_families, mocker)
    assert_model_scene_widgets(WebLinkModel, view, property_proxy,
                               font_families, mocker)


@pytest.mark.skipif(system() == "Darwin",
                    reason="This test is mocking OSX from another OS.")
def test_text_widgets_on_macos(setup_widget_fonts_macos, mocker):
    """Test the macOS mock of the fonts"""
    view, property_proxy = setup_widget_fonts_macos
    mocker.patch("karabogui.fonts.GUI_DPI_FACTOR", new=MAC_DPI_FACTOR)
    assert_model_text_widgets(LabelModel, view, property_proxy, mocker)
    assert_model_text_widgets(SceneLinkModel, view, property_proxy, mocker)
    assert_model_text_widgets(DeviceSceneLinkModel, view, property_proxy,
                              mocker)
    assert_model_text_widgets(StickerModel, view, property_proxy, mocker)
    assert_model_text_widgets(WebLinkModel, view, property_proxy, mocker)
