# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import time
from datetime import datetime, timezone
from functools import total_ordering

import dateutil.parser
import dateutil.tz
import numpy

RESOLUTION = 10 ** 18  # current Karabo resolution is attoseconds
MINUTE_IN_SECONDS = 60
HOUR_IN_SECONDS = MINUTE_IN_SECONDS * 60
DAY_IN_SECONDS = HOUR_IN_SECONDS * 24


@total_ordering
class Timestamp:
    """This is a time stamp

    :param date: is either another timestamp (then we copy), None
                 (we return now), a float (from time.time()), an integer
                 (raw timestamp) or a string that will be parsed
                 with :mod:`dateutil`
    """

    # Reduce the memory needed for this _very_ common object
    __slots__ = ['time', '_tid']

    def __init__(self, date=None):
        self._tid = numpy.uint64(0)
        if date is None:
            self.time = int(time.time() * RESOLUTION)
        elif isinstance(date, Timestamp):
            self.time = date.time
        elif isinstance(date, float):
            self.time = int(date * RESOLUTION)
        elif isinstance(date, int):
            self.time = date
        else:
            d = dateutil.parser.parse(date)
            if d.tzinfo is None:
                d = d.replace(tzinfo=dateutil.tz.tzlocal())
            self.time = int(d.timestamp() * RESOLUTION)

    @classmethod
    def fromHashAttributes(cls, attrs):
        if 'sec' not in attrs:
            return None
        ret = cls.__new__(cls)
        # Avoid overflows, cast back to integer
        ret.time = int(attrs['frac']) + int(attrs['sec']) * RESOLUTION
        ret.tid = attrs['tid']
        return ret

    def toHashAttributes(self, hash_):
        time_properties = self.toDict()
        for entry in hash_:
            for k, v in time_properties.items():
                hash_.setAttribute(entry, k, v)

    @property
    def time_frac(self):
        """The fractional seconds of the timestamp in attoseconds
        """
        return numpy.uint64(self.time % RESOLUTION)

    @property
    def time_sec(self):
        """The seconds of the timestamp from epoch
        """
        return numpy.uint64(self.time // RESOLUTION)

    @property
    def tid(self):
        """The train Id associated with this Timestamp
        """
        return self._tid

    @tid.setter
    def tid(self, value):
        self._tid = numpy.uint64(value)

    def toDict(self):
        return {"frac": self.time_frac,
                "sec": self.time_sec,
                "tid": self.tid}

    def toTimestamp(self):
        """Return the time as seconds since 1970-01-01 00:00 UTC"""
        return self.time / RESOLUTION

    def toLocal(self, sep="T", timespec="auto"):
        """
        Return the time as an ISO 8601 string in the local timezone

        :param sep: unicode character to separate date and time, default is 'T'
        :param timespec: specifies the number of additional components
                         of the time to include, defaults to 'auto'.
                         Further options are 'hours', 'minutes', 'seconds',
                         'milliseconds' and 'microseconds'
        """
        return datetime.fromtimestamp(self.toTimestamp()).isoformat(
            sep, timespec=timespec)

    def __eq__(self, other):
        if not isinstance(other, Timestamp):
            return False
        return self.time == other.time

    def __lt__(self, other):
        if not isinstance(other, Timestamp):
            return NotImplemented
        return self.time < other.time

    def __str__(self):
        """See toLocal()"""
        return self.toLocal()

    def __repr__(self):
        """Return the time as an ISO 8601 string in UTC"""
        ts = datetime.fromtimestamp(
            self.toTimestamp(), tz=timezone.utc).replace(tzinfo=None)
        return ts.isoformat() + " UTC"

    def __add__(self, other):
        """Add operator for the time in seconds since 1970-01-01 00:00 UTC"""
        if isinstance(other, Timestamp):
            return self.toTimestamp() + other.toTimestamp()
        elif isinstance(other, (float, int)):
            return self.toTimestamp() + other

        return NotImplemented

    def __sub__(self, other):
        """Sub operator for the time in seconds since 1970-01-01 00:00 UTC"""
        if isinstance(other, Timestamp):
            return self.toTimestamp() - other.toTimestamp()
        elif isinstance(other, (float, int)):
            return self.toTimestamp() - other

        return NotImplemented


def minutesAgo(n=1):
    """Return a timestamp from `n` minutes ago

    :param n: minutes, defaults to `1`
    """
    return Timestamp(Timestamp() - (MINUTE_IN_SECONDS * n))


def hoursAgo(n=1):
    """Return a timestamp from `n` hours ago

    :param n: hours, defaults to `1`
    """
    return Timestamp(Timestamp() - (HOUR_IN_SECONDS * n))


def daysAgo(n=1):
    """Return a timestamp from `n` days ago

    :param n: days, defaults to `1`
    """
    return Timestamp(Timestamp() - (DAY_IN_SECONDS * n))
