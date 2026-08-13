# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the GNU General Public License
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import pytest
from qtpy.QtCore import QEvent, QPointF, Qt
from qtpy.QtGui import QClipboard, QKeyEvent, QMouseEvent
from qtpy.QtWidgets import QWidget
from traits.api import Instance

from karabo.common.scenemodel.api import BaseWidgetObjectData, SceneModel
from karabogui.binding.api import FloatBinding, ProxyStatus
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.events import (
    KaraboEvent, register_for_broadcasts, unregister_from_broadcasts)
from karabogui.sceneview.tools.selection import ProxySelectionTool
from karabogui.sceneview.view import SceneView
from karabogui.singletons.mediator import Mediator
from karabogui.testing import flushed_registry, singletons, system_hash
from karabogui.topology.api import SystemTopology


class UniqueModel(BaseWidgetObjectData):
    """A simple unique model"""


@pytest.fixture(scope="module")
def display_widget():
    with flushed_registry():
        @register_binding_controller(
            klassname="Editor", binding_type=FloatBinding)
        class DisplayWidget(BaseBindingController):
            model = Instance(UniqueModel)

            def create_widget(self, parent=None):
                return QWidget(parent)

        yield DisplayWidget


def test_proxy_selection_tool(gui_app, display_widget):
    topology = SystemTopology()
    topology.initialize(system_hash())
    mediator = Mediator()
    events = []
    event_map = {KaraboEvent.ShowConfiguration: events.append}
    with singletons(topology=topology, mediator=mediator):
        register_for_broadcasts(event_map)
        scene_view = SceneView(model=SceneModel(children=[]))
        model = UniqueModel(keys=["divyy.prop"])
        scene_view.add_models(model)
        controller = scene_view.controller_at_position(
            QPointF(0, 0).toPoint())
        device = controller.widget_controller.proxy.root_proxy
        key_event = QKeyEvent(QEvent.KeyPress, Qt.Key_Alt, Qt.AltModifier)
        scene_view.keyPressEvent(key_event)
        assert isinstance(scene_view.current_tool, ProxySelectionTool)
        event = QMouseEvent(
            QEvent.MouseButtonPress, QPointF(controller.geometry().center()),
            Qt.LeftButton, Qt.LeftButton, Qt.AltModifier)
        assert event.modifiers() & Qt.AltModifier

        device.status = ProxyStatus.ONLINE
        scene_view.mousePressEvent(event)
        gui_app.processEvents()
        assert events == [{"proxy": device}]

        device.status = ProxyStatus.OFFLINE
        clipboard = gui_app.clipboard()
        clipboard.clear(mode=QClipboard.Clipboard)
        scene_view.mousePressEvent(event)
        assert clipboard.text(mode=QClipboard.Clipboard) == "divyy"
        # Offline, nothing added
        assert events == [{"proxy": device}]

        scene_view.destroy()
        unregister_from_broadcasts(event_map)
