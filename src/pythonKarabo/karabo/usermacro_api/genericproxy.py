"""The Generic Proxy module contains
all generalized interface to devices
"""
from asyncio import wait_for
from karabo.middlelayer import (connectDevice, KaraboError, Proxy,
                                waitUntilNew)
from karabo.middlelayer_api.proxy import synchronize


class GenericProxy(object):
    """Base class for all generic proxies"""
    _proxy = Proxy()
    _generic_proxies = None
    locker = ""
    state_mapping = {}

    def __repr__(self):
        if self._generic_proxies is None:
            rep = "{}('{}')".format(type(self).__name__,
                                    self._proxy.deviceId)
        else:
            rep = "{}(".format(type(self).__name__)
            for gp in self._generic_proxies:
                if gp._generic_proxies is None:
                    rep += "'{}', ".format(gp._proxy.deviceId)
                else:
                    rep += "{}, ".format(gp)
            rep = rep[:-2] + ")"
        return rep

    @classmethod
    def error(cls, msg):
        raise KaraboError(msg)

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

    @classmethod
    def create_generic_proxy_container(cls, proxies):
        for subclass in GenericProxy.__subclasses__():
            if isinstance(proxies[0], subclass):
                obj = object.__new__(subclass)
                obj._generic_proxies = proxies
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
                        cls.error("Could not connect to {}. Is it on?"
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

                        if not gproxies or (gproxies and
                            isinstance(gproxy, type(gproxies[0]).__bases__[0])):
                            gproxies.append(gproxy)
                        else:
                            cls.error("Provided different types of Devices")
                return cls.create_generic_proxy_container(gproxies)

        #If args is none, act as a container


    @synchronize
    def lockon(self, locker):
        """Lock device"""
        while self._proxy.lockedBy != locker:
            if self._proxy.lockedBy == "":
                self._proxy.lockedBy = locker
            else:
                try:
                    print("waiting for lock!")
                    wait_for(waitUntilNew(self._proxy.lockedBy),
                             timeout=2)
                except TimeoutError:
                    self.error("Could not lock {}".format(self._proxy.classId))
        self.locker = locker

    @synchronize
    def lockoff(self):
        """Release lock"""
        current_locker = self._proxy.lockedBy
        if self.locker != current_locker:
            self.error("{} is locked by {}."
                       .format(self._proxy.deviceId, current_locker))
        self._proxy.lockedBy = ""

    @property
    def deviceId(self):
        return self._proxy.deviceId

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
