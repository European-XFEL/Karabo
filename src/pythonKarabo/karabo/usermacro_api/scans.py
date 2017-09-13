#!/usr/bin/env python3

"""Scan macros

Scan macros are meant to be run:
    either interactively in ikarabo or the GUI console,
    or from a User Macro script.

The general syntax is:

data = AScan|DScan(movable, positions, sensible, exposure_time,
                   steps=True, number_of_steps=0)

data = AMesh|DMesh(movable, positions_on_axis1, positions_on_axis2, sensible,
                   exposure_time, steps=True, number_of_steps=0)

data = TScan(sensible, exposure_time, duration)

Before setting up a scan you need to define at least:
 - a movable, which is the set of devices that will be controlled
   by the scan (e.g. motors, slits),
   Example of movable definition:
   my_movable = Movable('motorId1', 'motorId2', 'motorId3')

   An extended syntax, using case insensitive regular expressions
   (https://docs.python.org/2/library/re.html), allows to specify
   the parameters to be fetched from logs at the end of the scan.

   Example of movable definition with parameters-of-interest:
   my_movable = Movable('motorId1@(encoderPosition|targetPosition)',
                        'motorId2@.*position$', 'motorId3')
   Observe that the parameter specification follows a '@' sign. Also note that
   the second parameter specification *.position$ extends the first
   specification (encoderPosition|targetPosition) in matching
   stepCounterPosition and saveLimitPosition as well for Beckhoff
   simple motors.

 - a sensible (measurable), which is the set of devices to acquire scan data
   from (e.g. detectors, cameras, image processors, spectrometers, ADC
   or any sensor).
   Example of sensible (measurable) definition:
   my_sensible = Sensible('aCameraId1, 'anotherCameraId1', 'aSensorId',
                          'anImageProcessorId')

   An extended syntax, using case insensitive regular expressions allows
   here also, specifying the set of parameters to be displayed
   in the console as the scan goes, and to be fetched from the logs
   when the scan ends.
   Example of sensible (measurable) definition with parameters-of-interest:
   my_sensible = Sensible('aCameraId1@frameRate, 'anotherCameraId1@frameCount',
                          'aSensorId',
                          'anImageProcessorId@(.*PxValues|[xy]0$)')
   Here (.*PxValues|[xy]0$) means select Mean, Min and Max Pixels values
   from the image processor
   as well as the centers of mass (i.e. x0 and y0).

 - a list of positions for the movable, wich contains the coordinates of the
   positions that define movable trajectory (i.e. a list of tuples where the
   number of coordinates must be equal to the number of devices the movable is
   made up of)
   Example:
   my_positions = [(0, 1, 1), (1, 2, 2), (4, 6, 6.5), (7, 10, 9.5)]

 - the exposure time for sensibles. The movable will always pause between
   steps for at least this duration.
   e.g. my_exposure_time=0.1

To run an absolute scan you can now execute:

    data = AScan(my_movable, my_positions, my_sensible, my_exposure_time)()

This form will also work:

    data = AScan(Movable('motorId1', 'motorId2', 'motorId3'),
                    [(0, 1, 1), (1, 2, 2)],
                    Sensible('aCameraId1', 'anImageProcessorId'),
                    0.1)()

Optional arguments are:
- a steps boolean: True by default and impying step-wise scans or False
  for continuous scans

- the number_of_steps (0 by default) that defines the number
of steps the trajectory will be split in. If this number is 0, and steps=True,
the scan will pause at every point of the movable trajectory.

You can run a delta scan by calling:

    data = DScan( ... )()

which has the same properties of AScan but the position list is interpreted as
increments relative to current position of movables, and the movables will home
to their starting positions.

You can run an Absolute Mesh scan by calling:

    data = AMesh(movable, positions1, positions2, sensible, ...)()

The Delta Mesh scan is denoted DMesh.

A time scan (TScan) runs in a similar way, but does not use movables.
It only repeatedly acquires then stops a sensible (measurable) after a bit more
than the exposure time, during a given duration. It sensible
supports the extended syntax described above and an AcquiredData object is
also returned.

The helper macro AMove(movable, position) and DMove(movable, position)
might be useful to switch on and move a given movable in a single command.

The scan returns an AcquiredData object that can come from one of these
three sources: data loggers, online DAQ, offline DAQ.

Data Loggers:
-------------
The data object returned by a command line that uses the extended syntax
data = AScan(...@param1, ...@param2, ....)()
is ready for plotting. Just type

     plot(data)

You can also manually load the values of the properties you are interested in
into the acquired data fifo:

    data.query('motorId1.aMotorProperty',
                      'aCameraId1.aCameraProperty',
                      'anImageProcessorId.aProcessorProperty')

Then you can plot of data by calling the plot()
function:

    plotdata = plot(data)

It displays a simple default plot windows and returns data in a convenient
format for custom plots. For more details about this you can run:

    help(plot)

Online DAQ:
----------
Let us assume one instantiates an AScan without running it:

    ascan = AScan(Movable('motorId1', 'motorId2'),
                    [(0, 1), (1, 2)],
                    Sensible('aCameraId1', 'anImageProcessorId'),
                    0.1)
    ascan()

During a scan, you can obtain live, lossy data, from the DAQ system, by
querying the scan:

    print(ascan.data)
    datum = next(my_scan.data)
    datum.trainId

The data is stored in the form of hashes in a limited fifo, with the intention
to be used to check on your scan. To do more advanced analysis, use the
Offline DAQ data which is provided to you at the end of a scan.

Offline DAQ:
-----------
Once a scan is done, you will get in return an object that obtains scan data
from the datalogger, and another that queries the DAQ, granted that its
configuration could be retrieved.

The offline DAQ object accesses the data stored in the run's HDF5 files, via a
DataReader. This object is non-lossy, but only stores a limited amount of train
data in memory, as to not be overwhelming memory intensive. The object will
wait for an entry to be consumed (that is, popped from the fifo) before
receiving the next train data.

Use it as follows:

    log_data, daq_data = AScan(...)()
    for train_data in daq_data:
        image_frame = NDArray().toKaraboValue(train_data['data.image.data'])
        plt.imshow(image_frame[0, 0, :, :])

"""

