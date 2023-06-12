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
import pytest
from qtpy.QtCore import QEvent, QPoint, Qt
from qtpy.QtGui import QDropEvent
from qtpy.QtWidgets import QWidget
from traits.api import Dict, Instance

from karabo.common.scenemodel.api import (
    BaseWidgetObjectData, BoxLayoutModel, SceneModel)
from karabo.common.states import State
from karabo.native import AccessMode, Configurable, Double, String
from karabogui.binding.api import FloatBinding, StringBinding
from karabogui.configurator.utils import dragged_configurator_items
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.sceneview.tools.scenedndhandler import ConfigurationDropHandler
from karabogui.sceneview.view import SceneView
from karabogui.testing import flushed_registry, get_property_proxy

GET_PROXY_PATH = "karabogui.sceneview.tools.scenedndhandler.get_proxy"


class TypeObject(Configurable):
    state = String(
        defaultValue=State.ON,
        enum=State,
        displayType="State",
        accessMode=AccessMode.READONLY)
    prop = Double(
        minInc=-1000.0,
        maxInc=1000.0,
        allowedStates=[State.ON])


class UniqueModel(BaseWidgetObjectData):
    attributes = Dict


@pytest.fixture(scope="module")
def display_widget():
    with flushed_registry():
        @register_binding_controller(
            klassname="Editor",
            binding_type=(StringBinding, FloatBinding))
        class DisplayWidget(BaseBindingController):
            model = Instance(UniqueModel)

            @staticmethod
            def initialize_model(proxy, model):
                model.attributes = proxy.binding.attributes

            def create_widget(self, parent=None):
                return QWidget(parent)

        yield DisplayWidget


def test_scene_dnd_handler(gui_app, display_widget, mocker):
    """Test the scene drag and drop handler"""
    schema = TypeObject.getClassSchema()
    proxy = get_property_proxy(schema, "prop")
    state_proxy = get_property_proxy(schema, "state")
    assert proxy.binding is not None
    assert state_proxy.binding is not None
    scene_model = SceneModel(children=[])
    scene_view = SceneView(model=scene_model)

    # 1. Reconfigurable item
    items = dragged_configurator_items([proxy])
    assert items is not None

    event = QDropEvent(QPoint(0, 0), Qt.CopyAction, items,
                       Qt.LeftButton, Qt.NoModifier, QEvent.Drop)
    handler = ConfigurationDropHandler()
    assert handler.can_handle(event)
    assert len(scene_model.children) == 0

    def get_proxy(*args, **kwargs):
        return proxy

    mocker.patch(GET_PROXY_PATH, new=get_proxy)
    handler.handle(scene_view, event)

    assert len(scene_model.children) == 1
    assert isinstance(scene_model.children[0], BoxLayoutModel)
    model = scene_model.children[0].children[1]
    assert isinstance(model, UniqueModel)
    assert model.attributes["minInc"] == -1000
    assert model.attributes["maxInc"] == 1000

    # 2. Readonly param
    items = dragged_configurator_items([state_proxy])
    assert items is not None

    event = QDropEvent(QPoint(0, 0), Qt.CopyAction, items,
                       Qt.LeftButton, Qt.NoModifier, QEvent.Drop)
    assert handler.can_handle(event)
    assert len(scene_model.children) == 1

    def get_proxy(*args, **kwargs):
        return state_proxy

    mocker.patch(GET_PROXY_PATH, new=get_proxy)
    handler.handle(scene_view, event)

    assert len(scene_model.children) == 2
    assert isinstance(scene_model.children[1], BoxLayoutModel)
    model = scene_model.children[1].children[1]
    assert isinstance(model, UniqueModel)
    assert model.attributes["displayType"] == "State"

    scene_view.destroy()
