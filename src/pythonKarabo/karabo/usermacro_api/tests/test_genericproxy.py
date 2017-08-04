from asyncio import coroutine
import unittest
from contextlib import contextmanager
from karabo.middlelayer import Device, KaraboError, Slot
from karabo.middlelayer_api.tests.eventloop import DeviceTest, sync_tst, async_tst
from karabo.middlelayer_api.injectable import Injectable

from karabo.usermacros import (BeckhoffMotorAsMovable, GenericProxy, Movable,
                               Sensible)

class TestDev(Injectable, Device):

    def __init__(self, configuration):
        super().__init__(configuration)

    @Slot
    def ping(self):
        print("Ping!")


class Tests(DeviceTest):

    @classmethod
    @contextmanager
    def lifetimeManager(cls):

        cls.local = TestDev({"_deviceId_": "GenericProxy_UnitTests"})

        cls.testm1 = TestDev({"_deviceId_": "tm1",
                              "_serverId_": "TestServer",
                              "stepLength": 0,
                              "lockedBy": "YOLO",
                              "_classId_": "BeckhoffMC2Motor"})
        cls.testm2 = TestDev({"_deviceId_": "tm2",
                              "stepLength": 0,
                              "lockedBy": "",
                              "_classId_": "BeckhoffSimpleMotor"})
        cls.testm3 = TestDev({"_deviceId_": "tm3",
                              "stepLength": 0,
                              "lockedBy": "",
                              "_classId_": "BeckhoffSimpleMotor"})
        cls.limasim = TestDev({"_deviceId_": "lsim",
                               "_classId_": "LimaSimulatedCamera",
                               "cameraType": "Simulator"})
        with cls.deviceManager(cls.testm1, cls.testm2, cls.testm3,
                               cls.limasim, lead=cls.local):
            yield

    @sync_tst
    def test_single_motor_instantiation(self):
        output = GenericProxy('testm1')
        self.assertEqual(type(output), BeckhoffMotorAsMovable)
        self.assertEqual(output.__repr__(), "BeckhoffMotorAsMovable('testm1')")

    @sync_tst
    def test_wrong_type_instantiation(self):
        with self.assertRaises(KaraboError):
            output = yield from Movable('lsim')

    @sync_tst
    def test_triplet_instantiation(self):
        tripletRep = "Movable(BeckhoffMotorAsMovable('tm1'), \
        BeckhoffMotorAsMovable('tm2'), BeckhoffMotorAsMovable('tm3')"

        output = yield from GenericProxy('tm1', 'tm2', 'tm3')
        self.assertEqual(output.__repr__(), tripletRep)

        output = yield from Movable('tm1', 'tm2', 'tm3')
        self.assertEqual(output.__repr__(), tripletRep)

    @sync_tst
    def test_triplet_container_instantiation(self):
        containerRep = "Movable(BeckhoffMotorAsMovable('testm1'), \
        Movable(BeckhoffMotorAsMovable('testm2'), \
        BeckhoffMotorAsMovable('testm3')))"

        output = yield from GenericProxy('tm1', Movable('tm2', 'tm3'))
        self.assertEqual(output.__repr__(), containerRep)

    @sync_tst
    def test_fail_on_type_mixes(self):
        with self.assertRaises(KaraboError):
            output = yield from GenericProxy('tm1', 'lsim')
        with self.assertRaises(KaraboError):
            output = yield from Movable('testm1', 'testm2', Sensible('lsim'))



if __name__ == "__main__":
    unittest.main()

"""
HAHAHA!
Nobody expects the spanish inquisition!
Our chief weapons are handcrafted tests, and cheap coloured outputs.

Expecting unit tests, were you!?

Instantiate three BeckhoffSimpleMotor devices as testm{1,2,3}, and a lima
camera as limasim.
Open ikarabo and import this file:

In[1]: import karabo.usermacro_api.tests.genericproxy_test


print("TEST 1: ", end='')
output = None
output = GenericProxy('testm1')
print('single generation as GP {}:\n{}'.format(OK, output))

#################################################

output = None
output = GenericProxy('testm1', 'testm2', 'testm3')
print("TEST 2: ", end='')

if type(output) == Movable:
    print('triple generation as GP {}:\n{}'
          .format(OK, output))
else:
    raise Exception("Triple generation as GP did not generate container: {}".
                    format(output))

#################################################

print("TEST 3: ", end='')
output = None
output = Movable('testm1', 'testm2', 'testm3')

if type(output) == Movable:
    print('triple generation as Movable {}:\n{}'
          .format(OK, output))
else:
    raise Exception("Triple generation as Movable did not generate container: {}"
                    .format(output))
#################################################

print("TEST 4: ", end='')
output = None
output = Movable('testm1', Movable('testm2', 'testm3'))

if type(output) == Movable:
    print('triple generation as Movable Container {}:\n{}'
          .format(OK, output))
else:
    raise Exception("Triple generation did not generate container")

#################################################

print("TEST 5: ", end='')
output = None
output = Movable(Movable('testm2'))

if type(output) == Movable:
    print('generation as Movable Container from Movable(Movable) {}:\n{}'
          .format(OK, output))
else:
    raise Exception("Movable(Movable) -> Movable generation did not generate container")

#################################################

print("TEST 6: ", end='')
output = None
output = GenericProxy(Movable('testm2'))

if type(output) == Movable:
    print('generation as Movable Container from GP(Movable) {}:\n{}'
          .format(OK, output))
else:
    raise Exception("GP(Movable) -> Movable generation did not generate container")

#################################################

print("TEST 7: ", end='')
output = None
output = Movable(GenericProxy('testm2'))

if type(output) == Movable:
    print('generation as Movable Container from Movable(GP) {}:\n{}'
          .format(OK, output))
else:
    raise Exception("Movable(GP) -> Movable generation did not generate container")

#################################################

print("TEST 8: ", end='')
output = None
output = GenericProxy(GenericProxy('testm2'))

if type(output) == Movable:
    print('generation as Movable from Movable {}:\n{}'
          .format(OK, output))
else:
    raise Exception("Movable -> Movable generation did not generate container")

#################################################

print("TEST 9: ", end='')
output = None
try:
    output = GenericProxy('testm1', 'testm2', 'limasim')
except KaraboError as e:
    print('triple generation with Mixed GP failed {}:\n{}'
          .format(OK, e))

#################################################

print("TEST 10: ", end='')
output = None
try:
    output = Movable('testm1', 'testm2', 'limasim')
except KaraboError as e:
    print('triple generation with Mixed as Movable failed {}:\n{}'
          .format(OK, e))
"""