from ast import literal_eval
from asyncio import coroutine
import ntpath
import numpy as np
import os
import sys

from karabo.usermacro_api.middlelayer import (
    AccessMode, Bool, Float, Int32, InputChannel, State, String,
    sleep, UInt32, Unit, waitUntil, VectorString)
from karabo.usermacro_api.genericproxy import Movable, Sensible
from karabo.usermacro_api.usermacro import UserMacro
from karabo.usermacro_api.util import (
    flatten, meshTrajectory, reformat, splitTrajectory)
from karabo.usermacro_api.dataobjects import (
    AcquiredFromLog, AcquiredOffline, AcquiredOnline)
# This is required for GenericProxy to
# find all its subclasses when scans are run standalone
# (from command line for example)
import karabo.usermacro_api.generalized  # noqa


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

    dataReader = String(
        displayedName="Offline data source",
        description="",
        defaultValue="datareader:output")

    daqDone = Bool(
        displayedName="DAQDone",
        defaultValue=False,
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
            except (SyntaxError, ValueError):
                self._movable = Movable(movable)

        if isinstance(sensible, Sensible):
            self._sensible = sensible
        else:
            try:
                sensible = literal_eval(sensible)
                self._sensible = Sensible(*sensible)
            except (SyntaxError, ValueError):
                self._sensible = Sensible(sensible)

        self._pos_list = (
            literal_eval(pos_list)
            if isinstance(pos_list, str) else pos_list)
        self.exposureTime = float(exposureTime)

        self.steps = (
            bool(steps)
            if isinstance(steps, bool)
            else literal_eval(steps))
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
                self.stepNum = step_num
                self.update()
                if self.steps or step_num == 0:
                    yield from self._sensible.acquire()

                if self.steps:
                    print("Step {} - Motors at {}"
                          .format(self.stepNum.magnitude,
                                  self._movable.position))
                    print("  Acquiring for {}"
                          .format(self.exposureTime + self.time_epsilon))

                    yield from waitUntil(
                        lambda: self._sensible.state != State.STOPPED)
                    yield from sleep(self.exposureTime + self.time_epsilon)

                if self.steps or step_num == 1:
                    yield from self._sensible.stop()
                    if self._sensible.value:
                        print("  Value: {}".format(
                            reformat(self._sensible.value)))

                step_num += 1
        # Add an extra step to upper bound last measurements time-wise
        self.stepNum += 1
        self.update()
        print("-"*linelen)

        if self.daqDone:
            return AcquiredOffline(self.experimentId,
                                   source=self.dataReader)
        else:
            attrs = (
                ["{}.stepNum".format(self.deviceId)]
                + [k for k in flatten(self._movable.getBoundParameters())]
                + [k for k in flatten(self._sensible.getBoundParameters())])
            data = AcquiredFromLog(self.deviceId, *attrs)
            yield from data.query()
            return data

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

    @coroutine
    def execute(self):
        """Overriden for moving Movable back to its initial position"""
        pos0 = self._movable.position
        data = yield from super().execute()
        yield from self._movable.moveto(pos0)
        print("Moving back to {}".format(pos0))
        return data


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

    stepNum = UInt32(
        displayedName="Current step",
        defaultValue=0,
        accessMode=AccessMode.READONLY)

    dataReader = String(
        displayedName="Offline data source",
        description="",
        defaultValue="datareader:output")

    daqDone = Bool(
        displayedName="DAQDone",
        defaultValue=False,
        accessMode=AccessMode.READONLY)

    data = AcquiredOnline()

    _sensible = Sensible()

    def __init__(self, sensible, exposureTime, duration, **kwargs):
        super().__init__(**kwargs)

        if isinstance(sensible, Sensible):
            self._sensible = sensible
        else:
            try:
                sensible = literal_eval(sensible)
                self._sensible = Sensible(*sensible)
            except (SyntaxError, ValueError):
                self._sensible = Sensible(sensible)

        self.exposureTime = float(exposureTime)
        self.duration = float(duration)
        self.sensibleId = self._sensible.deviceId
        self.boundSensibles = flatten(self._sensible.getBoundDevices())
        self.data = AcquiredOnline(source=self._update.connectedOutputChannels)

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
            print("Step {} - at time {}".format(i, elaps))
            self.stepNum = i
            self.update()
            yield from self._sensible.acquire()
            yield from waitUntil(
                lambda: self._sensible.state != State.STOPPED)
            yield from sleep(self.exposureTime + self.time_epsilon)
            yield from self._sensible.stop()

            if self._sensible.value:
                print("  Value: {}".format(
                    reformat(self._sensible.value)))

            elaps += self.exposureTime + self.time_epsilon

        # Add an extra step to upper bound last measurements time-wise
        self.stepNum += 1
        self.update()
        print("-"*linelen)

        if self.daqDone:
            return AcquiredOffline(self.deviceId,
                                   source=self.dataReader)
        else:
            attrs = (
                ["{}.stepNum".format(self.deviceId)]
                + [k for k in flatten(self._sensible.getBoundParameters())])
            data = AcquiredFromLog(self.deviceId, *attrs)
            data.query()
            return data

    @InputChannel(displayedName="Online data source")
    @coroutine
    def _update(self, meta, data):
        self.data.append(meta, data)


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

    @coroutine
    def execute(self):
        """Overriden for moving Movable back to its initial position"""
        pos0 = self._movable.position
        data = yield from super().execute()
        yield from self._movable.moveto(pos0)
        print("Moving back to {}".format(pos0))
        return data


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
            except (SyntaxError, ValueError):
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
