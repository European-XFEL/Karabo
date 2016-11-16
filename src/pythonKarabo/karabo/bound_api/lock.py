from contextlib import contextmanager


from karathon import Hash

class LockError(Exception):
    pass

class Lock(contextmanager):

    def __init__(self, signalSlotable, deviceId, recursive=False):
        super().__init__()
        self._ss = signalSlotable
        self._deviceId = deviceId
        self._recursive = recursive
        self._valid = False
        self._ownInstance = self._ss.getInstanceId()
        self._lockTimeOut = 5000

    def __enter__(self):
        self._lockImpl(self._recursive)

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._unlockImpl()

    def lock(self, recursive=False):
        self._lockImpl(recursive)

    def unlock(self):
        self._unlockImpl()

    def _lockImpl(self, recursive):
        try:
            h = None
            self._ss.request(self._deviceId, "slotGetConfiguration")\
                .timeout(self._lockTimeOut).receive(h)
            lockHolder = h.get("lockedBy")
            if (not self._recursive and lockHolder == "")\
                or (self._recursive and lockHolder != self._ownInstance and
                    lockHolder != ""):
                raise LockError("Could not acquire lock on {}, it is locked "
                                "by {}".format(self._deviceId, lockHolder))

        except Exception as e: #this will be a nasty boost::python exception
            raise LockError("Could not acquire lock on {}: {}"
                            .format(self._deviceId, str(e)))

        #now we try setting
        ok = None
        errorText = None
        self._ss.request(self._deviceId, "slotReconfigure",
                         Hash("lockedBy",self._ownInstance))\
                        .timeout(self._lockTimeOut).receive(ok, errorText)

        h = None
        self._ss.request(self._deviceId, "slotGetConfiguration")\
                .timeout(self._lockTimeOut).receive(h)
        lockHolder = h.get("lockedBy")

        if(self._ownInstance != lockHolder or not ok):
            raise LockError("Could not acquire lock on {}, it is locked "
                            "by {}".format(self._deviceId, lockHolder))

        self._valid = True


    def _unlockImpl(self):
        if(self._valid):
            self._ss.call(self._deviceId, "slotClearLock")


    def valid(self):
        h = None
        self._ss.request(self._deviceId, "slotGetConfiguration")\
                .timeout(self._lockTimeOut).receive(h)
        lockHolder = h.get("lockedBy")
        return (self._valid and lockHolder == self._deviceId)