from enum import Enum, IntEnum


class Capabilities(IntEnum):
    PROVIDES_SCENES = 1
    # add future capabilities as bit maskable properties:
    # FUTURE_CAPABILITY = 2
    # SOME_OTHER_CAPABILITY = 4
    # ...


class DeviceStatus(Enum):
    # device could, but is not started
    OFFLINE = 'offline'
    # the device is online, but no detailed information retrieved yet
    OK = 'ok'
    # the device is online but doesn't have a schema yet
    ONLINE = 'online'
    # online device waiting for its schema
    ONLINEREQUESTED = 'onlinerequested'
    # everything is up-and-running
    ALIVE = 'alive'
    # we are registered to monitor this device
    MONITORING = 'monitoring'
    # a schema is requested, but didnt arrive yet
    REQUESTED = 'requested'
    # the device has a schema, but no value yet
    SCHEMA = 'schema'
    # the device is dead
    DEAD = 'dead'
    # device server not available
    NOSERVER = 'noserver'
    # class plugin not available
    NOPLUGIN = 'noplugin'
    # device running, but of different type
    INCOMPATIBLE = 'incompatible'
    MISSING = 'missing'
    ERROR = 'error'


# The device is online in these status
ONLINE_STATUSES = (
    DeviceStatus.OK, DeviceStatus.ONLINE, DeviceStatus.ALIVE,
    DeviceStatus.ONLINEREQUESTED, DeviceStatus.MONITORING,
    DeviceStatus.SCHEMA, DeviceStatus.ERROR
)


# The device is not able to receive configuration
NO_CONFIG_STATUSES = (
    DeviceStatus.NOPLUGIN, DeviceStatus.NOSERVER, DeviceStatus.REQUESTED
)
