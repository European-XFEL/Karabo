#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on May 9, 2017
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
#############################################################################
from functools import partial
from inspect import signature
from io import StringIO

from qtpy.QtCore import QTimer, Slot
from traits.api import Undefined

from karabo.common.api import Capabilities, walk_traits_object
from karabo.common.project.api import read_macro
from karabo.common.scenemodel.api import SceneTargetWindow, read_scene
from karabo.native import Hash
from karabogui import messagebox
from karabogui.binding.api import DeviceProxy, extract_sparse_configurations
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.fonts import substitute_font
from karabogui.logger import get_logger
from karabogui.singletons.api import get_manager, get_network, get_topology
from karabogui.util import get_reason_parts

_WAIT_SECONDS = 5
_waiting_devices = {}


def call_device_slot(handler, instance_id, slot_name, **kwargs):
    """Call a device slot via the GUI server. This works with slots which
    take a single `Hash` as an argument and reply with a `Hash`.

    :param handler: Callable with the signature

                    - handler(success, reply)
                    - handler(success, reply, request)

                    where:
                        success - a bool indicating whether the call succeeded
                        reply - a Hash containing the slot reply
                        request - the initial request

                    This handler can be called when the GUI server connection
                    is lost. In that case, `success` is False, and `reply`
                    is the `reason` of failure (string).
                    In case the call fails or times out, `success` is also
                    False.
                    The request Hash can only include the `token` on failure

    :param instance_id: Device ID of the device whose slot will be called
    :param slot_name: Name of the slot which will be called
    :param **kwargs: The argument list of the slot being called. `token` is
                     allowed to be a key-word argument

    :returns token: A unique identifier for the call

    NOTE: You had better know what you are doing here. Misuse of this feature
    can seriously degrade performance of devices external to the GUI server.
    """
    # Verify that the handler takes the correct number of arguments
    sig = signature(handler)
    if len(sig.parameters) not in (2, 3):
        raise ValueError(
            'A slot callback handler must take either two or three arguments')

    # Prepare the parameters Hash
    params = Hash()
    for k, v in kwargs.items():
        params.set(k, v)

    # Call the slot and return the token
    return get_manager().callDeviceSlot(handler, instance_id,
                                        slot_name, params)


def send_property_changes(proxies):
    """Sends a reconfiguration request to the GUI server

    :param proxies: A sequence (list, tuple, set) of PropertyProxy instances
                    with their `edit_value` trait set. See the function
                    `extract_sparse_configurations` for more details.
    """
    configs = extract_sparse_configurations(proxies)
    network, manager, topology = get_network(), get_manager(), get_topology()
    for device_id, config in configs.items():
        properties = [proxy for proxy in proxies
                      if proxy.root_proxy.device_id == device_id]
        device_proxy = topology.get_device(device_id)
        manager.expect_properties(device_proxy, properties)
        network.onReconfigure(device_id, config)
        # and more information
        info = Hash("type", "reconfigure", "instanceId", device_id,
                    "configuration", config)
        network.onInfo(info)


def onConfigurationUpdate(proxy, handler=None, request=False, remove=True):
    """Execute on a config update of `proxy` and request if necessary

    :param proxy: The DeviceProxy or PropertyProxy
    :param handler: The handler to be executed on update
    :param request: Boolean to set if a config should be requested
    :param remove: Boolean to set if the handler should be removed after
    """
    if not isinstance(proxy, DeviceProxy):
        root_proxy = proxy.root_proxy
    else:
        root_proxy = proxy

    def config_handler():
        if remove:
            proxy.binding.on_trait_change(config_handler, "config_update",
                                          remove=True)
        handler()

    proxy.binding.on_trait_change(config_handler, "config_update")
    if request and root_proxy.online:
        device_id = root_proxy.device_id
        get_network().onGetDeviceConfiguration(device_id)

    return config_handler


def onSchemaUpdate(proxy, handler, request=False, remove=True):
    """Execute on a schema update of `proxy` and `request` if necessary

    :param proxy: The DeviceProxy or PropertyProxy
    :param handler: The handler to be executed on update
    :param request: Boolean to set if a config should be requested
    :param remove: Boolean to set if the handler should be removed after
    """
    if not isinstance(proxy, DeviceProxy):
        root_proxy = proxy.root_proxy
    else:
        root_proxy = proxy

    def schema_handler():
        if remove:
            root_proxy.on_trait_change(schema_handler, "schema_update",
                                       remove=True)
        handler()

    root_proxy.on_trait_change(schema_handler, "schema_update")
    if request and root_proxy.online:
        device_id = root_proxy.device_id
        get_network().onGetDeviceSchema(device_id)

    return schema_handler


