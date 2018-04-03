#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on May 9, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial
from inspect import signature
import uuid
from weakref import WeakKeyDictionary

from PyQt4.QtCore import pyqtSlot, QTimer

from karabo.middlelayer import Hash
from karabogui import messagebox
from karabogui.binding.api import extract_sparse_configurations
from karabogui.events import broadcast_event, KaraboEventSender
from karabogui.singletons.api import get_manager, get_network, get_topology

# Devices which are waiting for a configuration to come back from the server
_waiting_devices = WeakKeyDictionary()
WAIT_SECONDS = 5


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


def send_property_changes(proxies):
    """Given a collection of PropertyProxy instances, gather all the user edits
    and send them to the GUI server. Then wait for an answer to come back.

    If an answer, in the form of a new device configuration, doesn't arrive
    after `WAIT_SECONDS` seconds, a warning dialog is shown. However, if a new
    configuration does arrive, it is assumed that all property edits were
    applied and the `edit_value` traits are reset accordingly.

    :param proxies: A sequence (list, tuple, set) of PropertyProxy instances
                    with their `edit_value` trait set. See the function
                    `extract_sparse_configurations` for more details.
    """
    def _config_handler(device_proxy, name, new):
        """Handle a device getting a new configuration
        """
        global _waiting_devices
        properties, timer = _waiting_devices.pop(device_proxy, ([], None))
        if len(properties) == 0:
            return

        # Remove the trait handler
        device_proxy.on_trait_change(_config_handler, 'config_update',
                                     remove=True)

        # Clear the timer and property edits
        timer.stop()
        for proxy in properties:
            proxy.revert_edit()

        # NOTE: We have to send a signal to the view to asynchronously reset
        # the apply and decline buttons
        broadcast_event(KaraboEventSender.UpdateValueConfigurator,
                        {'proxy': device_proxy})

    @pyqtSlot()
    def _timeout_handler(device_proxy):
        """Handle our wait timer expiring.
        """
        global _waiting_devices
        properties, _ = _waiting_devices.pop(device_proxy, ([], None))
        if len(properties) == 0:
            return

        # Remove the trait handler
        device_proxy.on_trait_change(_config_handler, 'config_update',
                                     remove=True)

        # Inform the user
        msg = ('The property changes submitted to device "{}" were not '
               'acknowledged after {} seconds. The changes may or may '
               'not have been successful!')
        msg = msg.format(device_proxy.device_id, WAIT_SECONDS)
        messagebox.show_warning(msg)

    def _wait_for_changes(device_proxy, properties):
        """Set all the handlers up
        """
        global _waiting_devices
        timer = QTimer()
        timer.setSingleShot(True)
        timer.timeout.connect(partial(_timeout_handler, device_proxy))
        device_proxy.on_trait_change(_config_handler, 'config_update')

        _waiting_devices[device_proxy] = (properties, timer)
        timer.start(WAIT_SECONDS * 1000)

    configs = extract_sparse_configurations(proxies)
    topology, network = get_topology(), get_network()
    for device_id, config in configs.items():
        device_proxy = topology.get_device(device_id)
        properties = [proxy for proxy in proxies
                      if proxy.root_proxy.device_id == device_id]
        _wait_for_changes(device_proxy, properties)
        network.onReconfigure(device_id, config)
