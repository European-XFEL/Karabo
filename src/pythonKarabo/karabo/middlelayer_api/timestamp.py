
from datetime import datetime
from functools import total_ordering

import dateutil.parser
import dateutil.tz
import numpy

import time


RESOLUTION = 10 ** 18  # current Karabo resolution is attoseconds


@total_ordering
class Timestamp(object):
    def __init__(self, date=None):
        """create a new timestamp

        date is either another timestamp (then we copy), None
        (we return now) or a string that will be parsed with
        dateutil"""
        self.tid = 0
        if date is None:
            self.time = int(time.time() * RESOLUTION)
        elif isinstance(date, Timestamp):
            self.time = date.time
        else:
            d = dateutil.parser.parse(date)
            if d.tzinfo is None:
                d = d.replace(tzinfo=dateutil.tz.tzlocal())
            self.time = int(d.timestamp() * RESOLUTION)

    @staticmethod
    def fromHashAttributes(attrs):
        if 'sec' not in attrs:
            return None
        ret = Timestamp()
        ret.time = attrs['frac'] + attrs['sec'] * RESOLUTION
        ret.tid = attrs['tid']
        return ret

    def toDict(self):
        return {"frac": numpy.uint64(self.time % RESOLUTION),
                "sec": numpy.uint64(self.time // RESOLUTION),
                "tid": self.tid}

    def toTimestamp(self):
        """Return the time as seconds since 1970-01-01 00:00 UTC"""
        return self.time / 10 ** 18

    def toLocal(self):
        """Return the time as an ISO 8601 string in the local timezone"""
        return datetime.fromtimestamp(self.toTimestamp()).isoformat()

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
        ts = datetime.utcfromtimestamp(self.toTimestamp())
        return ts.isoformat() + " UTC"
