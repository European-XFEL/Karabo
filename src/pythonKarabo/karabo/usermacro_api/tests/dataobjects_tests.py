"""
These tests are part of the karabo.usermacros API.
To run all tests, execute:
   nostetests -v usermacro_api
"""

from asyncio import async, coroutine, create_subprocess_exec, get_event_loop, sleep
from collections import deque
from contextlib import contextmanager
import unittest
from subprocess import PIPE
import sys

from karabo.middlelayer import Device, getDevice, Hash, Slot
from karabo.middlelayer_api.tests.eventloop import (DeviceTest, sync_tst,
                                                   async_tst)

from karabo.usermacros import AcquiredOffline, AcquiredOnline
from karabo.usermacro_api.dataobjects import AcquiredData


class Abstract(object):
    """ Abstracts more complex properties real devices would have """
    pass


class TestDev(Device):

    def __init__(self, configuration):
        super().__init__(configuration)
        self.lockedBy = ""
        self._lock_count = 0

    @Slot()
    def ping(self):
        print("Ping!")


def getMockDevice(name, **kwargs):
    cls = type(name, (TestDev,), {})
    return cls(kwargs)


class TestAD(unittest.TestCase):
    def test_initialization(self):
        """Test AcquiredData object initialization"""
        ad = AcquiredData()
        self.assertEqual(ad.__repr__(), "AcquiredData(None, size=10)")
        self.assertEqual(str(ad), "Unknown Experiment: []")

        ad = AcquiredData(107)
        self.assertEqual(ad.__repr__(), "AcquiredData(107, size=10)")
        self.assertEqual(str(ad), "Experiment 107: []")

    def test_ad_fifo_behaviour(self):
        """Test the fifo behaviour"""
        ad = AcquiredData()
        for i in range(20):
            ad.append(i)
        self.assertEqual(ad._fifo, deque(
                         [10, 11, 12, 13, 14, 15, 16, 17, 18, 19],
                         maxlen=10))
        x = next(ad)
        self.assertEqual(x, 10)

        ad = AcquiredData(size=40)
        self.assertEqual(ad._fifo.maxlen, 40)

        ad.append('1')
        ad.append('2')
        self.assertEqual(ad[0], '1')
        self.assertEqual(ad[-1], '2')


class TestAcquiredOnline(unittest.TestCase):
    def test_initalization(self):
        """Test AcquiredOnline object initialization"""
        ao = AcquiredOnline()
        expRep = "AcquiredOnline(None, size=10, source=None)"
        self.assertEqual(ao.__repr__(), expRep)

        ao = AcquiredOnline(10, 'source:channel')
        expRep = "AcquiredOnline(10, size=10, source=source:channel)"
        self.assertEqual(ao.__repr__(), expRep)

    def test_append(self):
        ao = AcquiredOnline()
        data = Hash([('header', Hash([('trainId', 65535)]))])
        timestamp = Abstract()
        timestamp.timestamp = 1
        meta = Abstract()
        meta.timestamp = timestamp
        expected_hash = Hash([('timestamp', 1),
                             ('trainId', 65535),
                             ('data', data),
                             ('meta', meta)])
        ao.append(data, meta)

        self.assertEqual(len(ao), 1)
        # k-hashes don't have equality tests, test again their representation
        self.assertEqual(str(ao[0]), str(expected_hash))
        next(ao)
        self.assertEqual(len(ao), 0)


class TestAcquiredOffline(DeviceTest):
    """The tests in this class run on behalf of the device "local".

    As this class inherits from :class:`DeviceTest`, it is possible to provide
    a :func:`lifetimeManager`, such that the test can be run within an
    eventloop
    """

    @classmethod
    @contextmanager
    def lifetimeManager(cls):
        cls.local = TestDev(dict(_deviceId_="AcquiredData_UnitTests"))
        with cls.deviceManager(lead=cls.local):
            yield

    @sync_tst
    def test_failed_initalization(self):
        """Test AcquiredOffline object failed initialization"""
        with self.assertRaises(TypeError):
            ao = AcquiredOffline()

    #@async_tst
    #def test_append(self):
    #    # Initialise a bound device, that has an OuputChannel,
    #    # Overwrite its schema to output a plausible pclayer hash
    #    # check that we could format it properly
    #    # It takes a couple of seconds for the bound device to start
    #    self.process = yield from create_subprocess_exec(
    #        sys.executable, "-m", "karabo.bound_api.launcher",
    #        "run", "karabo.bound_device_test", "TestDevice",
    #        stdin=PIPE)
    #    self.process.stdin.write(b"<_deviceId_>boundDevice</_deviceId_>")
    #    self.process.stdin.close()

    #    proxy = yield from getDevice("boundDevice")
    #    yield from proxy.injectSchema()

    #    data = Hash([('header', Hash([('trainId', 65535)]))])

    #    ao = AcquiredOffline(source="boundDevice:output2")
    #    #proxy.output2 = data
    #    yield from proxy.send()
    #    expected_hash_keys = ['timestamp', 'trainId', 'data', 'meta']

    #    self.assertEqual(len(ao), 1)
    #    # k-hashes don't have equality tests, test again their representation
    #    self.assertEqual(ao[0].getKeys(), expected_hash_keys)
    #    selg.assertEqual(ao[0]['trainId'], 65535)
    #    next(ao)
    #    self.assertEqual(len(ao), 0)


if __name__ == "__main__":
    unittest.main()
