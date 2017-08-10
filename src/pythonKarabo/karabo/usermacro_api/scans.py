"""Scantool macros"""
from asyncio import coroutine
import itertools
import numpy as np

from karabo.middlelayer import (
    AccessMode, Bool, Float, Int32, State, String,
    sleep, UInt32, Unit, VectorHash, waitUntil)
from .usermacro import UserMacro
from .genericproxy import Movable, Sensible


class AScan(UserMacro):
    """Absolute scan"""
    experimentId = String(
        displayedName="ExperimentId",
        defaultValue="",
        accessMode=AccessMode.INITONLY)

    movableId = String(
        displayedName="Movable",
        accessMode=AccessMode.READONLY)

    sensibleId = String(
        displayedName="Sensible",
        accessMode=AccessMode.READONLY)

    exposureTime = Float(
        displayedName="Exposure time",
        defaultValue=0.1,
        unitSymbol=Unit.SECOND)

    steps = Bool(
        displayedName="HasSteps",
        defaultValue=True)

    number_of_steps = Int32(
        displayedName="Number of steps",
        defaultValue=0)

    stepNum = UInt32(
        displayedName="Current step",
        defaultValue=0,
        accessMode=AccessMode.READONLY)

    # Should be later an AcquiredData object
    data = VectorHash()

    _movable = Movable()
    _sensible = Sensible()
    _pos_list = []

    def __init__(self, movable, pos_list, sensible, exposureTime,
                 steps=True, number_of_steps=0, **kwargs):
        super().__init__(kwargs)
        self._movable = (movable
                         if isinstance(movable, Movable)
                         else Movable(movable))
        self.movableId = self._movable.deviceId
        self._pos_list = pos_list
        self._sensible = (sensible
                          if isinstance(sensible, Sensible)
                          else Sensible(sensible))
        self.exposureTime = exposureTime
        self.steps = steps
        self.number_of_steps = number_of_steps

    def onInitialization(self):
        if self.experimentId == "":
            self.experimentId = self.deviceId
        self.sensibleId = self._sensible.deviceId

    @coroutine
    def execute(self):
        """Body. Override UserMacro's"""
        # Prepare for move and acquisition
        self._sensible.exposureTime = self.exposureTime
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
            yield from sleep(self.exposureTime + self.time_epsilon)
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
    exposureTime = Float(displayedName="Exposure time",
                         defaultValue=0.1, unitSymbol=Unit.SECOND)
    duration = Float(displayedName="Duration",
                     defaultValue=1, unitSymbol=Unit.SECOND)
    _sensible = Sensible()

    def __init__(self, sensible, exposureTime, duration, **kwargs):
        super().__init__(kwargs)
        self._sensible = (sensible
                          if isinstance(sensible, Sensible)
                          else Sensible(sensible))
        self._sensibleId = self._sensible.deviceId
        self.exposureTime = exposureTime
        self.duration = duration

    @coroutine
    def execute(self):
        """Body. Override UserMacro's"""

        # Prepare for acquisition
        self._sensible.exposureTime = self.exposureTime
        yield from self._sensible.prepare()

        linelen = 80
        print("\n"+"-"*linelen)
        # Iterate over positions
        elaps = 0
        i = 0
        while elaps < self.duration:
            i += 1
            print("Step {} - at time {}"
                  .format(i, elaps))
            yield from self._sensible.acquire()
            yield from sleep(self.exposureTime + self.time_epsilon)
            yield from self._sensible.stop()
            elaps += self.exposureTime + self.time_epsilon

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
