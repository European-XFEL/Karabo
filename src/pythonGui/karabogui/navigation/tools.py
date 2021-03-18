#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on January 7, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from abc import abstractmethod

from traits.api import ABCHasStrictTraits, Undefined

from karabo.common.api import Capabilities
from karabogui import messagebox
from karabogui.enums import NavigationItemTypes
from karabogui.request import get_scene_from_server
from karabogui.singletons.api import get_topology


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
        device_id = info.get('deviceId')
        capabilities = info.get('capabilities')

        def _test_mask(mask, bit):
            return (mask & bit) == bit

        has_scene = _test_mask(capabilities, Capabilities.PROVIDES_SCENES)
        if not has_scene:
            messagebox.show_warning("The device <b>{}</b> does not provide a "
                                    "scene!".format(device_id))
            return

        def _config_handler():
            """Act on the arrival of the configuration
            """
            proxy.on_trait_change(_config_handler, 'config_update',
                                  remove=True)
            scenes = proxy.binding.value.availableScenes.value
            if scenes is Undefined or not len(scenes):
                messagebox.show_warning(
                    "The device <b>{}</b> does not specify a scene "
                    "name!".format(device_id))
            else:
                scene_name = scenes[0]
                get_scene_from_server(device_id, scene_name)

        def _schema_handler():
            """Act on the arrival of the schema
            """
            proxy.on_trait_change(_schema_handler, 'schema_update',
                                  remove=True)
            scenes = proxy.binding.value.availableScenes.value
            if scenes is Undefined:
                proxy.on_trait_change(_config_handler, 'config_update')
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
            proxy.on_trait_change(_schema_handler, 'schema_update')
        elif proxy.binding.value.availableScenes.value is Undefined:
            # The configuration did not yet arrive and we cannot get
            # a scene name from the availableScenes. We wait for the
            # configuration to arrive and install a handler.
            proxy.on_trait_change(_config_handler, 'config_update')
        else:
            scenes = proxy.binding.value.availableScenes.value
            if not len(scenes):
                # The device might not have a scene name in property
                messagebox.show_warning(
                    "The device <b>{}</b> does not specify a scene "
                    "name!".format(device_id))
            else:
                scene_name = scenes[0]
                get_scene_from_server(device_id, scene_name)
