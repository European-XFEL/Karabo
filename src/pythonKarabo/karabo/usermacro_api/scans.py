#!/usr/bin/env python3
"""Scantool macros"""
from ast import literal_eval
from asyncio import coroutine
import itertools
import ntpath
import numpy as np
import os
import sys

from karabo.middlelayer import (
    AccessMode, Bool, Float, Int32, InputChannel, State, String,
    sleep, UInt32, Unit, waitUntil)
from karabo.usermacro_api.genericproxy import Movable, Sensible
from karabo.usermacro_api.usermacro import UserMacro
from karabo.usermacro_api.util import flatten
from karabo.usermacro_api.generalized import *
from karabo.usermacros import AcquiredOffline, AcquiredOnline


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
        itpos1, itpos2 = itertools.tee(iter(pos_list))
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

    boundMovables = VectorString(
        displayedName="BoundMovables",
        accessMode=AccessMode.READONLY)

    boundSensibles = VectorString(
        displayedName="BoundSensibles",
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

    data = AcquiredOnline()

    _movable = Movable()
    _sensible = Sensible()
    _pos_list = []

    def __init__(self, movable, pos_list, sensible, exposureTime,
                 steps=True, number_of_steps=0, **kwargs):
        super().__init__(**kwargs)

        if isinstance(movable, Movable):
            self._movable = movable
        else:
            try:
                movable = literal_eval(movable)
                self._movable = Movable(*movable)
            except ValueError:
                self._movable = Movable(movable)

        if isinstance(sensible, Sensible):
            self._sensible = sensible
        else:
            try:
                sensible = literal_eval(sensible)
                self._sensible = Sensible(*sensible)
            except ValueError:
                self._sensible = Sensible(sensible)

        self._pos_list = (
            literal_eval(pos_list)
            if isinstance(pos_list, str) else pos_list)
        self.exposureTime = float(exposureTime)
        self.steps = bool(steps)
        self.number_of_steps = int(number_of_steps)
        if self.experimentId == "":
            self.experimentId = self.deviceId
        self.movableId = self._movable.deviceId
        self.boundMovables = flatten(self._movable.getBoundDevices())
        self.sensibleId = self._sensible.deviceId
        self.boundSensibles = flatten(self._sensible.getBoundDevices())
        self.pos_list = self._pos_list

        self.data = AcquiredOnline(self.experimentId,
                                   self._update.connectedOutputChannels)

    def __repr__(self):
        rep = "{cls}('{mov}', {pos}, '{sens}', {exp}, ".format(
            cls=type(self).__name__,
            mov=self._movable.deviceId,
            pos=self._pos_list,
            sens=self._sensible.deviceId,
            exp=str(self.exposureTime).split()[0])

        rep += "steps={steps}, number_of_steps={num})".format(
            steps=self.steps,
            num=str(self.number_of_steps).split()[0])

        return rep

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

        step_num = 0
        # Iterate over position
        for pos, pause in splitTrajectory(
                self._pos_list,
                self.number_of_steps.magnitude):

            if self.cancelled:
                break

            yield from self._movable.moveto(pos)

            expected = (State.MOVING, State.ON)

            yield from waitUntil(
                lambda: self._movable.state != State.MOVING
                and np.linalg.norm(
                    np.subtract(self._movable.position,
                                pos)) < self.position_epsilon.magnitude**2
                or self._movable.state not in expected)

            if self._movable.state != State.ON:
                type(self)._error("Unexpected state during scan: {}"
                                  .format(self._movable.state))

            if pause:
                if self.steps or step_num == 0:
                    self.stepNum = step_num
                    self.update()
                    yield from self._sensible.acquire()

                if self.steps:
                    print("Step {} - Motors at {}"
                          .format(self.stepNum.magnitude,
                                  self._movable.position))
                    print("  Acquiring for {}"
                          .format(self.exposureTime + self.time_epsilon))

                    yield from sleep(self.exposureTime + self.time_epsilon)
                    yield from self._sensible.stop()

                step_num += 1

        # Stop acquisition here for continuous scans
        if self._sensible.state == State.ACQUIRING:
            yield from self._sensible.stop()

        print("-"*linelen)

        return AcquiredOffline(self.experimentId)

    @InputChannel(displayedName="Online data source")
    @coroutine
    def _update(self, meta, data):
        self.data.append(meta, data)


class AMesh(AScan):
    """Absolute Mesh scan"""
    def __init__(self, movable, pos_list1, pos_list2, sensible, exposureTime,
                 steps=True, number_of_steps=0, **kwargs):
        pos_list1 = (
            literal_eval(pos_list1)
            if isinstance(pos_list1, str) else pos_list1)
        pos_list2 = (
            literal_eval(pos_list2)
            if isinstance(pos_list2, str) else pos_list2)
        super().__init__(movable, meshTrajectory(pos_list1, pos_list2),
                         sensible, exposureTime, steps,
                         number_of_steps, **kwargs)
        self._pos_list1 = pos_list1
        self._pos_list2 = pos_list2

    def __repr__(self):
        # By default, meshTrajectory has an ugly representation.
        # This may break on non-64bits machines.

        rep = super().__repr__()
        rep = "{begin}{list1}, {list2}{end}".format(begin=rep[:13],
                                                    list1=self._pos_list1,
                                                    list2=self._pos_list2,
                                                    end=rep[64:])
        return rep


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
        # Only used for representation
        self._raw_pos_list = pos_list

        # Convert position from relative to absolute
        self._pos_list = np.array(
            self._movable.position) + np.array(self._pos_list)

    def __repr__(self):
        """ np.arrays are pretty printed, and have new lines in them,
            with user input, before the positions delta calculated
        """
        rep = super().__repr__().split(',')
        rep = rep[0] + ", {},".format(self._raw_pos_list) + ",".join(rep[2:])
        return rep

    def __str__(self):
        """ This outputs the position list with the values already delta """
        return super().__repr__().replace('\n', ',')


class TScan(UserMacro):
    """Time scan"""
    sensibleId = String(
        displayedName="SensibleId",
        accessMode=AccessMode.READONLY)

    boundSensibles = VectorString(
        displayedName="BoundSensibles",
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
        super().__init__(**kwargs)

        if isinstance(sensible, Sensible):
            self._sensible = sensible
        else:
            try:
                sensible = literal_eval(sensible)
                self._sensible = Sensible(*sensible)
            except ValueError:
                self._sensible = Sensible(sensible)

        self.exposureTime = float(exposureTime)
        self.duration = float(duration)
        self.sensibleId = self._sensible.deviceId
        self.boundSensibles = flatten(self._sensible.getBoundDevices())

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
            if self.cancelled:
                break
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
    movableId = String(
        displayedName="Movable",
        accessMode=AccessMode.READONLY)

    boundMovables = VectorString(
        displayedName="BoundMovables",
        accessMode=AccessMode.READONLY)

    _movable = Movable()
    _position = []

    def __init__(self, movable, position, **kwargs):
        super().__init__(**kwargs)

        if isinstance(movable, Movable):
            self._movable = movable
        else:
            try:
                movable = literal_eval(movable)
                self._movable = Movable(*movable)
            except ValueError:
                self._movable = Movable(movable)

        self.movableId = self._movable.deviceId
        self.boundMovables = flatten(self._movable.getBoundDevices())

        self._position = (
            literal_eval(position)
            if isinstance(position, str)
            else position)

    @coroutine
    def execute(self):
        """Body. Override UserMacro's"""
        def __print_motor_position():
            linelen = 80
            print("\n"+"-"*linelen)
            print("Motors at {}".format(self._movable.position))
            print("-"*linelen)

        expected = (State.MOVING, State.ON)

        __print_motor_position()
        yield from self._movable.prepare()
        yield from self._movable.moveto(self._position)
        yield from waitUntil(
            lambda: self._movable.state != State.MOVING
            and np.linalg.norm(
                np.subtract(self._movable.position,
                            self._position))
            < self.position_epsilon.magnitude**2
            or self._movable.state not in expected)

        if self._movable.state != State.ON:
            type(self)._error("Unexpected state after move: {}"
                              .format(self._movable.state))
        __print_motor_position()


class DMove(AMove):
    """Delta move"""
    def __init__(self, movable, position, **kwargs):
        super().__init__(movable, position, **kwargs)

        # Convert position from relative to absolute
        self._position = np.array(
            self._movable.position) + np.array(self._position)


if __name__ == "__main__":
    cls = eval(ntpath.basename(sys.argv[0]))
    cls.main()
    os._exit(0)
