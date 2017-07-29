"""The Generic Proxy module contains
all generalized interface to devices
"""
from asyncio import coroutine
from karabo.middlelayer import getDevice
from karabo.middlelayer import KaraboError
from karabo.middlelayer_api.proxy import ProxyBase


class GenericProxy(object):
    """Base class for all generic proxies"""
    proxy = ProxyBase()
    locker = ""

    def __init__(self, **kwargs):
        self.proxy = kwargs.get('proxy')

    def __repr__(self):
        # For now, return the internal proxy representation
        return self.proxy.__repr__()

    @classmethod
    def _create_generic_proxy(cls, proxy, *args, **kwargs):
        return [cls.__new__(cls, proxy.deviceId, *args, **kwargs)
                if proxy.deviceId in getattr(cls, 'generalizes')
                else cls._create_generic_proxy(subclass, proxy,
                                               *args, **kwargs)
                for subclass in cls.__subclasses__()].pop()

    def __new__(cls, deviceId, *args, **kwargs):
        proxy = getDevice(deviceId)
        return cls._create_generic_proxy(cls, proxy, *args, **kwargs)

    @coroutine
    def lock_on(self, locker):
        """Lock device"""
        current_locker = self.proxy.lockedBy
        if locker != current_locker:
            raise KaraboError("{} is already locked by {}."
                              .format(self.proxy.deviceId, current_locker))
        self.proxy.lockedBy = locker
        self.locker = locker

    @coroutine
    def lock_off(self):
        """Release lock"""
        current_locker = self.proxy.lockedBy
        if self.locker != current_locker:
            raise KaraboError("{} is locked by {}."
                              .format(self.proxy.deviceId, current_locker))
        self.proxy.lockedBy = ""


class Movable(GenericProxy):
    """Movable generic proxy

    This is a generalized interface to motors, virtual motors
    or wizard devices (those requiring human
    to press for e.g. a next button)
    """
    position = 0
    current_step = 0

    @coroutine
    def moveto(self, pos):
        """Move to *pos*"""
        pass

    @coroutine
    def stop(self):
        """Stop"""
        pass

    @coroutine
    def step(self, stp):
        """Move to step *stp*"""
        self.current_step = stp

    @coroutine
    def home(self):
        """Home"""
        pass


class Sensible(GenericProxy):
    """Sensible generic proxy

    This is a generalized interface to detectors, sensors
    and image processors
    """
    @coroutine
    def acquire(self):
        """Start acquisition"""
        pass

    @coroutine
    def stop(self):
        """Stop acquisition"""
        pass


class Coolable(GenericProxy):
    """Generalized interface to coolers"""
    temperature = 0

    @coroutine
    def cool(self, temperature):
        """Cool to *temperature*"""
        pass

    @coroutine
    def stop(self):
        """Stop cooling"""
        pass


class Pumpable(GenericProxy):
    """Generalized interface to pumps"""
    @coroutine
    def pump(self):
        """Start pumping"""
        pass

    @coroutine
    def stop(self):
        """Stop pumping"""
        pass


class Closable(GenericProxy):
    """Generalized interface to valves"""
    @coroutine
    def open(self):
        """Open it"""
        pass

    @coroutine
    def close(self):
        """Close it"""
        pass
