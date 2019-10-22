import datetime

import numpy as np

from karabo.native import Hash, Timestamp


def build_historic_data_float():
    """build an array of historic data with time stamps
    """
    # 10 random values in between [0, 100]
    vals = np.random.rand(10) * 100.

    # use now as base, add random values in minutes to the past
    now = datetime.datetime.now()
    timefmt = r"%Y-%m-%d %H:%M:%S"

    def time_rewind(nmin):
        dt = now + datetime.timedelta(minutes=-nmin)
        return Timestamp(dt.strftime(timefmt)).toDict()

    tss = map(time_rewind, reversed(sorted(vals)))
    ret = []
    for vv, tt in zip(vals, tss):
        h = Hash('v', vv)
        for attk, attv in tt.items():
            h.setAttribute('v', attk, attv)
        ret.append(h)
    return ret


def build_historic_data_string():
    """build an array of historic data state string with time stamps
    """
    vals = ['ON', 'OFF', 'ACQUIRING', 'STOPPED', 'DISABLED', 'RUNNING',
            'CHANGING', 'ERROR', 'UNKNOWN', 'STARTED', 'MOVING']

    now = datetime.datetime.now()
    timefmt = r"%Y-%m-%d %H:%M:%S"

    ret = []
    for vv in vals:
        dt = now + datetime.timedelta(minutes=-1)
        tt = Timestamp(dt.strftime(timefmt)).toDict()
        h = Hash('v', vv)
        for attk, attv in tt.items():
            h.setAttribute('v', attk, attv)
        ret.append(h)
    return ret
