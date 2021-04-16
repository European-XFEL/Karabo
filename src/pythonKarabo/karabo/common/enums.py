from enum import Enum, IntEnum


class Capabilities(IntEnum):
    PROVIDES_SCENES = 1
    PROVIDES_MACROS = 2
    PROVIDES_INTERFACES = 4
    # add future capabilities as bit maskable properties:
    # FUTURE_CAPABILITY = 8
    # SOME_OTHER_CAPABILITY = 16
    # ...


class Interfaces(IntEnum):
    Motor = 1
    MultiAxisMotor = 2
    Trigger = 4
    Camera = 8
    Processor = 16
    DeviceInstantiator = 32
    # add future interfaces as bit maskable properties:
    # FUTURE_INTERFACE = 64
    # SOME_OTHER_INTERFACE = 128
    # ...


class ProxyStatus(Enum):
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
    ProxyStatus.OK, ProxyStatus.ONLINE, ProxyStatus.ALIVE,
    ProxyStatus.ONLINEREQUESTED, ProxyStatus.MONITORING,
    ProxyStatus.SCHEMA, ProxyStatus.ERROR
)


SCHEMA_STATUSES = (
    ProxyStatus.ALIVE, ProxyStatus.MONITORING, ProxyStatus.SCHEMA,
)


# The device is not able to receive configuration
NO_CONFIG_STATUSES = (
    ProxyStatus.NOPLUGIN, ProxyStatus.NOSERVER, ProxyStatus.REQUESTED
)
