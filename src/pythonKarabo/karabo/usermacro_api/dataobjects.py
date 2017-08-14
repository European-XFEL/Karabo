from collections import deque
# These are currently under another import, as they will be unecessary
# and removed once the actual query functions will be implemented
from karabo.middlelayer import (background, Hash, Int8, MetricPrefix, Unit)
import time

from karabo.middlelayer_api.device_client import getHistory

class AcquiredData(object):
    """ Acquired Data is an iterable object that queries the DAQ
        It has a buffer of a specified `_max_queue_size` size
    """
    _max_queue_length = 10

    def __init__(self, experimentId=None, instanceId=None, size=10):
        self.experimentId = experimentId
        self.instanceId = instanceId
        self._max_queue_size = size
        self._fifo = deque([], self._max_queue_size)

        self.running = True

    def __repr__(self):
        rep = "AcquiredData({exp}, {inst}, size={size})".format(
              exp=self.experimentId,
              inst=self.instanceId,
              size=self._max_queue_size)
        return rep

    def __str__(self):
        exp = ("Experiment " + str(self.experimentId) if self.experimentId
                                                 else "Unknown Experiment")
        inst = (" Instance " + str(self.instanceId) if self.instanceId else "")
        srep = ','.join('{}'.format(dat) for dat in self._fifo)
        return exp + inst + ': [' + srep + ']'

    def append(self, data):
        self._fifo.append(data)

    def __getitem__(self, index):
        if index >= len(self):
            raise IndexError("buffer index out of range")
        return self._fifo[index]

    def _fillUp(self, func):
        while len(self) < self._max_queue_size:
            self.append(func())

    def __iter__(self):
        return self

    def __next__(self):
        if len(self) == 0:
            raise StopIteration
        ret = self._fifo.popleft()
        self._fillUp(self.queryDataReader)
        return ret

    def __len__(self):
        return len(self._fifo)

    def queryDataReader(self):
        # This is a tiny mock implementation
        d = Int8(unitSymbol=Unit.METER, metricPrefixSymbol=MetricPrefix.NANO)
        return Hash('position', d.toKaraboValue(7))


class AcquiredFromLog(AcquiredData):
    """
    Child class to retrieve 'slow' data from datalogger history
    """
    def queryData(self):

        # retrieve steps from scan history assuming there have been only one
        # scan with given deviceId from the big-bang up to now
        steps = getHistory("{}.stepNum".format(self.experimentId),
                           "01-01-1970T00:00:00",
                           time.strftime('%Y-%m-%dT%H:%M:%S'))
        # steps has the following format:
        # [(seconds_from_1970, train_id, is_last_of_set, value) ]

        # begin of the scan = timestamp of first step
        begin = time.strftime('%Y-%m-%dT%H:%M:%S',time.localtime(steps[0][0]))
        # end of the scan = timestamp of last step
        end = time.strftime('%Y-%m-%dT%H:%M:%S',
                            time.localtime(steps[0][len(steps)]))

        print(steps) #TODO remove thios line

        # retrieve parameters from history
        # TODO

        # keep only values whith same train_id of scan steps
        # TODO

        # format them into a hash
        # TODO

        # append to fifo with self.append()
        # TODO

