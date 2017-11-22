#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on May 9, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from inspect import signature
import uuid

from karabo.middlelayer import Hash
from karabogui.singletons.api import get_manager


def call_device_slot(handler, device_id, slot_name, **kwargs):
    """Call a device slot via the GUI server. This works with slots which
    take a single `Hash` as an argument and reply with a `Hash`.

    :param handler: Callable with the signature handler(success, reply)
                    where:
                        success - a bool indicating whether the call succeeded
                        reply - a Hash containing the slot reply
                    This handler can be called when the GUI server connection
                    is lost. In that case, `success` is False, and `reply`
                    should be ignored.
                    In case the call fails or times out, `success` is also
                    False and `reply` may or may not contain useful info.
    :param device_id: Device ID of the device whose slot will be called
    :param slot_name: Name of the slot which will be called
    :param **kwargs: The argument list of the slot being called
    :returns token: A unique identifier for the call

    NOTE: You had better know what you are doing here. Misuse of this feature
    can seriously degrade performance of devices external to the GUI server.
    """
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

    # Call the slot
    get_manager().callDeviceSlot(token, handler, device_id, slot_name, params)

    return token
