"""The Generic Proxy module contains
all generalized interface to devices
"""
from asyncio import wait_for

from karabo.common.states import StateSignifier
from karabo.middlelayer import (
    allCompleted, connectDevice, KaraboError,
    Proxy, waitUntilNew)
from karabo.middlelayer_api.proxy import synchronize


class GenericProxy(object):
    """Base class for all generic proxies"""
    state_mapping = {}
    preparation_timeout = 10
    locker = ""
    _proxy = Proxy()
    _generic_proxies = []

    def __repr__(self):
        def _repr_gproxy(gproxy):
            return "{}('{}'), ".format(type(gproxy).__name__,
                                       gproxy._proxy.deviceId)

        def _repr_gproxy_container(gproxy):
            return "{}, ".format(gproxy)

        if self._generic_proxies:
            rep = "{}(".format(type(self).__name__)
            for gproxy in self._generic_proxies:
                if gproxy._generic_proxies:
                    rep += _repr_gproxy_container(gproxy)
                else:
                    rep += _repr_gproxy(gproxy)
            rep = rep[:-2] + ")"
        else:
            rep = _repr_gproxy(self)[:-2]
        return rep

    @classmethod
    def _error(cls, msg):
        raise KaraboError(msg)

    @classmethod
    def create_generic_proxy(cls, proxy):
        """Create generalized interface from device ID"""
        if proxy.classId in getattr(cls, 'generalizes', []):
            obj = object.__new__(cls)
            obj._proxy = proxy
            return obj
        for subclass in cls.__subclasses__():
            generic_proxy = subclass.create_generic_proxy(proxy)
            if generic_proxy:
                return generic_proxy

    @classmethod
    def create_generic_proxy_container(cls, gproxies):
        """Create generalized interface from generic proxies"""
        for subclass in GenericProxy.__subclasses__():
            if isinstance(gproxies[0], subclass):
                obj = object.__new__(subclass)
                obj._generic_proxies = gproxies
                return obj

    def __new__(cls, *args):
        if args:
            if len(args) == 1:
                deviceId = args[0]
                if isinstance(deviceId, str):
                    # Act as a generic proxy
                    try:
                        proxy = connectDevice(deviceId)
                        return cls.create_generic_proxy(proxy)
                    except:
                        cls._error("Could not connect to {}. Is it on?"
                                   .format(deviceId))
                else:
                    # Act as a container with a single generic proxy
                    return cls.create_generic_proxy_container(args)

            else:
                # Act as container and instantiate all the proxies
                # if they are not already
                gproxies = []
                for device in args:
                    if isinstance(device, GenericProxy):
                        gproxy = device
                    else:
                        # Assume it's a string, and delegate the problem
                        # to instantiation if it isn't
                        gproxy = GenericProxy(device)

                    if not gproxies or (gproxies
                                        and isinstance(gproxy,
                                                       type(gproxies[0])
                                                       .__bases__[0])):
                        gproxies.append(gproxy)
                    else:
                        cls._error("Provided different types of Devices")

                return cls.create_generic_proxy_container(gproxies)

    @synchronize
    def lockon(self, locker):
        """Lock device"""
        if self._generic_proxies:
            for gproxy in self._generic_proxies:
                yield from gproxy.lockon(locker)
        else:
            while self._proxy.lockedBy != locker:
                if self._proxy.lockedBy == "":
                    self._proxy.lockedBy = locker
                else:
                    try:
                        print("waiting for lock!")
                        wait_for(waitUntilNew(self._proxy.lockedBy),
                                 timeout=2)
                    except TimeoutError:
                        self._error("Could not lock {}"
                                    .format(self._proxy.classId))
        self.locker = locker

    @synchronize
    def lockoff(self):
        """Release lock"""
        if self._generic_proxies:
            for gproxy in self._generic_proxies:
                yield from gproxy.lockoff()
        else:
            current_locker = self._proxy.lockedBy
            if self.locker != current_locker:
                self._error("{} is locked by {}."
                            .format(self._proxy.deviceId, current_locker))
            self._proxy.lockedBy = ""

        self.locker = None

    @synchronize
    def prepare(self):
        """Get ready to be used"""
        if self._generic_proxies:
            yield from allCompleted(**{gproxy.deviceId: gproxy.prepare()
                                       for gproxy in self._generic_proxies})

    @property
    def deviceId(self):
        """Get device ID"""
        if self._generic_proxies:
            return repr(self)
        return self._proxy.deviceId

    @property
    def state(self):
        """Get state"""
        if self._generic_proxies:
            signifier = StateSignifier()
            return signifier.returnMostSignificant(
                [gproxy.state for gproxy in self._generic_proxies])

        return self.state_mapping.get(self._proxy.state, self._proxy.state)


