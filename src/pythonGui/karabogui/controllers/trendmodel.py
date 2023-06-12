# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import datetime

import numpy
from pyqtgraph import PlotDataItem
from qtpy.QtCore import QDateTime
from traits.api import (
    Array, Constant, Float, HasStrictTraits, Instance, Int, List, WeakRef,
    on_trait_change)

from karabo.native import Timestamp
from karabogui.binding.api import PropertyProxy
from karabogui.graph.common.const import ALARM_INTEGER_MAP, STATE_INTEGER_MAP

ONE_WEEK = "One Week"
ONE_DAY = "One Day"
ONE_HOUR = "One Hour"
TEN_MINUTES = "Ten Minutes"
UPTIME = "Uptime"
HIDDEN = "Hidden"


def get_start_end_date_time(selected_time_span):
    """ Return beginning and end date time for given ``selected_time_span``.
        If the ``selected_time_span`` is not supported ``None`` is returned.
    """
    current_date_time = QDateTime.currentDateTime()
    if selected_time_span == ONE_WEEK:
        # One week
        start_date_time = current_date_time.addDays(-7)
    elif selected_time_span == ONE_DAY:
        # One day
        start_date_time = current_date_time.addDays(-1)
    elif selected_time_span == ONE_HOUR:
        # One hour
        start_date_time = current_date_time.addSecs(-3600)
    elif selected_time_span == TEN_MINUTES:
        # Ten minutes
        start_date_time = current_date_time.addSecs(-600)
    else:
        return None, None

    return start_date_time, current_date_time


class _Generation:
    """ This holds a single generation of a Curve's data.
    """
    size = 200
    base = 10  # number of points to combine per generation

    def __init__(self):
        self.fill = 0
        self.xs = numpy.empty(self.size, dtype=float)
        self.ys = numpy.empty(self.size, dtype=float)

    def add_point(self, x, y):
        self.xs[self.fill] = x
        self.ys[self.fill] = y
        self.fill += 1

        if self.fill == self.size:
            return self.reduce_data()
        return None

    def reduce_data(self):
        x = self.xs[:self.base].mean()
        y = self.ys[:self.base].mean()
        self.xs[:-self.base] = self.xs[self.base:]
        self.ys[:-self.base] = self.ys[self.base:]
        self.fill -= self.base
        return x, y


