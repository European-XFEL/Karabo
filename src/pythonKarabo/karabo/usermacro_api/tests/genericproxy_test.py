from karabo.middlelayer import KaraboError
from karabo.usermacros import *

OK = "\033[92mokay\033[0m"

"""
HAHAHA!
Nobody expects the spanish inquisition!
Our chief weapons are handcrafted tests, and cheap coloured outputs.

Expecting unit tests, were you!?

Instantiate three BeckhoffSimpleMotor devices as testm{1,2,3}, and a lima
camera as limasim.
Open ikarabo and import this file:

In[1]: import karabo.usermacro_api.tests.genericproxy_test
"""

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
