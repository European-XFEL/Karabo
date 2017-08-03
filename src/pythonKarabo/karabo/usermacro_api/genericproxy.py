"""The Generic Proxy module contains
all generalized interface to devices
"""
from karabo.middlelayer import connectDevice
from karabo.middlelayer import KaraboError
from karabo.middlelayer_api.proxy import ProxyBase, synchronize


class GenericProxy(object):
    """Base class for all generic proxies"""
    _proxy = ProxyBase()
    locker = ""
    state_mapping = {}

    def __repr__(self):
        # For now, return the internal proxy representation
        return self._proxy.__repr__()

    @classmethod
    def create_generic_proxy(cls, proxy):
        """Create generalized interface"""
        if proxy.classId in getattr(cls, 'generalizes', []):
            obj = object.__new__(cls)
            obj._proxy = proxy
            return obj
        for subclass in cls.__subclasses__():
            generic_proxy = subclass.create_generic_proxy(proxy)
            if generic_proxy:
                return generic_proxy

    def __new__(cls, deviceId):
        proxy = connectDevice(deviceId)
        return cls.create_generic_proxy(proxy)

    @synchronize
    def lockon(self, locker):
        """Lock device"""
        current_locker = self._proxy.lockedBy
        if locker != current_locker:
            raise KaraboError("{} is already locked by {}."
                              .format(self._proxy.deviceId, current_locker))
        self._proxy.lockedBy = locker
        self.locker = locker

    @synchronize
    def lockoff(self):
        """Release lock"""
        current_locker = self._proxy.lockedBy
        if self.locker != current_locker:
            raise KaraboError("{} is locked by {}."
                              .format(self._proxy.deviceId, current_locker))
        self._proxy.lockedBy = ""

    @property
    def state(self):
        """Get state"""
        return self.state_mapping.get(self._proxy.state, self._proxy.state)


class Movable(GenericProxy):
    """Movable generic proxy

    This is a generalized interface to motors, virtual motors
    or wizard devices (those requiring human
    to press for e.g. a next button)
    """
    @property
    def actualPosition(self):
        """"Position getter """
        return self._proxy.encoderPosition

    @actualPosition.setter
    def actualPosition(self, value):
        self.moveto(value)

    @synchronize
    def moveto(self, pos):
        """Move to *pos*"""
        self._proxy.targetPosition = pos
        yield from self._proxy.move()

    @synchronize
    def stop(self):
        """Stop"""
        yield from self._proxy.stop()

    @synchronize
    def home(self):
        """Home"""
        yield from self._proxy.home()


class Sensible(GenericProxy):
    """Sensible generic proxy

    This is a generalized interface to detectors, sensors
    and image processors
    """
    @synchronize
    def acquire(self):
        """Start acquisition"""
        yield from self._proxy.acquire()

    @synchronize
    def stop(self):
        """Stop acquisition"""
        yield from self._proxy.stop()


class Coolable(GenericProxy):
    """Generalized interface to coolers"""
    @property
    def actualTemperature(self):
        """"Temperature getter """
        return self._proxy.currentColdHeadTemperature

    @actualTemperature.setter
    def actualTemperature(self, value):
        self.cool(value)

    @synchronize
    def cool(self, temperature):
        """Cool to *temperature*"""
        self._proxy.targetCoolTemperature = temperature
        yield from self._proxy.cool()

    @synchronize
    def stop(self):
        """Stop cooling"""
        yield from self._proxy.stop()


class Pumpable(GenericProxy):
    """Generalized interface to pumps"""

    @property
    def pressure(self):
        """Pressure getter"""
        return self._proxy.pressure

    @synchronize
    def pump(self):
        """Start pumping"""
        yield from self._proxy.pump()

    @synchronize
    def stop(self):
        """Stop pumping"""
        yield from self._proxy.stop()


class Closable(GenericProxy):
    """Generalized interface to valves"""
    @synchronize
    def open(self):
        """Open it"""
        yield from self._proxy.open()

    @synchronize
    def close(self):
        """Close it"""
        yield from self._proxy.close()
