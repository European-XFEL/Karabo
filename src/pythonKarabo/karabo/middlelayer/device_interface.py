# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
from karabo.common.enums import Capabilities, Interfaces
from karabo.middlelayer.device_client import getTopology

DEVICE = "device"


def _test_word(word, bit):
    """Test the bit word with our capabilities and interfaces enums
    """
    if (word & bit.value) == bit.value:
        return True

    return False


def _get_interface_device(interfaces, matchPattern=None):
    """Get the specific interface type from the systemTopology

    :param interfaces: Search for device interfaces
    :param: matchPattern: Optionally provide a string pattern to find the
                          deviceId's containing the matchPattern.

    :type interfaces: List of Enums
    :type matchPattern: str
    """
    ret = []
    topology = getTopology()[DEVICE]
    for k, v, a in topology.iterall():
        has_interfaces = _test_word(a["capabilities"],
                                    Capabilities.PROVIDES_INTERFACES)
        if has_interfaces and _test_word(a["interfaces"], interfaces):
            ret.append(k)

    if matchPattern is not None:
        ret = [dev for dev in ret if matchPattern.lower() in dev.lower()]

    return ret


def listMotors(matchPattern=None):
    """List all the Motors from the systemTopology

    Parameters
    ----------
    matchPattern: Optionally provide a string pattern to find the
                  deviceId's containing the matchPattern.
    Interface
    ---------
    A full abstract motor interface provides the following:

    - Properties: actualPosition, targetPosition, isCWLimit, isCCWLimit

      Optionally, if provided, there will be: isSWLimitHigh, isSWLimitLow,
      velocity, backlash

    - Commands: move, stop

    Returns
    -------
    List of found deviceId's of motor devices in the system topology
    """
    ret = _get_interface_device(Interfaces.Motor, matchPattern)

    return ret


def listMultiAxisMotors(matchPattern=None):
    """List all the MultiAxisMotors from the systemTopology

    Parameters
    ----------
    matchPattern: Optionally provide a string pattern to find the
                  deviceId's containing the matchPattern.
    Interface
    ---------
    A MultiAxisMotor contains full abstract motor interface in nodes.
    All nodes are specified in the ``axes`` property.

    Each node contains the following properties:

    - Properties: actualPosition, targetPosition, isCWLimit, isCCWLimit

      Optionally, if provided, there will be: isSWLimitHigh, isSWLimitLow,
      velocity, backlash

    - Commands: move, stop

    Returns
    -------
    List of found deviceId's of multiaxis motor devices in the system topology
    """
    ret = _get_interface_device(Interfaces.MultiAxisMotor, matchPattern)

    return ret


def listCameras(matchPattern=None):
    """List all the Cameras from the systemTopology

    Parameters
    ----------
    matchPattern: Optionally provide a string pattern to find the
                  deviceId's containing the matchPattern.

    Interface
    ---------
    A device described by a camera interface continously acquires data and thus
    does not automatically stop.
    An abstract camera interface is described by:

    - Commands: start, stop

    Returns
    -------
    List of found deviceId's of camera devices in the system topology
    """
    ret = _get_interface_device(Interfaces.Camera, matchPattern)

    return ret


def listDeviceInstantiators(matchPattern=None):
    """List all the DeviceInstantiators from the systemTopology

    Parameters
    ----------
    matchPattern: Optionally provide a string pattern to find the
                  deviceId's containing the matchPattern.

    Interface
    ---------
    A device described by a device instantiator interface that is capable of
    instantiating devices.

    - Commands: startInstantiate

    Returns
    -------
    List of found deviceId's of device instantiators in the system topology
    """
    ret = _get_interface_device(Interfaces.DeviceInstantiator, matchPattern)

    return ret


def listTriggers(matchPattern=None):
    """List all the Triggers from the systemTopology

    Parameters
    ----------
    matchPattern: Optionally provide a string pattern to find the
                  deviceId's containing the matchPattern.

    Interface
    ---------
    A device described by a trigger interface acquires data for a defined
    period, e.g. numberOfTrains or acquisitionTime

    - Properties: acquisitionTime (float: in seconds)
    - Commands: start, stop

    Returns
    -------
    List of found deviceId's of trigger devices in the system topology
    """
    ret = _get_interface_device(Interfaces.Trigger, matchPattern)

    return ret


def listProcessors(matchPattern=None):
    """List all the Processors from the systemTopology

    Parameters
    ----------
    matchPattern: Optionally provide a string pattern to find the
                  deviceId's containing the matchPattern.
    Interface
    ---------
    A Processor is a pipelining device that depends on receiving data via
    an input channel to update its properties

    - States.ON: if signalEndOfStream is received or no data is processed
    - State.PROCESSING: Once data is received, the devices goes to this state

    Returns
    -------
    List of found deviceId's of processor devices in the system topology
    """
    ret = _get_interface_device(Interfaces.Processor, matchPattern)

    return ret