def get_scene_from_server(device_id, scene_name, project=None,
                          target_window=SceneTargetWindow.Dialog,
                          slot_name='requestScene', **kwargs):
    """Get a scene from the a device

    :param device_id: The deviceId of the device
    :param scene_name: The scene name
    :param project: The project owner of the scene. Default is None.
    :param target_window: The target window option. Default is Dialog.
    :param slot_name: The slot to be called to retrieve the scene
    """

    def scene_handler(dev_id, name, project, target_window, success, reply,
                      request):
        """Callback handler for a request to a device"""

        if not success:
            reason, details = get_reason_parts(reply)
            msg = (f"Scene '{name}' from device '{dev_id}' was not retrieved."
                   "<br><br>The reason is:<br>"
                   f"<i>{reason}</i><br><br>")
            messagebox.show_warning(msg, title='Load Scene from Device Failed',
                                    details=details)
            return
        elif not reply.get('payload.success', False):
            reason = reply.get("payload.reason", "No reason specified")
            msg = (f"Retrieval of scene '{name}' from device '{dev_id}' "
                   f"reported an error: {reason}.")
            messagebox.show_warning(msg, title='Load Scene from Device Failed')
            return

        data = reply.get('payload.data', '')
        if not data:
            msg = 'Scene "{}" from device "{}" contains no data!'
            messagebox.show_warning(msg.format(name, dev_id),
                                    title='Load Scene from Device Failed')
            return

        with StringIO(data) as fp:
            scene = read_scene(fp)
            scene.modified = True
            scene.simple_name = f'{dev_id}|{name}'
            scene.reset_uuid()

        # Add to the project AND open it
        event_type = KaraboEvent.ShowUnattachedSceneView
        window = SceneTargetWindow.MainWindow
        if target_window is not None:
            window = target_window
        if project is not None:
            event_type = KaraboEvent.ShowSceneView
            project.scenes.append(scene)

        # TODO: Repair scene fonts here! 2.11.0, to be removed 2.13?
        def visitor(model):
            substitute_font(model)

        event_data = {'model': scene, 'target_window': window}
        position = request["args"].get("position")
        if position is not None:
            event_data["position"] = position

        walk_traits_object(scene, visitor_func=visitor)
        broadcast_event(event_type, event_data)

    handler = partial(scene_handler, device_id, scene_name, project,
                      target_window)
    return call_device_slot(handler, device_id, slot_name=slot_name,
                            name=scene_name, **kwargs)


def get_macro_from_server(device_id, macro_name, project):
    """Get a macro from a device

    :param device_id: The deviceId of the device
    :param macro_name: The macro name
    :param project: The project owner of the macro. Macros must have a project
    """

    def macro_handler(dev_id, name, project, success, reply):
        if not success or not reply.get('payload.success', False):
            msg = 'Macro "{}" from device "{}" was not retrieved!'
            messagebox.show_warning(msg.format(name, dev_id),
                                    title='Load Macro from Device Failed')
            return

        data = reply.get('payload.data', '')
        if not data:
            msg = 'Macro "{}" from device "{}" contains no data!'
            messagebox.show_warning(msg.format(name, dev_id),
                                    title='Load Macro from Device Failed')
            return

        with StringIO(data) as fp:
            macro = read_macro(fp)
            macro.initialized = macro.modified = True
            macro.simple_name = f'{dev_id}-{name}'
            macro.reset_uuid()

        # Macro's can only be added to project. Hence, add first to the project
        project.macros.append(macro)
        # and then open it
        broadcast_event(KaraboEvent.ShowMacroView, {'model': macro})

    handler = partial(macro_handler, device_id, macro_name, project)

    return call_device_slot(handler, device_id, slot_name='requestMacro',
                            name=macro_name)


def retrieve_default_scene(device_id):
    """Retrieve a default scene from device with `device_id`"""
    attrs = get_topology().get_attributes(f"device.{device_id}")
    if attrs is None:
        msg = f"The device <b>{device_id}</b> is not online."
        get_logger().info(msg)
        return

    capabilities = attrs.get("capabilities", 0)
    bit = Capabilities.PROVIDES_SCENES

    if (capabilities & bit) != bit:
        msg = f"The device <b>{device_id}</b> does not provide a scene."
        get_logger().info(msg)
        return

    proxy = get_topology().get_device(device_id)

    def config_update():
        proxy.on_trait_change(config_update, "config_update", remove=True)
        proxy.remove_monitor()
        get_proxy_scene(proxy, request=False)

    def get_proxy_scene(proxy, request=True):
        scenes = proxy["availableScenes"]
        if request and (scenes is None or scenes.value is Undefined):
            proxy.on_trait_change(config_update, "config_update")
            proxy.add_monitor()
        elif not len(scenes.value):
            get_logger().info(
                "The device <b>{}</b> does not specify a scene "
                "name!".format(device_id))
        else:
            scene_name = scenes.value[0]
            get_scene_from_server(device_id, scene_name)

    get_proxy_scene(proxy)


def onShutdown(device_proxy, handler, parent=None):
    """Shutdown a device_proxy and perform an action handler on device_proxy

    Note: The device must be online!
    """

    def _offline_handler(device_proxy, name, new):
        """Handle a device getting a new configuration
        """
        global _waiting_devices
        handler, timer = _waiting_devices.pop(device_proxy,
                                              (lambda: None, None))

        # Remove the trait handler
        device_proxy.on_trait_change(_offline_handler, "online", remove=True)

        # Clear the timer
        timer.stop()
        # Execute the handler
        handler()

    @Slot()
    def _timeout_handler(device_proxy):
        """Handle our wait timer expiring"""
        global _waiting_devices
        _waiting_devices.pop(device_proxy, None)

        # Remove the trait handler
        device_proxy.on_trait_change(_offline_handler, "online",
                                     remove=True)

        # Inform the user
        msg = f"The device didn't shutdown in {_WAIT_SECONDS} ... aborting."
        msg = msg.format(device_proxy.device_id, _WAIT_SECONDS)
        messagebox.show_warning(msg, parent=parent)

    if not device_proxy.online:
        msg = "The device is already offline ..."
        messagebox.show_error(msg, parent=parent)
        return

    global _waiting_devices
    if device_proxy in _waiting_devices:
        # Already waiting
        return

    timer = QTimer()
    timer.setSingleShot(True)
    timer.timeout.connect(partial(_timeout_handler, device_proxy))
    device_proxy.on_trait_change(_offline_handler, 'online')

    _waiting_devices[device_proxy] = (handler, timer)
    timer.start(_WAIT_SECONDS * 1000)

    instanceId = device_proxy.device_id
    # Shutdown the device
    get_network().onKillDevice(instanceId)
