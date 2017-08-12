"""Scantool macros"""
from asyncio import coroutine
import itertools
import numpy as np

from karabo.middlelayer import (
    AccessMode, Bool, Float, Int32, State, String,
    sleep, UInt32, Unit, VectorHash, waitUntil)
from .usermacro import UserMacro
from .genericproxy import Movable, Sensible


def splitTrajectory(pos_list, number_of_steps):
    """Generates a segmented trajectory"""
    fepsilon = 1e-3
    if number_of_steps == 0:
        # Path Scan case
        itpos = iter(pos_list)
        while True:
            # Instruct to pause at every point
            yield next(itpos), True
    else:
        # Step-scan case
        len_traj = 0
        itpos1, itpos2 = itertools.tee(iter(pos_list), 2)
        pos = next(itpos1)

        # Compute the trajectory length
        for nextpos in itpos1:
            v = np.subtract(nextpos, pos)
            nv = np.linalg.norm(v)
            pos = nextpos
            len_traj += nv

        len_step = len_traj / number_of_steps

        # Yield segment edges and pause points
        pos = next(itpos2)
        nextpos = next(itpos2)
        dl = len_step
        yield pos, True

        while True:
            v = np.subtract(nextpos, pos)
            nv = np.linalg.norm(v)
            if abs(dl - nv) < fepsilon * dl:
                # Acquire at the edge of this segment
                p = nextpos
                dl = len_step
                yield p, True
                nextpos = next(itpos2)
            elif dl < nv:
                # Acquire in this segment
                v1 = v / nv
                p = pos + v1 * dl
                dl = len_step
                yield p, True
            else:
                # The step spans to the next segment
                p = nextpos
                if nv > fepsilon * dl:
                    # Only command significant motions
                    dl -= nv
                    yield p, False
                nextpos = next(itpos2)
            pos = p


def meshTrajectory(pos_list1, pos_list2):
    """Generates a mesh trajectory"""
    reverse = False
    for pos1 in pos_list1:
        for pos2 in reversed(pos_list2) if reverse else pos_list2:
            yield pos1, pos2
        reverse = not reverse


class AScan(UserMacro):
    """Absolute scan
    Base class for absolute step-wise scans
    and *naive* continous scans"""
    experimentId = String(
        displayedName="ExperimentId",
        defaultValue="",
        accessMode=AccessMode.INITONLY)

    movableId = String(
        displayedName="MovableId",
        accessMode=AccessMode.READONLY)

    sensibleId = String(
        displayedName="SensibleId",
        accessMode=AccessMode.READONLY)

    pos_list = String(
        displayedName="Trajectory",
        accessMode=AccessMode.READONLY)

    exposureTime = Float(
        displayedName="Exposure time",
        defaultValue=0.1,
        unitSymbol=Unit.SECOND,
        accessMode=AccessMode.INITONLY)

    steps = Bool(
        displayedName="HasSteps",
        defaultValue=True,
        accessMode=AccessMode.INITONLY)

    number_of_steps = Int32(
        displayedName="Number of steps",
        defaultValue=0,
        accessMode=AccessMode.INITONLY)

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
        self._sensible = (sensible
                          if isinstance(sensible, Sensible)
                          else Sensible(sensible))
        self._pos_list = pos_list
        self.exposureTime = exposureTime
        self.steps = steps
        self.number_of_steps = number_of_steps

    @coroutine
    def onInitialization(self):
        """Overrides classclass  SignalSlotable's"""
        if self.experimentId == "":
            self.experimentId = self.deviceId
        self.movableId = self._movable.deviceId
        self.sensibleId = self._sensible.deviceId
        self.pos_list = self._pos_list

    @coroutine
    def execute(self):
        """Body. Override UserMacro's"""
        # Prepare for move and acquisition
        self._sensible.exposureTime = self.exposureTime
        yield from self._movable.prepare()
        yield from self._sensible.prepare()

        linelen = 80
        print("\n"+"-"*linelen)
        if not self.steps:
            print("Continuous Scan along trajectory {}".format(self._pos_list))

        # Iterate over position
        for pos, pause in splitTrajectory(
                self._pos_list,
                self.number_of_steps.magnitude):

            yield from self._movable.moveto(pos)

            unexpected = (State.ERROR, State.OFF, State.UNKNOWN)

            yield from waitUntil(
                lambda: self._movable.state != State.MOVING
                and np.linalg.norm(
                    np.subtract(self._movable.position,
                                pos)) < self.position_epsilon.magnitude**2
                or self._movable.state in unexpected)

            if self._movable.state in unexpected:
                type(self)._error("Unexpected state during scan: {}"
                                  .format(self._movable.state))

            if pause:
                if self.steps or self.stepNum == 0:
                    yield from self._sensible.acquire()

                if self.steps:
                    print("Step {} - Motors at {}"
                          .format(self.stepNum.magnitude,
                                  self._movable.position))
                    print("  Acquiring for {}"
                          .format(self.exposureTime + self.time_epsilon))

                    yield from sleep(self.exposureTime + self.time_epsilon)
                    yield from self._sensible.stop()

                self.stepNum += 1

        # Stop acquisition here for continuous scans
        if self._sensible.state == State.ACQUIRING:
            yield from self._sensible.stop()

        print("-"*linelen)


