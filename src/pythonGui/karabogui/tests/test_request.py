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
import inspect
from unittest import main, mock

from karabo.common.project.api import MacroModel, ProjectModel, write_macro
from karabo.common.scenemodel.api import (
    SceneModel, SceneTargetWindow, write_scene)
from karabo.native import Hash
from karabogui.binding.api import (
    DeviceProxy, PropertyProxy, ProxyStatus, apply_configuration,
    apply_default_configuration, build_binding)
from karabogui.events import KaraboEvent
from karabogui.request import (
    get_macro_from_server, get_scene_from_server, onConfigurationUpdate,
    onSchemaUpdate)
from karabogui.singletons.manager import Manager
from karabogui.testing import GuiTestCase, get_simple_schema, singletons


class TestRequestModule(GuiTestCase):

    def setUp(self):
        schema = get_simple_schema()
        root_binding = build_binding(schema)
        apply_default_configuration(root_binding)
        self.root_proxy = DeviceProxy(binding=root_binding, device_id="marty")
        self.root_proxy.status = ProxyStatus.ONLINE
        self.property_proxy = PropertyProxy(
            root_proxy=self.root_proxy, path="foo")

    def test_config_update(self):
        received = 0

        def handler():
            nonlocal received
            received += 1

        for proxy in (self.root_proxy, self.property_proxy):
            received = 0

            onConfigurationUpdate(proxy, handler)

            config = Hash("foo", True, "bar", "a", "charlie", 0)
            apply_configuration(config, self.root_proxy.binding)
            assert received == 1
            apply_configuration(config, self.root_proxy.binding)
            assert received == 1

            handler = onConfigurationUpdate(proxy, handler, remove=False)
            apply_configuration(config, self.root_proxy.binding)
            assert received == 2
            apply_configuration(config, self.root_proxy.binding)
            assert received == 3

            # Remove manually
            proxy.binding.on_trait_change(handler, "config_update",
                                          remove=True)
            apply_configuration(config, self.root_proxy.binding)
            assert received == 3

            network = mock.Mock()
            with singletons(network=network):
                onConfigurationUpdate(proxy, handler, request=True)
                apply_configuration(config, self.root_proxy.binding)
                assert received == 4
                network.onGetDeviceConfiguration.assert_called_with("marty")

    def test_schema_update(self):
        received = 0

        def handler():
            nonlocal received
            received += 1

        for proxy in (self.root_proxy, self.property_proxy):
            received = 0

            onSchemaUpdate(proxy, handler)

            build_binding(get_simple_schema(),
                          existing=self.root_proxy.binding)
            assert received == 1
            build_binding(get_simple_schema(),
                          existing=self.root_proxy.binding)
            assert received == 1

            handler = onSchemaUpdate(proxy, handler, remove=False)
            build_binding(get_simple_schema(),
                          existing=self.root_proxy.binding)
            assert received == 2

            build_binding(get_simple_schema(),
                          existing=self.root_proxy.binding)
            assert received == 3

            # Remove manually
            self.root_proxy.on_trait_change(handler, "schema_update",
                                            remove=True)
            build_binding(get_simple_schema(),
                          existing=self.root_proxy.binding)
            assert received == 3

            network = mock.Mock()
            with singletons(network=network):
                onSchemaUpdate(proxy, handler, request=True, remove=True)
                build_binding(get_simple_schema(),
                              existing=self.root_proxy.binding)
                network.onGetDeviceSchema.assert_called_with("marty")

    def test_get_macro_from_server(self):
        network = mock.Mock()
        with singletons(network=network):
            manager = Manager()
            with singletons(manager=manager):
                # 1. Success case
                project = ProjectModel(simple_name="MyProject")
                assert len(project.macros) == 0

                token = get_macro_from_server("macro_device",
                                              "new_macro", project)
                network.onExecuteGeneric.assert_called_with(
                    "macro_device", "requestMacro", Hash("name", "new_macro"),
                    token=token)

                macro = MacroModel(simple_name="NewMacro", code="import time")
                payload = Hash("success", True, "data", write_macro(macro))
                reply = Hash("payload", payload)
                request = Hash("token", token)

                path = "karabogui.request.broadcast_event"
                with mock.patch(path) as broadcast:
                    manager.handle_requestGeneric(True, request, reply=reply)
                    broadcast.assert_called_with(KaraboEvent.ShowMacroView,
                                                 {"model": mock.ANY})
                # A new macro has been added
                assert len(project.macros) == 1

                # 2. False case
                token = get_macro_from_server("macro_device",
                                              "new_macro", project)
                network.onExecuteGeneric.assert_called_with(
                    "macro_device", "requestMacro", Hash("name", "new_macro"),
                    token=token)
                payload = Hash("success", False, "data", "")
                reply = Hash("payload", payload)
                request = Hash("token", token)
                path = "karabogui.request.messagebox"
                with mock.patch(path) as mbox:
                    manager.handle_requestGeneric(True, request, reply=reply)
                    mbox.show_warning.assert_called_once()

                # No further macro added
                assert len(project.macros) == 1

    def test_get_scene_from_server(self):
        network = mock.Mock()
        with singletons(network=network):
            manager = Manager()
            with singletons(manager=manager):
                # 1. Success case
                project = ProjectModel(simple_name="MyProject")
                assert len(project.scenes) == 0

                token = get_scene_from_server("scene_device",
                                              "new_scene", project)
                network.onExecuteGeneric.assert_called_with(
                    "scene_device", "requestScene", Hash("name", "new_scene"),
                    token=token)

                scene = SceneModel(simple_name="NewScene")
                payload = Hash("success", True, "data", write_scene(scene))
                reply = Hash("payload", payload)
                request = Hash("token", token, "args", Hash())

                path = "karabogui.request.broadcast_event"
                with mock.patch(path) as broadcast:
                    manager.handle_requestGeneric(True, request, reply=reply)
                    broadcast.assert_called_with(
                        KaraboEvent.ShowSceneView,
                        {"model": mock.ANY,
                         'target_window': SceneTargetWindow.Dialog})
                # A new scene has been added
                assert len(project.scenes) == 1

                # 2. False case - from scene protocol
                token = get_scene_from_server("scene_device",
                                              "new_scene", project)
                network.onExecuteGeneric.assert_called_with(
                    "scene_device", "requestScene", Hash("name", "new_scene"),
                    token=token)
                payload = Hash("success", False, "data", "")
                reply = Hash("payload", payload)
                request = Hash("token", token)
                path = "karabogui.request.messagebox"
                with mock.patch(path) as mbox:
                    manager.handle_requestGeneric(True, request, reply=reply)
                    mbox.show_warning.assert_called_once()

                # No further scene added
                assert len(project.scenes) == 1

                # 3. False case - from failing slot call
                token = get_scene_from_server("scene_device",
                                              "new_scene", project)
                network.onExecuteGeneric.assert_called_with(
                    "scene_device", "requestScene", Hash("name", "new_scene"),
                    token=token)
                payload = Hash("success", False, "data", "")
                reply = "failure reason"
                request = Hash("token", token)
                path = "karabogui.request.messagebox"
                with mock.patch(path) as mbox:
                    manager.handle_requestGeneric(False, request, reply=reply)
                    mbox.show_warning.assert_called_once()

                # No further scene added
                assert len(project.scenes) == 1

                # 4. success case, with position
                assert len(project.scenes) == 1
                token = get_scene_from_server("scene_device",
                                              "extra_scene", project=None,
                                              position=[0, 120])

                network.onExecuteGeneric.assert_called_with(
                    "scene_device", "requestScene", Hash("name", "extra_scene",
                                                         "position", [0, 120]),
                    token=token)

                scene = SceneModel(simple_name="NewScene")
                payload = Hash("success", True, "data", write_scene(scene))
                reply = Hash("payload", payload)
                request = Hash("token", token,
                               "args", Hash("position", [0, 120]))

                path = "karabogui.request.broadcast_event"
                with mock.patch(path) as broadcast:
                    manager.handle_requestGeneric(True, request, reply=reply)
                    broadcast.assert_called_with(
                        KaraboEvent.ShowUnattachedSceneView,
                        {"model": mock.ANY,
                         "target_window": SceneTargetWindow.Dialog,
                         "position": [0, 120]})
                # A new scene has not been added
                assert len(project.scenes) == 1

    def test_signature_scene_from_server(self):
        """If this tests fails, we need to check the db connection db scene"""
        sig = inspect.signature(get_scene_from_server)
        params = sig.parameters
        assert "slot_name" in params
        assert "project" in params
        assert "slot_name" in params
        assert "target_window" in params
        assert "scene_name" in params
        assert len(params) == 6


if __name__ == "__main__":
    main()
