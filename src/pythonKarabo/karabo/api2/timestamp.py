
from datetime import datetime

import dateutil.parser
import dateutil.tz

import time


attosec = 10 ** 18  # current Karabo resolution is attoseconds


class Timestamp(object):
    def __init__(self, date=None):
        """create a new timestamp

        date is either another timestamp (then we copy), None
        (we return now) or a string that will be parsed with
        dateutil"""
        self.tid = 0
        if date is None:
            self.time = int(time.time() * attosec)
        elif isinstance(date, Timestamp):
            self.time = date.time
        else:
            d = dateutil.parser.parse(date)
            if d.tzinfo is None:
                d = d.replace(tzinfo=dateutil.tz.tzlocal())
            self.time = int(d.timestamp() * attosec)

    @staticmethod
    def fromHashAttributes(attrs):
        if 'sec' not in attrs:
            return None
        ret = Timestamp()
        ret.time = attrs['frac'] + attrs['sec'] * attosec
        ret.tid = attrs['tid']
        return ret

    def toTimestamp(self):
        return self.time / 10 ** 18

    def toLocal(self):
        return datetime.fromtimestamp(self.toTimestamp()).isoformat()
