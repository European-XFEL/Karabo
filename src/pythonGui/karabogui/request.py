#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on May 9, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from io import StringIO

from functools import partial

from inspect import signature
import uuid
from karabo.common.enums import ONLINE_STATUSES
from karabo.common.project.macro import read_macro
from karabo.common.scenemodel.const import SceneTargetWindow
from karabo.common.scenemodel.io import read_scene
from karabo.common.services import KARABO_DAEMON_MANAGER
from karabo.common.traits import walk_traits_object

from karabo.native import Hash
from karabogui import messagebox
from karabogui.binding.api import extract_sparse_configurations
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.fonts import substitute_font
from karabogui.singletons.api import get_manager, get_network, get_topology


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
    assert "token" not in kwargs, "No `token` in kwargs allowed"

    # Generate a unique token for the transaction
    token = uuid.uuid4().hex

    # Verify that the handler takes the correct number of arguments
    sig = signature(handler)
    if len(sig.parameters) != 2:
        raise ValueError('A slot callback handler must take two arguments')

    # Prepare the parameters Hash
    params = Hash()
    for k, v in kwargs.items():
        params.set(k, v)

    # Add token to parameters
    params["token"] = token

    # Call the slot
    get_manager().callDeviceSlot(token, handler, instance_id,
                                 slot_name, params)

    return token


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


def get_scene_from_server(device_id, scene_name, project=None,
                          target_window=SceneTargetWindow.Dialog):
    """Get a scene from the a device

    :param device_id: The deviceId of the device
    :param scene_name: The scene name
    :param project: The project owner of the scene. Default is None.
    :param target_window: The target window option. Default is Dialog.
    """

    handler = partial(handle_scene_from_server, device_id, scene_name,
                      project, target_window)
    call_device_slot(handler, device_id, 'requestScene',
                     name=scene_name)


def handle_scene_from_server(dev_id, name, project, target_window, success,
                             reply):
    """Callback handler for a request to a device to load one of its scenes.
    """
    if not success or not reply.get('payload.success', False):
        msg = 'Scene "{}" from device "{}" was not retrieved!'
        messagebox.show_warning(msg.format(name, dev_id),
                                title='Load Scene from Device Failed')
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


def handle_macro_from_server(dev_id, name, project, success, reply):
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


def request_daemon_action(serverId, hostId, action):
    """Request an action for the daemon manager

    :param serverId: The targeted `serverId`
    :param hostId: The `hostId` of the server with `serverId`
    :param action: The action to be performed, e.g. `kill`, ...
    """
    device_id = KARABO_DAEMON_MANAGER
    device = get_topology().get_device(device_id)
    # XXX: Protect here if the device is offline. We share the same
    # logic as the device scene link!
    if device is not None and device.status not in ONLINE_STATUSES:
        messagebox.show_warning("Device is not online!", "Warning")
        return

    handler = partial(handle_daemon_from_server, serverId, action)
    call_device_slot(handler, device_id, 'requestDaemonAction',
                     serverId=serverId, hostId=hostId, action=action)


def handle_daemon_from_server(serverId, action, success, reply):
    """Callback handler for a request the daemon manager"""
    if not success or not reply.get('payload.success', False):
        msg = 'The command "{}" for the server "{}" was not successful!'
        messagebox.show_warning(msg.format(action, serverId),
                                title='Daemon Service Failed')
        return

    msg = 'The command "{}" for the server "{}" was successful!'
    messagebox.show_information(msg.format(action, serverId),
                                title='Daemon Service Success!')

    return
