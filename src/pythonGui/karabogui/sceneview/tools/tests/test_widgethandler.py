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
from qtpy.QtGui import QMouseEvent
from qtpy.QtWidgets import QWidget
from traits.api import Dict, Instance

from karabo.common.scenemodel.api import BaseWidgetObjectData, SceneModel
from karabo.native import Configurable, Double
from karabogui import icons
from karabogui.binding.api import FloatBinding, ProxyStatus, StringBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.sceneview.api import ControllerContainer, SceneView
from karabogui.sceneview.tools.widgethandler import SceneControllerHandler
from karabogui.testing import flushed_registry, singletons, system_hash
from karabogui.topology.api import SystemTopology


class TypeObject(Configurable):
    prop = Double(
        minInc=-1000.0,
        maxInc=1000.0)


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


def test_scene_widget_handler(gui_app, display_widget, mocker):
    """Test the scene widget handler"""
    topology = SystemTopology()
    topology.initialize(system_hash())
    with singletons(topology=topology):
        scene_model = SceneModel(children=[])
        scene_view = SceneView(model=scene_model)
        models = UniqueModel(
            keys=["divyy.prop"],
            attributes={"One": "Two"})
        scene_view.add_models(models)

        assert len(scene_model.children) == 1

        item = scene_view.controller_at_position(QPoint(0, 0))
        assert item is not None
        assert isinstance(item, ControllerContainer)
        assert isinstance(item.widget_controller, display_widget)
        assert "minInc" not in item.model.attributes
        assert "maxInc" not in item.model.attributes

        handler = SceneControllerHandler(widget=item)
        assert handler.can_handle()

        event = QMouseEvent(
            QEvent.MouseButtonPress,
            QPoint(80, 0),
            Qt.RightButton,
            Qt.RightButton,
            Qt.NoModifier)

        menu = mocker.MagicMock()
        handler.handle_widget(scene_view, event, menu)
        menu.addAction.assert_called_with("No mutation for offline properties")

        item.widget_controller.proxy.root_proxy.status = ProxyStatus.ONLINE
        topology.device_schema_updated("divyy", TypeObject.getClassSchema())
        assert item.widget_controller.proxy.binding is not None

        menu.reset_mock()
        handler.handle_widget(scene_view, event, menu)
        menu.addMenu.assert_called_with(icons.change, "Change Widget")

        # Change the widget
        handler._change_widget(scene_view, display_widget)
        item = scene_view.controller_at_position(QPoint(0, 0))
        assert item is not None
        assert isinstance(item, ControllerContainer)
        assert isinstance(item.widget_controller, display_widget)
        assert item.model.attributes["minInc"] == -1000.0
        assert item.model.attributes["maxInc"] == 1000.0

        scene_view.destroy()
