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
from karabo.bound import Hash


def get_hash():
    return Hash("should", 1, "be", 2, "iterated", 3, "in", 4,
                "correct", 5, "order", 6)


def test_iteration_1():
    insertionOrder = []
    h = get_hash()
    for k in h:
        insertionOrder.append(str(k))
    assert insertionOrder[0] == "should"
    assert insertionOrder[1] == "be"
    assert insertionOrder[2] == "iterated"
    assert insertionOrder[3] == "in"
    assert insertionOrder[4] == "correct"
    assert insertionOrder[5] == "order"


def test_iteration_2():
    alphaNumericOrder = []
    h = get_hash()
    for k in h:
        alphaNumericOrder.append(k.getKey())
    alphaNumericOrder.sort()
    assert alphaNumericOrder[0] == "be"
    assert alphaNumericOrder[1] == "correct"
    assert alphaNumericOrder[2] == "in"
    assert alphaNumericOrder[3] == "iterated"
    assert alphaNumericOrder[4] == "order"
    assert alphaNumericOrder[5] == "should"


def test_iteration_3():
    h = Hash(get_hash())
    h.set("be", "2")  # Has no effect on order

    insertionOrder = []
    for k in h:
        insertionOrder.append(str(k.getKey()))

    assert insertionOrder[0] == "should"
    assert insertionOrder[1] == "be"
    assert insertionOrder[2] == "iterated"
    assert insertionOrder[3] == "in"
    assert insertionOrder[4] == "correct"
    assert insertionOrder[5] == "order"


def test_iteration_4():
    alphaNumericOrder = []
    h = get_hash()
    for k in h:
        alphaNumericOrder.append(str(k.getKey()))
    alphaNumericOrder.sort()
    assert alphaNumericOrder[0] == "be"
    assert alphaNumericOrder[1] == "correct"
    assert alphaNumericOrder[2] == "in"
    assert alphaNumericOrder[3] == "iterated"
    assert alphaNumericOrder[4] == "order"
    assert alphaNumericOrder[5] == "should"


def test_iteration_5():
    h = Hash(get_hash())
    h.erase("be")
    # Must be last element in sequence now
    h.set("be", "2")

    insertionOrder = []
    for k in h:
        insertionOrder.append(str(k.getKey()))

    assert insertionOrder[0] == "should"
    assert insertionOrder[1] == "iterated"
    assert insertionOrder[2] == "in"
    assert insertionOrder[3] == "correct"
    assert insertionOrder[4] == "order"
    assert insertionOrder[5] == "be"


def test_iteration_6():
    alphaNumericOrder = []
    h = get_hash()
    for k in h:
        alphaNumericOrder.append(str(k.getKey()))
    alphaNumericOrder.sort()

    assert alphaNumericOrder[0] == "be"
    assert alphaNumericOrder[1] == "correct"
    assert alphaNumericOrder[2] == "in"
    assert alphaNumericOrder[3] == "iterated"
    assert alphaNumericOrder[4] == "order"
    assert alphaNumericOrder[5] == "should"


def test_iteration_7():
    tmp = []  # create empty set
    h = get_hash()
    h.getKeys(tmp)  # fill set by keys
    tmp.sort()
    it = iter(tmp)

    assert str(next(it)) == "be"
    assert str(next(it)) == "correct"
    assert str(next(it)) == "in"
    assert str(next(it)) == "iterated"
    assert str(next(it)) == "order"
    assert str(next(it)) == "should"


def test_iteration_8():
    tmp = []  # create empty vector
    h = get_hash()
    h.getKeys(tmp)  # fill vector by keys
    it = iter(tmp)

    assert str(next(it)) == "should"
    assert str(next(it)) == "be"
    assert str(next(it)) == "iterated"
    assert str(next(it)) == "in"
    assert str(next(it)) == "correct"
    assert str(next(it)) == "order"


def test_iteration_9():
    h = Hash("b", "bla-la-la", "a.b.c", 1, "abc.2", 2.2222222,
             "a.b.c1.d", "abc1d", "abc.1", 1.11111111)

    ll = []
    h.getKeys(ll)  # use top level keys

    assert len(ll) == 3
    i = iter(ll)  # "canonical" order: on every level insertion order
    assert str(next(i)) == "b"
    assert str(next(i)) == "a"
    assert str(next(i)) == "abc"

    ll = []
    h.getPaths(ll)   # use full keys

    assert len(ll) == 5
    i = iter(ll)
    assert str(next(i)) == "b"
    assert str(next(i)) == "a.b.c"
    assert str(next(i)) == "a.b.c1.d"
    assert str(next(i)) == "abc.2"
    assert str(next(i)) == "abc.1"
