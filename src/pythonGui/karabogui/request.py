#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on May 9, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from inspect import signature
import uuid

from karabo.native import Hash
from karabogui.binding.api import extract_sparse_configurations
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
