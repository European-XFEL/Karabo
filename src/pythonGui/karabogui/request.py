#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on May 9, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
from inspect import signature
from io import StringIO

from traits.api import Undefined

from karabo.common.enums import Capabilities
from karabo.common.project.macro import read_macro
from karabo.common.scenemodel.const import SceneTargetWindow
from karabo.common.scenemodel.io import read_scene
from karabo.common.traits import walk_traits_object
from karabo.native import Hash
from karabogui import messagebox
from karabogui.binding.api import DeviceProxy, extract_sparse_configurations
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.fonts import substitute_font
from karabogui.logger import get_logger
from karabogui.singletons.api import get_manager, get_network, get_topology
from karabogui.util import get_reason_parts


def call_device_slot(handler, instance_id, slot_name, **kwargs):
    """Call a device slot via the GUI server. This works with slots which
    take a single `Hash` as an argument and reply with a `Hash`.

    :param handler: Callable with the signature handler(success, reply)
                    where:
                        success - a bool indicating whether the call succeeded
                        reply - a Hash containing the slot reply
                    This handler can be called when the GUI server connection
                    is lost. In that case, `success` is False, and `reply`
                    is the `reason` of failure (string).
                    In case the call fails or times out, `success` is also
                    False.

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
    if len(sig.parameters) != 2:
        raise ValueError('A slot callback handler must take two arguments')

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

    def scene_handler(dev_id, name, project, target_window, success, reply):
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
            scene.simple_name = '{}|{}'.format(dev_id, name)
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

        walk_traits_object(scene, visitor_func=visitor)
        broadcast_event(event_type, {'model': scene, 'target_window': window})

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
            macro.simple_name = '{}-{}'.format(dev_id, name)
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
