#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on January 7, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from abc import abstractmethod

from traits.api import ABCHasStrictTraits, Undefined

from karabo.common.api import Capabilities
from karabogui import messagebox
from karabogui.dialogs.api import LogDialog
from karabogui.itemtypes import NavigationItemTypes
from karabogui.request import (
    get_scene_from_server, onConfigurationUpdate, onSchemaUpdate)
from karabogui.singletons.api import get_topology
from karabogui.util import version_compatible


class NavigationHandler(ABCHasStrictTraits):
    @abstractmethod
    def can_handle(self, info):
        """Check whether the click event can be handled."""

    @abstractmethod
    def handle(self, info):
        """Handle the click event."""


class DeviceSceneHandler(NavigationHandler):
    """Device DoubleClick for the navigation view
    """

    def can_handle(self, info):
        navigation_type = info.get('type')
        if navigation_type is NavigationItemTypes.DEVICE:
            return True
        return False

    def handle(self, info):
        device_id = info.get("deviceId")
        capabilities = info.get("capabilities")

        def _test_mask(mask, bit):
            return (mask & bit) == bit

        has_scene = _test_mask(capabilities, Capabilities.PROVIDES_SCENES)
        if not has_scene:
            messagebox.show_warning("The device <b>{}</b> does not provide a "
                                    "scene!".format(device_id))
            return

        def _config_handler():
            """Act on the arrival of the configuration"""
            scenes = proxy["availableScenes"].value
            if scenes is Undefined or not len(scenes):
                messagebox.show_warning(
                    "The device <b>{}</b> does not specify a scene "
                    "name!".format(device_id))
            else:
                scene_name = scenes[0]
                get_scene_from_server(device_id, scene_name)

        def _schema_handler():
            """Act on the arrival of the schema"""
            scenes = proxy["availableScenes"].value
            if scenes is Undefined:
                onConfigurationUpdate(proxy, _config_handler)
            elif not len(scenes):
                messagebox.show_warning(
                    "The device <b>{}</b> does not specify a scene "
                    "name!".format(device_id))
            else:
                scene_name = scenes[0]
                get_scene_from_server(device_id, scene_name)

        proxy = get_topology().get_device(device_id)
        if not len(proxy.binding.value):
            # We completely miss our schema and wait for it.
            onSchemaUpdate(proxy, _schema_handler)
        elif proxy["availableScenes"].value is Undefined:
            onConfigurationUpdate(proxy, _config_handler)
        else:
            scenes = proxy["availableScenes"].value
            if not len(scenes):
                # The device might not have a scene name in property
                messagebox.show_warning(
                    "The device <b>{}</b> does not specify a scene "
                    "name!".format(device_id))
            else:
                scene_name = scenes[0]
                get_scene_from_server(device_id, scene_name)


class ServerLogHandler(NavigationHandler):
    """Device DoubleClick for the navigation view
    """

    def can_handle(self, info):
        navigation_type = info.get("type")
        if (navigation_type is NavigationItemTypes.SERVER and
                version_compatible(info.get("karaboVersion"), 2, 14)):
            return True
        return False

    def handle(self, info):
        server_id = info.get("serverId")
        dialog = LogDialog(server_id)
        dialog.exec()
