from collections import deque
from karabo.middlelayer import Hash


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
