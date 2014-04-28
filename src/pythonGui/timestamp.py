from __future__ import division

from datetime import datetime


class Timestamp(object):
    @staticmethod
    def fromHashAttributes(attrs):
        ret = Timestamp()
        ret.time = attrs['frac'] + attrs['sec'] * 10 ** 18
        ret.tid = attrs['tid']
        return ret

    def toTimestamp(self):
        return self.time / 10 ** 18


    def toLocal(self):
        return datetime.fromtimestamp(self.toTimestamp()).isoformat()
