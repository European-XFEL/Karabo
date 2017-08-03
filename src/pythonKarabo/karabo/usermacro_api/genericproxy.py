"""The Generic Proxy module contains
all generalized interface to devices
"""
from asyncio import coroutine, wait_for
from karabo.middlelayer import (connectDevice, KaraboError, Proxy,
                                waitUntilNew)


class GenericProxy(object):
    """Base class for all generic proxies"""
    _proxy = Proxy()
    _proxies = None
    locker = ""

    def __repr__(self):
        if self._proxies is None:
            rep = "{}('{}')".format(type(self).__name__,
                                    self._proxy.deviceId)
        else:
            rep = "{}(".format(type(self).__name__)
            for gp in self._proxies:
                if gp._proxies is None:
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
        if type(proxies[0]) in cls.__subclasses__():
            obj = object.__new__(cls)
            obj._proxies = proxies
            return obj
        for subclass in cls.__subclasses__():
            gp_container = subclass.create_generic_proxy_container(proxies)
            if gp_container:
                return gp_container

    def __new__(cls, *args):
        if len(args) == 0:
            # Act as a container
            pass

        elif len(args) == 1:
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
                # I'm a container with a single generic proxy
                return cls.create_generic_proxy_container(args)

        else:
            # Act as container: instantiate all the proxies
            # if they are not already
            proxies = []
            for device in args:
                if isinstance(device, GenericProxy):
                    proxies.append(device)
                else:
                    # Assume it's a string,
                    # and delegate the problem to instantiation if it isn't
                    proxy = connectDevice(device)
                    proxies.append(cls.create_generic_proxy(proxy))
            if all(isinstance(dev, type(proxies[0]).__bases__[0])
                   for dev in proxies):
                    return cls.create_generic_proxy_container(proxies)
            cls.error("Provided different types of Devices")

    @coroutine
    def lock_on(self, locker):
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

    @coroutine
    def lock_off(self):
        """Release lock"""
        current_locker = self._proxy.lockedBy
        if self.locker != current_locker:
            self.error("{} is locked by {}."
                       .format(self._proxy.deviceId, current_locker))
        self._proxy.lockedBy = ""

    @property
    def deviceId(self):
        return self._proxy.deviceId


class Movable(GenericProxy):
    """Movable generic proxy

    This is a generalized interface to motors, virtual motors
    or wizard devices (those requiring human
    to press for e.g. a next button)
    """
    current_step = 0

    @property
    def position(self):
        """Get the position"""
        raise NotImplementedError

    @coroutine
    def moveto(self, pos):
        """Move to *pos*"""
        print("Montor will move to {}".format(pos))
        raise NotImplementedError

    @coroutine
    def stop(self):
        """Stop"""
        raise NotImplementedError

    @coroutine
    def step(self, stp):
        """Move to step *stp*"""
        self.current_step = stp

    @coroutine
    def home(self):
        """Home"""
        raise NotImplementedError


class Sensible(GenericProxy):
    """Sensible generic proxy

    This is a generalized interface to detectors, sensors
    and image processors
    """
    @coroutine
    def acquire(self):
        """Start acquisition"""
        print("Sensible will start acquire.")
        raise NotImplementedError

    @coroutine
    def stop(self):
        """Stop acquisition"""
        print("Sensible will stop.")
        raise NotImplementedError


class Coolable(GenericProxy):
    """Generalized interface to coolers"""

    @property
    def temperature(self):
        """Get the temperature from the device"""
        raise NotImplementedError

    @temperature.setter
    @coroutine
    def cool(self, temperature):
        """Cool to *temperature*"""
        raise NotImplementedError

    @coroutine
    def stop(self):
        """Stop cooling"""
        raise NotImplementedError


class Pumpable(GenericProxy):
    """Generalized interface to pumps"""
    @coroutine
    def pump(self):
        """Start pumping"""
        raise NotImplementedError

    @coroutine
    def stop(self):
        """Stop pumping"""
        raise NotImplementedError


class Closable(GenericProxy):
    """Generalized interface to valves"""
    @coroutine
    def open(self):
        """Open it"""
        raise NotImplementedError

    @coroutine
    def close(self):
        """Close it"""
        raise NotImplementedError
