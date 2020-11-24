import numpy as np

from ..hash import Hash, Schema
from .utils import create_refactor_hash, check_hash


def test_constructors():
    h = Hash()
    assert len(h) == 0

    h = Hash('a', 1)
    assert len(h) == 1
    assert h['a'] == 1

    h = Hash('a', 1, 'b', 2.0)
    assert len(h) == 2
    assert h['a'] == 1
    assert h['b'] == 2.0

    h = Hash("a.b.c", 1, "b.c", 2.0, "c", 3.7, "d.e", "4",
             "e.f.g.h", [5, 5, 5, 5, 5], "F.f.f.f.f", Hash("x.y.z", 99))
    assert len(h) == 6
    assert h['a.b.c'] == 1
    assert h['b.c'] == 2.0
    assert h['c'] == 3.7
    assert h['d.e'] == "4"
    assert h['e.f.g.h'][0] == 5
    assert len(h['e.f.g.h']) == 5
    assert h['F.f.f.f.f']['x.y.z'] == 99
    assert h['F.f.f.f.f.x.y.z'] == 99
    assert h['F']['f']['f']['f']['f']['x']['y']['z'] == 99

    del h['a.b.c']
    assert not ('a.b.c' in h)

    h = Hash({'foo': 42, 'bar': np.pi})
    assert h['foo'] == 42
    assert h['bar'] == np.pi


def test_iteration():
    h = Hash("should", 1, "be", 2, "iterated", 3, "in", 4, "correct", 5,
             "order", 6)
    insertionOrder = [k for k in h]
    assert insertionOrder == ["should", "be", "iterated", "in", "correct",
                              "order"]
    h["be"] = "2"  # Has no effect on order

    insertionOrder = [k for k in h]
    assert insertionOrder == ["should", "be", "iterated", "in", "correct",
                              "order"]

    del h["be"]  # Remove
    h["be"] = "2"  # Must be last element in sequence now
    insertionOrder = [k for k in h]
    assert insertionOrder == ["should", "iterated", "in", "correct", "order",
                              "be"]


def test_attributes():
    h = Hash("a.b.a.b", 42)
    h["a.b.a.b", "attr1"] = "someValue"
    assert "attr1" in h["a.b.a.b", ...]
    assert h["a.b.a.b", "attr1"] == "someValue"

    h = Hash("a.b.a.b", 42)
    h["a.b.a.b", "attr1"] = "someValue"
    assert h["a.b.a.b", "attr1"] == "someValue"

    h["a.b.a.b", "attr2"] = 42
    assert h["a.b.a.b", "attr1"] == "someValue"
    assert h["a.b.a.b", "attr2"] == 42

    h["a.b.a.b", "attr2"] = 43
    assert h["a.b.a.b", "attr1"] == "someValue"
    assert h["a.b.a.b", "attr2"] == 43

    h["a.b.a.b", "attr1"] = True
    assert h["a.b.a.b", "attr1"] is True
    assert h["a.b.a.b", "attr2"] == 43
    attrs = h["a.b.a.b", ...]
    assert attrs.get("attr1") is True
    assert attrs["attr1"] is True
    assert attrs.get("attr2") == 43
    assert attrs["attr2"] == 43

    del h["a.b.a.b", "attr1"]
    assert "attr1" not in attrs


def test_copy():
    h = create_refactor_hash()
    c = Hash(h)
    check_hash(c)


def test_merge():
    a = Hash()
    a["foo"] = 42
    b = Hash()
    b["bar"] = np.pi
    b["baz"] = Hash()

    expected = Hash()
    expected["foo"] = 42
    expected["bar"] = np.pi
    expected["baz"] = Hash()

    a.merge(b)
    assert a == expected

    a = Hash()
    a["foo"] = 42
    a["baz"] = "blabla"

    b = Hash()
    b["foo"] = np.pi

    # Copy the attributes instead of updating them
    a.merge(b, attribute_policy='overwrite')

    assert a["foo", ...] == b["foo", ...]


def test_paths_and_keys():
    paths = ['bool', 'int', 'string', 'stringlist', 'chars', 'vector',
             'emptyvector', 'hash.a', 'hash.b', 'hash', 'hashlist',
             'emptystringlist', 'nada', 'schema']

    h = create_refactor_hash()
    assert h.paths() == paths


def test_schema_simple():
    s = Schema('a_schema')
    assert len(s.hash) == 0
