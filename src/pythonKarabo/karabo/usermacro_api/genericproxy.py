"""The Generic Proxy module contains
all generalized interface to devices
"""
from karabo.middlelayer import getDevice
from karabo.middlelayer_api.proxy import ProxyBase


class GenericProxy(object):
    """Base class for all generic proxies"""
    proxy = ProxyBase()

    def __init__(self, deviceId, **kwargs):
        pass

    def __new__(cls, deviceId, *args, **kwargs):
        super().__new__(args, **kwargs)
        d = getDevice(deviceId)
        # .... e.g. turn a movable
        return Movable(deviceId, *args, **kwargs)

    def lock_on(self, locker):
        self.proxy.lockedBy = locker

    def lock_off(self):
        self.proxy.lockedBy = ""


class Movable(GenericProxy):
    """Movable generic proxy

    This is a generalized interface to motors, virtual motors
    or wizard devices (those requiring human
    to press for e.g. a next button)
    """
    position = 0
    stp = 0

    def __init__(self, deviceId, *args, **kwargs):
        super().__init__(deviceId, *args, **kwargs)

    def __new__(cls, *args, **kwargs):
        super().__new__(args, **kwargs)

    def move_to(self, pos):
        """Move the *pos*"""
        pass

    def stop(self):
        """Stop"""
        pass

    def step(self, stp):
        """Move to step *stp*"""
        pass

    def home(self):
        """Home"""
        pass


class Sensible(GenericProxy):
    """Sensible generic proxy

    This is a generalized interface to detectors, sensors
    and image processors
    """
    def __init__(self, deviceId, *args, **kwargs):
        super().__init__(deviceId, *args, **kwargs)

    def __new__(cls, *args, **kwargs):
        super().__new__(args, **kwargs)

    def acquire(self):
        """Start acquisition"""
        pass

    def stop(self):
        """Stop acquisition"""
        pass


class Coolable(GenericProxy):
    """Generalized interface to coolers"""
    temperature = 0

    def __init__(self, deviceId, *args, **kwargs):
        super().__init__(deviceId, *args, **kwargs)

    def __new__(cls, *args, **kwargs):
        super().__new__(args, **kwargs)

    def cool(self, temperature):
        """Cool to *temperature*"""
        pass

    def stop(self):
        """Stop cooling"""
        pass


class Pumpable(GenericProxy):
    """Generalized interface to pumps"""
    def __init__(self, deviceId, *args, **kwargs):
        super().__init__(deviceId, *args, **kwargs)

    def __new__(cls, *args, **kwargs):
        super().__new__(args, **kwargs)

    def pump(self):
        """Start pumping"""
        pass

    def stop(self):
        """Stop pumping"""
        pass


class Closable(GenericProxy):
    """Generalized interface to valves"""
    def __init__(self, deviceId, *args, **kwargs):
        super().__init__(deviceId, *args, **kwargs)

    def __new__(cls, deviceId, *args, **kwargs):
        super().__new__(cls, args, **kwargs)

    def open(self):
        """Open it"""
        pass

    def close(self):
        """Close it"""
        pass