class Movable(GenericProxy):
    """Movable generic proxy

    This is a generalized interface to motors, virtual motors
    or wizard devices (those requiring human
    to press for e.g. a next button)
    """
    @property
    def position(self):
        """"Position getter """
        if self._generic_proxies:
            return [gproxy.position for gproxy in self._generic_proxies]

        return self._proxy.encoderPosition

    @position.setter
    def position(self, value):
        self.moveto(value)

    @synchronize
    def moveto(self, pos):
        """Move to *pos*"""
        if self._generic_proxies:
            itpos = iter(pos)
            for gproxy in self._generic_proxies:
                yield from gproxy.moveto(next(itpos))
        else:
            self._proxy.targetPosition = pos
            yield from self._proxy.move()

    @synchronize
    def stop(self):
        """Stop"""
        if self._generic_proxies:
            for gproxy in self._generic_proxies:
                yield from gproxy.stop()
        else:
            yield from self._proxy.stop()

    @synchronize
    def home(self):
        """Home"""
        if self._generic_proxies:
            for gproxy in self._generic_proxies:
                yield from gproxy.home()
        else:
            yield from self._proxy.home()


class Sensible(GenericProxy):
    """Sensible generic proxy

    This is a generalized interface to detectors, sensors
    and image processors
    """
    @synchronize
    def acquire(self):
        """Start acquisition"""
        if self._generic_proxies:
            for gproxy in self._generic_proxies:
                yield from gproxy.acquire()
        else:
            yield from self._proxy.acquire()

    @synchronize
    def stop(self):
        """Stop acquisition"""
        if self._generic_proxies:
            for gproxy in self._generic_proxies:
                yield from gproxy.stop()
        else:
            yield from self._proxy.stop()


class Coolable(GenericProxy):
    """Generalized interface to coolers"""
    @property
    def temperature(self):
        """"Temperature getter """
        if self._generic_proxies:
            return [gproxy.temperature
                    for gproxy in self._generic_proxies]
        return self._proxy.currentColdHeadTemperature

    @temperature.setter
    def temperature(self, value):
        self.cool(value)

    @synchronize
    def cool(self, temperature):
        """Cool to *temperature*"""
        if self._generic_proxies:
            ittemp = iter(temperature)
            for gproxy in self._generic_proxies:
                yield from gproxy.cool(next(ittemp))
        else:
            self._proxy.targetCoolTemperature = temperature
            yield from self._proxy.stop()

    @synchronize
    def stop(self):
        """Stop cooling"""
        if self._generic_proxies:
            for gproxy in self._generic_proxies:
                yield from gproxy.stop()
        else:
            yield from self._proxy.stop()


class Pumpable(GenericProxy):
    """Generalized interface to pumps"""

    @property
    def pressure(self):
        """Pressure getter"""
        if self._generic_proxies:
            return [gproxy.pressure
                    for gproxy in self._generic_proxies]
        return self._proxy.pressure

    @synchronize
    def pump(self):
        """Start pumping"""
        if self._generic_proxies:
            for gproxy in self._generic_proxies:
                yield from gproxy.pump()
        else:
            yield from self._proxy.pump()

    @synchronize
    def stop(self):
        """Stop pumping"""
        if self._generic_proxies:
            for gproxy in self._generic_proxies:
                yield from gproxy.stop()
        else:
            yield from self._proxy.stop()


class Closable(GenericProxy):
    """Generalized interface to valves"""
    @synchronize
    def open(self):
        """Open it"""
        if self._generic_proxies:
            for gproxy in self._generic_proxies:
                yield from gproxy.open()
        else:
            yield from self._proxy.open()

    @synchronize
    def close(self):
        """Close it"""
        if self._generic_proxies:
            for gproxy in self._generic_proxies:
                yield from gproxy.close()
        else:
            yield from self._proxy.close()