class Curve(HasStrictTraits):
    """This holds the data for one curve

    The currently to be shown data is in self.x and self.y.
    Up to self.histsize it is filled with historical data, up to self.fill
    it is then filled with "current" data, meaning data that has been
    accumulated from the changes coming in.

    There is a second data structure, self.generations, which is a list of
    _Generation objects containing averaged data, always `base` points are
    averaged over and put into the next higher aggregated storage.

    Once the basic storage in self.x and self.y flows over, it gets replaced
    by the averaged data in self.generations.
    """
    curve = WeakRef(PlotDataItem)
    proxy = Instance(PropertyProxy)
    generations = List(Instance(_Generation))

    histsize = Int(0)
    fill = Int(0)

    x = Array(dtype=float)
    y = Array(dtype=float)

    t0 = Float(0)
    t1 = Float(0)

    spare = Constant(100)
    # Limits amount of data from past
    maxHistory = Constant(500)
    sparsesize = Constant(400)
    # minimum number of points shown (if possible)
    minHistory = Constant(100)

    # Defines which curve we are using. Curve type 0 is classic, 1 is state
    # and 2 is alarms
    curve_type = Int(0)

    def _generations_default(self):
        return [_Generation() for _ in range(4)]

    def _x_default(self):
        arraysize = self.spare + sum([g.size for g in self.generations], 0)
        return numpy.empty(arraysize, dtype=float)

    def _y_default(self):
        arraysize = self.spare + sum([g.size for g in self.generations], 0)
        return numpy.empty(arraysize, dtype=float)

    def purge(self):
        """Purge the curve by resetting all values to their default"""
        self.generations = self._generations_default()
        self.x = self._x_default()
        self.y = self._y_default()
        self.histsize = 0
        self.fill = 0
        self.update()

    def add_point(self, value, timestamp):
        # Fill the generations data, possibly propagating averaged values
        point = (timestamp, value)
        for gen in reversed(self.generations):
            point = gen.add_point(*point)
            if point is None:
                break

        # Fill the main data buffer
        self.x[self.fill] = timestamp
        self.y[self.fill] = value
        self.fill += 1
        # When the main buffer fills up, copy the generations data
        if self.fill == len(self.x):
            self.fill_current()
        self.update()

    def fill_current(self):
        pos = self.histsize
        for gen in self.generations:
            fill = gen.fill
            if fill == 0:
                continue
            self.x[pos:pos + fill] = gen.xs[:fill]
            self.y[pos:pos + fill] = gen.ys[:fill]
            pos += fill
        self.fill = pos

    def get_property_history(self, t0, t1):
        # Avoid if not currently visible
        if not self.proxy.visible:
            return
        t0 = str(datetime.datetime.utcfromtimestamp(t0).isoformat())
        t1 = str(datetime.datetime.utcfromtimestamp(t1).isoformat())
        self.proxy.get_history(t0, t1, max_value_count=self.maxHistory)

    def changeInterval(self, t0, t1, force=False):
        """Change the time interval of this Curve

        :param t0: new start time of the curve
        :param t1: new end time of the curve
        """
        p0 = self.x[:self.fill].searchsorted(t0)
        p1 = self.x[:self.fill].searchsorted(t1)

        not_enough_data = (p1 - p0) < self.minHistory
        no_data = self.histsize == 0
        zoomed_out = self.histsize > self.sparsesize
        nearly_left_border = 0.9 * self.t0 + 0.1 * self.t1
        nearly_right_border = 0.1 * self.t0 + 0.9 * self.t1

        # Request history only needs to be done under certain circumstances
        if (force or no_data or (not_enough_data and zoomed_out) or
                (self.x[p0] > nearly_left_border) or
                (p1 < self.histsize and self.x[p1 - 1] < nearly_right_border)):
            self.get_property_history(t0, t1)

        self.t0 = t0
        self.t1 = t1

    def update(self):
        """ Show the new data on screen"""
        if self.curve:
            self.curve.setData(self.x[:self.fill], self.y[:self.fill])

    @on_trait_change("proxy:visible")
    def _visibility_update(self, visible):
        if visible and self.t1 >= self.x[self.histsize]:
            self.get_property_history(self.t0, self.t1)

    @on_trait_change("proxy:binding:historic_data")
    def _historic_data_arrival(self, data):
        if not data:
            return

        if self.t1 == self.t0:
            # Prevent division by 0, otherwise self.curve.plot() is None
            return

        datasize = len(data)
        gensize = sum([g.size for g in self.generations], 0)
        arraysize = datasize + gensize + self.spare
        x = numpy.empty(arraysize, dtype=float)
        y = numpy.empty(arraysize, dtype=float)

        for i, d in enumerate(data):
            # Protect against inf by setting numpy.NaN which is not shown
            x[i] = Timestamp.fromHashAttributes(d["v", ...]).toTimestamp()
            # Do some gymnastics for crazy values!
            value = d["v"]
            if self.curve_type == 1:
                value = STATE_INTEGER_MAP[value]
            elif self.curve_type == 2:
                value = ALARM_INTEGER_MAP[value]
            y[i] = value

        p0 = self.x[:self.fill].searchsorted(self.t0)
        p1 = self.x[:self.fill].searchsorted(self.t1)
        np0 = x[:datasize].searchsorted(self.t0)
        np1 = x[:datasize].searchsorted(self.t1)

        span = (self.x[p1 - 1] - self.x[p0]) / (self.t1 - self.t0)
        nspan = (x[np1 - 1] - x[np0]) / (self.t1 - self.t0)

        if (np1 - np0 < p1 - p0) and not nspan > span < 0.9:
            return

        # If the history overlaps generation data, favor the history data.
        end = x[datasize - 1]
        for gen in self.generations:
            fill = gen.fill
            if fill == 0:
                continue
            pos = gen.xs[:fill].searchsorted(end)
            if pos == 0:
                break
            gen.xs[:fill - pos] = gen.xs[pos:fill]
            gen.ys[:fill - pos] = gen.ys[pos:fill]
            gen.fill -= pos

        self.histsize = datasize
        self.x = x
        self.y = y
        self.fill_current()
        self.update()

    def get_max_timepoint(self):
        """ Return the max time point for the fill values"""
        return numpy.max(self.x[:self.fill]) if self.fill else 0.0
