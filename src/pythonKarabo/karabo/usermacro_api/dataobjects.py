from collections import deque
import time
import heapq

# These are currently under another import, as they will be unecessary
# and removed once the actual query functions will be implemented
from karabo.middlelayer import (Hash, getHistory)

from .util import getConfigurationFromPast


class AcquiredData(object):
    """ Acquired Data is an iterable object that queries various
        data sources.
        It has a buffer of a specified `_max_fifo_size` size.
        It returns a k-hash::

            data = AcquiredData(experimentId=1)
            h = next(data)
            type(h) == Hash

        Although not mandatory, the hash is expected to contain these
        entries:
            - a timestamp
            - a trainId
            - value (which is a hash itself)
    """

    def __init__(self, experimentId=None, size=10):
        self.experimentId = experimentId
        self._max_fifo_size = size
        self._fifo = deque([], self._max_fifo_size)

    def __repr__(self):
        rep = "{cls}({exp}, size={size})".format(
              cls=type(self).__name__,
              exp=self.experimentId,
              size=self._max_fifo_size)
        return rep

    def __str__(self):
        exp = ("Experiment " + str(self.experimentId) if self.experimentId
               else "Unknown Experiment")
        srep = ','.join('{}'.format(dat) for dat in self._fifo)
        return exp + ': [' + srep + ']'

    def append(self, data):
        self._fifo.append(data)

    def __getitem__(self, index):
        if index >= len(self):
            raise IndexError("buffer index out of range")
        return self._fifo[index]

    def _fillUp(self):
            raise NotImplementedError

    def __iter__(self):
        return self

    def __next__(self):
        if len(self) == 0:
            raise StopIteration
        return self._fifo.popleft()

    def __len__(self):
        return len(self._fifo)


class AcquiredOnline(AcquiredData):

    def __init__(self, experimentId=None, channel=None, size=10):
        super().__init__(experimentId, size)
        self.channel = channel

    def __repr__(self):
        rep = super().__repr__()
        rep = rep[:-1] + ", channel={})".format(self.channel)
        return rep

    def flatten(self, h):
        """ Given a hash, it will unify it such that all hashes within
            it are at the same level.
        """
        out = Hash()
        for k in h.getKeys():
            if isinstance(h[k], Hash):
                out.update(self.flatten(h[k]))
            else:
                out[k] = h[k]
        return out

    def append(self, data, meta):
        """ This function is to be called by the owner within their
        @InputChannel
        """
        x = Hash([('data', data), ('meta', meta)])
        x = self.flatten(x)
        super().append(x)


class AcquiredOffline(AcquiredData):
    pass


class AcquiredFromLog(AcquiredData):
    """
    Child class to retrieve 'slow' data from datalogger history

    self.movableIds: list of bound devices IDs used in the scan as movables
    self.measurableIds: list of bound devices IDs used in the scan
      as measurables
    self.bound_devices_properties[deviceId] list of logged properties logged
      for the device
    """

    def __init__(self, experimentId=None, size=10):
        super().__init__(experimentId, size)

        # retrieve steps from scan history assuming there have been only one
        # scan with given deviceId from the big-bang up to now
        self.steps = getHistory("{}.stepNum".format(self.experimentId),
                                "2010-01-01T00:00:00",
                                time.strftime('%Y-%m-%dT%H:%M:%S'))
        # steps has the following format:
        # [(seconds_from_1970, train_id, is_last_of_set, value) ]

        # begin of the scan = timestamp of first step
        self.begin = time.strftime('%Y-%m-%dT%H:%M:%S',
                                   time.localtime(self.steps[0][0]))

        # end of the scan = timestamp of last step
        self.end = time.strftime('%Y-%m-%dT%H:%M:%S',
                                 time.localtime(
                                     self.steps[len(self.steps)-1][0]))

        # if for any reason end value is wrong, assume end is now
        if(self.begin >= self.end):
            self.end = time.strftime('%Y-%m-%dT%H:%M:%S', time.localtime())

        # get IDs of devices used by Scan:
        his = getHistory("{}.boundMovables".format(self.experimentId),
                         self.begin, self.end)
        self.movableIds = his[0][3]

        his = getHistory("{}.boundSensibles".format(self.experimentId),
                         self.begin, self.end)
        self.measurableIds = his[0][3]

        # get properties for each device
        self.bound_devices_properties = {}

        for m in self.movableIds + self.measurableIds:
            self.bound_devices_properties[m] = []
            h, s = getConfigurationFromPast(m, self.begin)
            properties = h.getKeys()
            for p in properties:
                self.bound_devices_properties[m].append(p)

    def queryData(self, *args):
        """
        :param: a list of property strings with the form 'deviceId.property'
        retrieved data are queued in self.data, sorted by timestamp
        """

        histories = []
        for prop in args:
            his = getHistory(prop, self.begin, self.end)

            # add property name to tuples
            his2 = []
            for h in his:
                h = h + (prop,)
                his2.append(h)

            histories.append(his2)

        sorted_histories = heapq.merge(*histories)

        for val in sorted_histories:

            # TODO review hash data format according to DAQ specs

            # pack tuple into hash
            h = Hash()

            devid = val[4].split(".")[0]
            propid = val[4].split(".")[1]

            h.set("timestamp", val[0])
            h.set("trainId", val[1])
            h.set("deviceId", devid)
            h.set(propid, val[3])

            # put hash into fifo
            self.append(h)