class AMesh(AScan):
    """Absolute Mesh scan"""
    def __init__(self, movable, pos_list1, pos_list2, sensible, exposureTime,
                 steps=True, number_of_steps=0, **kwargs):
        super().__init__(movable, meshTrajectory(pos_list1, pos_list2),
                         sensible, exposureTime, steps,
                         number_of_steps, **kwargs)


class APathScan(AScan):
    """Absolute Mesh scan"""
    def __init__(self, movable, pos_list, sensible, exposureTime, **kwargs):
        super().__init__(movable, pos_list, sensible,
                         exposureTime, True, 0, **kwargs)


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
    sensibleId = String(
        displayedName="SensibleId",
        accessMode=AccessMode.READONLY)

    exposureTime = Float(
        displayedName="Exposure time",
        defaultValue=0.1,
        unitSymbol=Unit.SECOND,
        accessMode=AccessMode.INITONLY)

    duration = Float(
        displayedName="Duration",
        defaultValue=1,
        unitSymbol=Unit.SECOND,
        accessMode=AccessMode.INITONLY)

    _sensible = Sensible()

    def __init__(self, sensible, exposureTime, duration, **kwargs):
        super().__init__(kwargs)
        self._sensible = (sensible
                          if isinstance(sensible, Sensible)
                          else Sensible(sensible))
        self.exposureTime = exposureTime
        self.duration = duration

    @coroutine
    def onInitialization(self):
        """Overrides class Device's"""
        self.sensibleId = self._sensible.deviceId

    @coroutine
    def execute(self):
        """Body. Override UserMacro's"""

        # Prepare for acquisition
        self._sensible.exposureTime = self.exposureTime
        yield from self._sensible.prepare()

        linelen = 80
        print("\n"+"-"*linelen)
        # Iterate over positions
        elaps = self.time_epsilon * 0
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
                 steps=True, number_of_steps=0, **kwargs):
        super().__init__(movable, pos_list1, pos_list2, sensible, exposureTime,
                         steps, number_of_steps, **kwargs)

        # Convert position from relative to absolute
        current_pos = np.array(self._movable.position)
        pos_list = np.array(list(self._pos_list))
        self._pos_list = pos_list + current_pos


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
        self.movableId = self._movable.deviceId
        self._position = position

    @coroutine
    def execute(self):
        """Body. Override UserMacro's"""
        def __print_motor_position():
            linelen = 80
            print("\n"+"-"*linelen)
            print("Motors at {}".format(self._movable.position))
            print("-"*linelen)

        unexpected = (State.ERROR, State.OFF, State.UNKNOWN)

        __print_motor_position()
        yield from self._movable.prepare()
        yield from self._movable.moveto(self._position)
        yield from waitUntil(
            lambda: self._movable.state != State.MOVING
            and np.linalg.norm(
                np.subtract(self._movable.position,
                            self._position))
            < self.position_epsilon.magnitude**2
            or self._movable.state in unexpected)

        if self._movable.state in unexpected:
            type(self)._error("Unexpected state after move: {}"
                              .format(self._movable.state))
        __print_motor_position()


class DMove(AMove):
    """Delta move"""
    def __init__(self, movable, position, **kwargs):
        super().__init__(movable, position, **kwargs)

        # Convert position from relative to absolute
        self._position = np.array(self._movable.position) + np.array(position)
