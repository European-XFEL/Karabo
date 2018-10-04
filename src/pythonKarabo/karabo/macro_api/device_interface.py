from karabo.common.enums import Capabilities, Interfaces
from karabo.middlelayer_api.device_client import get_instance

DEVICE = "device"


def _test_word(word, bits):
    """Test the bit word with our capabilities and interfaces enums
    """
    for bit in bits:
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
    topology = get_instance().systemTopology[DEVICE]
    for k, v, a in topology.iterall():
        has_interfaces = _test_word(a["capabilities"],
                                    [Capabilities.PROVIDES_INTERFACES])
        if has_interfaces and _test_word(a["interfaces"], interfaces):
            ret.append(k)

    if matchPattern is not None:
        ret = [dev for dev in ret if matchPattern in dev]

    return ret


def listMotors(matchPattern=None):
    """List all the Motors and MultiAxisMotors from the systemTopology

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
    ret = _get_interface_device([Interfaces.Motor, Interfaces.MultiAxisMotor],
                                matchPattern)

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
    ret = _get_interface_device([Interfaces.Camera], matchPattern)

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
    ret = _get_interface_device([Interfaces.Trigger], matchPattern)

    return ret
