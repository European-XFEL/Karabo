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
import numpy as np

from .data import Timestamp


class TimeMixin:
    """Mediator class for TrainId calculation used for KaraboValues

    This class is updated via the slotTimeTick in any middlelayer device
    server.
    """
    # The tick trainId as an unsigned long long (Uint64)
    _tid = np.uint64(0)
    # time since epoch in seconds
    _time_sec = np.uint64(0)
    # the fractional seconds
    _time_frac = np.uint64(0)
    # the period between consecutive trains in microsec
    _period = np.uint64(0)

    @classmethod
    def set_reference(cls, train_id, sec, frac, period):
        """Update the TimeMixin via the TimeServer

        :param train_id: Propagated train Id
        :param sec: time since epoch in seconds
        :param frac: remaining time in attoseconds
        :param period: update interval between train Ids in microsec
        """
        if not period:
            # Protect against bad periods from the timeserver
            return
        cls._tid = np.uint64(train_id)
        cls._time_sec = np.uint64(sec)
        cls._time_frac = np.uint64(frac)
        cls._period = np.uint64(period)

    @classmethod
    def get_timestamp(cls, timestamp):
        """Correlated timestamp with latest reference to obtain trainId
        """
        if not cls._tid:
            return timestamp

        # create reference timestamp from timeserver information!
        attrs = {}
        attrs['tid'] = cls._tid
        attrs['sec'] = cls._time_sec
        attrs['frac'] = cls._time_frac
        reference = Timestamp.fromHashAttributes(attrs)

        # calculate how many trains are between reference and new timestamp
        since_id = cls.elapsed_tid(reference, timestamp)

        trainId = np.uint64(cls._tid + since_id)
        timestamp.tid = trainId

        return timestamp

    @classmethod
    def elapsed_tid(cls, reference, new):
        """Calculate the elapsed trainId between reference and newest timestamp

        :param reference: the reference timestamp
        :param new: the new timestamp

        :type reference: Timestamp
        :type new: Timestamp

        :returns: elapsed trainId's between reference and new timestamp
        """
        time_difference = new.toTimestamp() - reference.toTimestamp()
        return np.int64(time_difference * 1.0e6 // cls._period)

    @classmethod
    def toDict(cls):
        """Return the reference information as dictionary"""
        attrs = {}
        attrs['tid'] = cls._tid
        attrs['sec'] = cls._time_sec
        attrs['frac'] = cls._time_frac

        return attrs


def get_timestamp(timestamp=None):
    """Global function to return a Karabo Timestamp

    This function will return a Timestamp with updated train information
    from the TimeMixin depending on whether we are connected to
    the timeserver or not.
    """
    timestamp = timestamp or Timestamp()
    return TimeMixin.get_timestamp(timestamp)
