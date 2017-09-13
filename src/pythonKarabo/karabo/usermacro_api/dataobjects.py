from asyncio import coroutine, gather, sleep
from collections import deque
import heapq
import time

from .middlelayer import (
    connectDevice, DeviceClientBase, EventLoop, getDevice, getDevices,
    getHistory, Hash, InputChannel, shutdown, State, synchronize, waitUntilNew)
from .util import getConfigurationFromPast

DATE_FORMAT = "%Y-%m-%dT%H:%M:%S"


class AcquiredData(object):
    """ Acquired Data is an iterable object that queries various
        data sources.
        It has a buffer of a length specified by `size`.
        It returns a k-hash::

            data = AcquiredData(experimentId=1)
            h = next(data)
            isinstance(h, Hash)

        Although not mandatory, the hash is expected to contain these
        entries:
            - timestamp
            - trainId
            - data (which is a hash itself)

        The `experimentId` parameter is used for logging purposes.
        This Id should be the Id of the experiment of which the data
        we want to retrieve.
    """

    def __init__(self, experimentId=None, size=10):
        self.experimentId = experimentId
        self._fifo = deque([], size)

    def __repr__(self):
        rep = "{cls}({exp}, size={size})".format(
            cls=type(self).__name__,
            exp=self.experimentId,
            size=self._fifo.maxlen)
        return rep

    def __str__(self):
        exp = ("Experiment " + str(self.experimentId) if self.experimentId
               else "Unknown Experiment")
        srep = ', '.join('{}'.format(dat['trainId']) for dat in self._fifo)
        return "{}: [{}]".format(exp, srep)

    def append(self, data):
        self._fifo.append(data)

    def __getitem__(self, index):
        if index >= len(self):
            raise IndexError("buffer index out of range")
        return self._fifo[index]

    def __iter__(self):
        return self

    def __next__(self):
        if len(self) == 0:
            raise StopIteration
        return self._fifo.popleft()

    def __len__(self):
        return len(self._fifo)


class AcquiredOnline(AcquiredData):

    def __init__(self, experimentId=None, source=None, size=10):
        super().__init__(experimentId, size)
        self.source = source

    def __repr__(self):
        base_rep = super().__repr__()[:-1]
        return "{}, source={})".format(base_rep, self.source)

    def append(self, data, meta):
        """ This function is to be called by the owner within their
        @InputChannel.
        :param data: a k-hash provided by the InputChannel
        :param meta: a PipelineMetaData object provided by the InputChannel
        """
        formatted_hash = Hash([('timestamp', meta.timestamp.timestamp),
                               ('trainId', data['header']['trainId']),
                               ('data', data),
                               ('meta', meta)])
        super().append(formatted_hash)


class AcquiredOffline(AcquiredData, DeviceClientBase):

    def __init__(self, experimentId=None, size=10, source=None):
        """
            :param experimentId: the experimentId, used for logging, __str__
            :param size: the size of the fifo buffer, default is 10
            :param source: the output channel to listen to. The format should
                           respect the Karabo `deviceId:channelId` convention
        """
        configuration = dict(_deviceId_="OfflineData-{}".format(experimentId),
                             append=dict(connectedOutputChannels=[source]),
                             archive=False)
        DeviceClientBase.__init__(self, configuration=configuration)
        AcquiredData.__init__(self, experimentId, size)

        # The following is used as to get a grip on the event loop and allow
        # us to run as a `DeviceClientBase` on our own within Karabo
        # With this, we are able to have our own InputChannel!
        @coroutine
        def __run():
            yield from self.startInstance()
        loop = EventLoop.global_loop
        loop.call_soon_threadsafe(loop.create_task, __run())
        self.state = State.ON
        self.status = "Ready"

    def __repr__(self):
        return "{}, source={})".format(super().__repr__()[:-1],
                                       self.append.connectedOutputChannels)

    def __str__(self):
        return "{} - {} - {}".format(super().__str__(),
                                     self.state,
                                     self.status)

    @InputChannel(raw=True)
    @coroutine
    def append(self, data, meta):
        """
        Here, the assumption is that the datareader is set with its default
        configuration, and specifically that datareader.output.noInputShared
        is set to wait.

        This allows us to to keep a fifo of limited size, to consume data at
        our leisure, and not miss any entires.
        As such this function has a busy loop that waits, at the end, for
        data to be consumed, thus not returning, thus putting this
        InputChannel in noInputShared mode.

        :param data: a k-hash provided by the InputChannel
        :param meta: a PipelineMetaData object provided by the InputChannel
        """
        self.state = State.ACQUIRING
        self.status = "Incoming"
        formatted_hash = Hash([('timestamp', meta.timestamp.timestamp),
                               ('trainId', data['header']['trainId']),
                               ('data', data),
                               ('meta', meta)])
        super().append(formatted_hash)
        self.state = State.ON
        self.status = "Received Data"

        while len(self) is self._fifo.maxlen:
            yield from sleep(.1)

    @synchronize
    def query(self):
        data_reader = self.append.connectedOutputChannels[0].split(':')[0]
        with (yield from getDevice(data_reader)) as dev:
            if dev.state == State.STARTED:
                yield from dev.stop()
                yield from waitUntilNew(dev.state)
            if dev.state == State.STOPPED:
                yield from dev.start()

    def __del__(self):
        shutdown(self.deviceId)


