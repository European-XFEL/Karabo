from collections import deque
import time

# These are currently under another import, as they will be unecessary
# and removed once the actual query functions will be implemented
from karabo.middlelayer import (background, Hash, Int8, MetricPrefix, Unit,
                                getDevice)
import time

from karabo.middlelayer_api.device_client import getHistory

from .utils import getConfigurationFromPast


def parseDevicesFromGenericproxyId(gid):
    """
    :param gid: genericProxyId 
    e.g.: Movable(BeckhoffMotorAsMovable('M1'), BeckhoffMotorAsMovable('M2')) 
    :return:
    list of bound devices IDs
    e.g.: ['M1', 'M2']
    """
    dids = []
    if "('" in gid:
        while True:
            d = gid[gid.find("('")+2: gid.find("')")]
            if d:
                dids.append(d)
            else:
                break
            gid = gid[gid.find("')") + 2:]
    else: # in case of single proxy gid IS deviceId
        dids = [gid]

    return dids

class AcquiredData():
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
                           "2010-01-01T00:00:00",
                           time.strftime('%Y-%m-%dT%H:%M:%S'))
        # steps has the following format:
        # [(seconds_from_1970, train_id, is_last_of_set, value) ]


        # TODO this should be the proper way assuming correct timestamping
        # begin of the scan = timestamp of first step
        begin = time.strftime('%Y-%m-%dT%H:%M:%S',time.localtime(steps[0][0]))

        # end of the scan = timestamp of last step
        end = time.strftime('%Y-%m-%dT%H:%M:%S',
                            time.localtime(steps[len(steps)-1][0]))

        # TODO this is to workaround tha fact that
        # for some reason stepNum values have all the same timestamp in logged
        # data. I assume it to be the beginning and assume that the scan is
        # over
        if(begin >= end):
            end = time.strftime('%Y-%m-%dT%H:%M:%S', time.localtime())


        # TODO this just get everything about the scan since timestamps
        # are apparently random ...
        begin = "2010-01-01T00:00:00"
        end = time.strftime('%Y-%m-%dT%H:%M:%S', time.localtime())

        # retrieve parameters from history

        # At the moment the only way to get bound devices IDs from Ascan is to
        # parse movableId and sensibleId

        print(begin, end)

        his = getHistory("{}.movableId".format(self.experimentId), begin, end)
        movid = his[0][3]
        movables = parseDevicesFromGenericproxyId(movid)

        his = getHistory("{}.sensibleId".format(self.experimentId), begin, end)
        measid = his[0][3]
        measurables = parseDevicesFromGenericproxyId(measid)

        for m in movables:
            h,s = getConfigurationFromPast(m, begin)
            properties = h.getKeys()


#        pos_history = []
#        for mid in motids:
#            dev = getDevice(mid)
#            state_history = getHistory(dev.state, begin, end )
#            pos_history = getHistory(dev.encoderPosition, begin, end )
#            print(state_history)
#            print(pos_history)
#            print("=========")


        # keep only values whith same train_id of scan steps
        # TODO

        # format them into a hash
        # TODO

        # append to fifo with self.append()
        # TODO

