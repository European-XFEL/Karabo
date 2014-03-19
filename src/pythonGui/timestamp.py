from __future__ import division


class Timestamp(object):
    @staticmethod
    def fromHashAttributes(attrs):
        ret = Timestamp()
        ret.time = attrs['frac'] + attrs['sec'] * 10 ** 18
        return ret

    def toTimestamp(self):
        return self.time / 10 ** 18