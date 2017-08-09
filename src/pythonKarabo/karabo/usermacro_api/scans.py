"""Scantool macros"""
from asyncio import coroutine
import itertools
import numpy as np

from karabo.middlelayer import (
    Bool, Float, Int32, State, String, sleep, waitUntil)
from .usermacro import UserMacro
from .genericproxy import Movable, Sensible


class AScan(UserMacro):
    """Absolute scan"""
    movableId = String(displayedName="Movable")
    sensibleId = String(displayedName="Sensible")
    exposureTime = Float(displayedName="Exposure time", defaultValue=0.1)
    steps = Bool(displayedName="Steps", defaultValue=True)
    number_of_steps = Int32(displayedName="Number of steps", defaultValue=0)
    _movable = Movable()
    _sensible = Sensible()
    _pos_list = []

    def __init__(self, movable, pos_list, sensible, exposureTime,
                 steps=True, number_of_steps=0, **kwargs):
        super().__init__(kwargs)
        self._movable = (movable
                         if isinstance(movable, Movable)
                         else Movable(movable))
        self._movableId = self._movable.deviceId
        self._pos_list = pos_list
        self._sensible = (sensible
                          if isinstance(sensible, Sensible)
                          else Sensible(sensible))
        self._sensibleId = self._sensible.deviceId
        self._exposureTime = exposureTime
        self._steps = steps
        self._number_of_steps = number_of_steps

    @coroutine
    def execute(self):
        """Body. Override UserMacro's"""

        # Prepare for move and acquisition
        self._sensible.exposureTime = self._exposureTime
        yield from self._movable.prepare()
        yield from self._sensible.prepare()

        linelen = 80
        print("\n"+"-"*linelen)
        # Iterate over positions
        for i, pos in enumerate(self._pos_list):
            yield from self._movable.moveto(pos)
            yield from waitUntil(
                lambda: self._movable.state != State.MOVING
                and np.linalg.norm(
                    np.subtract(self._movable.position,
                                pos)) < self.position_epsilon)
            print("Step {} - Motors at {}"
                  .format(i + 1, self._movable.position))

            yield from self._sensible.acquire()
            yield from sleep(self._exposureTime)
            yield from self._sensible.stop()
        print("-"*linelen)


class AMesh(AScan):
    """Absolute Mesh scan"""
    def __init__(self, movable, pos_list1, pos_list2, sensible, exposureTime,
                 **kwargs):
        super().__init__(movable, itertools.product(pos_list1, pos_list2),
                         sensible, exposureTime, True, 0, **kwargs)


class APathScan(AScan):
    """Absolute Mesh scan"""


class DScan(AScan):
    """Delta scan"""
    def __init__(self, movable, pos_list, sensible, exposureTime,
                 steps=True, number_of_steps=0, **kwargs):
        super().__init__(movable, pos_list, sensible, exposureTime,
                         steps, number_of_steps, **kwargs)

        # Convert position from relative to absolute
        self._pos_list = np.array(self._movable.position) + np.array(pos_list)


class TScan(UserMacro):
    """Time scan"""
    sensibleId = String(displayedName="Sensible")
    exposureTime = Float(displayedName="Exposure time", defaultValue=0.1)
    duration = Float(displayedName="Duration", defaultValue=1)
    _sensible = Sensible()

    def __init__(self, sensible, exposureTime, duration, **kwargs):
        super().__init__(kwargs)
        self._sensible = (sensible
                          if isinstance(sensible, Sensible)
                          else Sensible(sensible))
        self._sensibleId = self._sensible.deviceId
        self._exposureTime = exposureTime
        self.duration = duration

    @coroutine
    def execute(self):
        """Body. Override UserMacro's"""

        # Prepare for acquisition
        self._sensible.exposureTime = self._exposureTime
        yield from self._sensible.prepare()

        linelen = 80
        print("\n"+"-"*linelen)
        # Iterate over positions
        for i, instant in enumerate(range(self.duration, self.exposureTime)):
            print("Step {} - at time {}"
                  .format(i + 1, instant))

            yield from self._sensible.acquire()
            yield from sleep(self.exposureTime)
            yield from self._sensible.stop()
        print("-"*linelen)


class DMesh(AMesh):
    """Delta Mesh scan"""
    def __init__(self, movable, pos_list1, pos_list2, sensible, exposureTime,
                 **kwargs):
        super().__init__(movable, pos_list1, pos_list2, sensible, exposureTime,
                         **kwargs)

        # Convert position from relative to absolute
        self._pos_list = np.array(
            self._movable.position) + np.array(self._pos_list)


class AMove(UserMacro):
    """Absolute move"""
    movableId = String(displayedName="Movable")
    _movable = Movable()
    _position = []

    def __init__(self, movable, position, **kwargs):
        super().__init__(kwargs)
        self._movable = (movable
                         if isinstance(movable, Movable)
                         else Movable(movable))
        self._movableId = self._movable.deviceId
        self._position = position

    @coroutine
    def execute(self):
        """Body. Override UserMacro's"""
        def __print_motor_position():
            linelen = 80
            print("\n"+"-"*linelen)
            print("Motors at {}".format(self._movable.position))
            print("-"*linelen)

        __print_motor_position()
        yield from self._movable.prepare()
        yield from self._movable.moveto(self._position)
        yield from waitUntil(
            lambda: self._movable.state != State.MOVING and np.linalg.norm(
                np.subtract(self._movable.position,
                            self._position)) < self.position_epsilon)
        __print_motor_position()


class DMove(AMove):
    """Delta move"""
    def __init__(self, movable, position, **kwargs):
        super().__init__(movable, position, **kwargs)

        # Convert position from relative to absolute
        self._position = np.array(self._movable.position) + np.array(position)
