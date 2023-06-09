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


def test_empty():
    # Check properties of empty Hash
    h = Hash()
    assert len(h) == 0
    assert h.empty()


def test_one_property():
    # Check Hash with one property
    h = Hash('a', 1)
    assert len(h) == 1
    assert not h.empty()
    assert h.get('a') == 1
    assert h['a'] == 1


def test_two_properties():
    # Check Hash with 2 properties
    h = Hash('a', 1, 'b', 2.0)
    assert not h.empty()
    assert len(h) == 2
    assert h['a'] == 1
    assert h['b'] == 2.0


def test_six_properties():
    # Check Hash with 6 properties of different types
    h = Hash("a.b.c", 1, "b.c", 2.0, "c", 3.7, "d.e", "4",
             "e.f.g.h", [5, 5, 5, 5, 5], "F.f.f.f.f", Hash("x.y.z", 99))
    assert not h.empty()
    assert len(h) == 6
    assert h['a.b.c'] == 1
    assert h['b.c'] == 2.0
    assert h['c'] == 3.7
    assert h['d.e'] == "4"
    assert h['e.f.g.h'][0] == 5
    assert len(h['e.f.g.h']) == 5
    assert h['F.f.f.f.f']['x.y.z'] == 99
    assert h['F.f.f.f.f.x.y.z'] == 99
    # Make Hash flat
    flat = Hash()
    Hash.flatten(h, flat)
    assert not flat.empty()
    assert len(flat) == 6
    assert flat.get('a.b.c', ' ') == 1
    assert flat.get('b.c', ' ') == 2.0
    assert flat.get('c', ' ') == 3.7
    assert flat.get('d.e', ' ') == "4"
    assert flat.get('e.f.g.h', ' ')[0] == 5
    assert len(flat.get('e.f.g.h', ' ')) == 5
    assert flat.get('F.f.f.f.f.x.y.z', ' ') == 99

    # Make flat Hash unflatten again
    tree = Hash()
    flat.unflatten(tree)
    assert not tree.empty()
    assert len(tree) == 6
    assert tree['a.b.c'] == 1
    assert tree['b.c'] == 2.0
    assert tree['c'] == 3.7
    assert tree['d.e'] == "4"
    assert tree['e.f.g.h'][0] == 5
    assert len(tree['e.f.g.h']) == 5
    assert tree['F.f.f.f.f']['x.y.z'] == 99
    assert tree['F.f.f.f.f.x.y.z'] == 99