class AcquiredFromLog(AcquiredData):
    """
    Child class to retrieve 'slow' data from datalogger history

    self.movableIds: list of bound devices IDs used in the scan as movables
    self.measurableIds: list of bound devices IDs used in the scan
      as measurables
    self.bound_devices_properties[deviceId] list of logged properties logged
      for the device
    """
    steps = []
    begin = None
    end = None
    index = 0
    attrs = []

    def __init__(self, experimentId, *args):
        size = 1000000
        if args and isinstance(args[-1], int):
            size = args[-1]
            del args[-1]
        self.attrs = args
        super().__init__(experimentId, size)

    def __next__(self):
        if self.index >= len(self):
            self.index = 0
            raise StopIteration

        item = self.__getitem__(self.index)
        self.index += 1
        return item

    @synchronize
    def query(self, *attrs, max_attempts=30):
        """
        :param: a list of property strings with the form 'deviceId.property'
        retrieved data are queued in self.data, sorted by timestamp
        """
        minWaitTime = 1

        if not attrs:
            attrs = self.attrs

        @coroutine
        def _attempt(func, *args, **kwargs):
            attempts = kwargs.get("max_attempts", max_attempts)

            ret = None
            while (not ret) and attempts:
                attempts -= 1
                ret = yield from func(*args)
                if (not ret) and attempts:
                    yield from sleep(minWaitTime)
            return ret

        @coroutine
        def _flushLogger(deviceId):
            loggerId = "DataLogger-" + deviceId

            if loggerId in getDevices():
                logger = yield from connectDevice(loggerId)
                flush = getattr(logger, "flush", None)
                if callable(flush):
                    yield from flush()
                    return minWaitTime
                else:
                    # Older data loggers do not flush in their destructor.
                    # and don't have a flush slot
                    # Hence, we must wait for data to be flushed.
                    return logger.flushInterval.magnitude
            return 0

        deviceIds = (attr.split(".", 1)[0] for attr in attrs)
        flushes = [_flushLogger(deviceId) for deviceId in deviceIds]
        waitingTimes = yield from gather(*flushes)

        if not waitingTimes:
            print("No data found.")
            return

        waitTime = max(waitingTimes)
        if waitTime:
            print("Waiting for {} s to ensure data logger flush ..."
                  .format(waitTime))
            yield from sleep(waitTime)

        print("Fetching {} from logs ...".format(attrs))
        self.index = 0

        # retrieve steps from scan history assuming there have been only one
        # scan with given deviceId from the big-bang up to now

        self.steps = yield from _attempt(
            getHistory, "{}.stepNum".format(self.experimentId),
            "2010-01-01T00:00:00", time.strftime(DATE_FORMAT))

        if not self.steps:
            print("No data found.")
            return

        # steps has the following format:
        # [(seconds_from_1970, train_id, is_last_of_set, value) ]

        # begin of the scan = timestamp of first step
        self.begin = time.strftime(DATE_FORMAT,
                                   time.localtime(self.steps[0][0]))

        # end of the scan = timestamp of last step  rounded to the next second
        self.end = time.strftime(DATE_FORMAT,
                                 time.localtime(
                                     self.steps[len(self.steps)-1][0] + 1))

        # if for any reason end value is wrong, assume end is now
        if self.begin >= self.end:
            self.end = time.strftime(DATE_FORMAT, time.localtime())

        # get IDs of devices used by Scans
        # TScans don't have BoundMovables. Don't try hard
        his = yield from _attempt(
            getHistory, "{}.boundMovables".format(self.experimentId),
            self.begin, self.end, max_attempts=2)

        self.movableIds = his[0][3] if his else []

        his = yield from _attempt(
            getHistory, "{}.boundSensibles".format(self.experimentId),
            self.begin, self.end)
        self.measurableIds = his[0][3] if his else []

        # get properties for each device
        self.bound_devices_properties = {}

        for m in self.movableIds + self.measurableIds:
            self.bound_devices_properties[m] = []
            h, s = yield from getConfigurationFromPast(m, self.begin)
            properties = h.getKeys()
            for p in properties:
                self.bound_devices_properties[m].append(p)

        histories = []
        for prop in attrs:
            his = yield from _attempt(getHistory, prop, self.begin, self.end)

            # add property name to tuples
            his2 = []
            for h in his:
                h = h + (prop,)
                his2.append(h)

            histories.append(his2)

        sorted_histories = heapq.merge(*histories)

        self._fifo.clear()
        for history in sorted_histories:
            # TODO review hash data format according to DAQ specs
            # pack tuple into hash
            devId = history[4].split(".")[0]
            propId = history[4].split(".")[1]

            formatted_hash = Hash([('timestamp', history[0]),
                                   ('trainId', history[1]),
                                   ('deviceId', devId),
                                   (propId, history[3])])
            # put hash into fifo
            self.append(formatted_hash)
