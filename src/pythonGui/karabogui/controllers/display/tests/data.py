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

import numpy as np

from karabo.native import Hash, Timestamp


def build_historic_data_float():
    """build an array of historic data with time stamps"""
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


def _build_value_string(vals):
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


def build_historic_state_string():
    """build an array of historic data state string with time stamps"""
    vals = ['ON', 'OFF', 'ACQUIRING', 'STOPPED', 'DISABLED', 'RUNNING',
            'CHANGING', 'ERROR', 'UNKNOWN', 'STARTED', 'MOVING']
    return _build_value_string(vals)


def build_historic_alarm_string():
    """build an array of historic data alarm string with time stamps"""
    vals = ['none', 'warn', 'warnLow', 'warnHigh', 'warnVarianceLow',
            'warnVarianceHigh', 'alarm', 'alarmLow', 'alarmHigh',
            'alarmVarianceLow', 'alarmVarianceHigh', 'interlock']
    return _build_value_string(vals)
