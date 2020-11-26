import numpy as np
from numpy.testing import assert_equal

from ..hash import Hash, Schema


def check_hash(h):
    """check that the hash *h* is the same as created by `create_hash`

    This method does advanced checking only available for
    Python-only hashes.
    """

    keys = ["bool", "int", "string", "stringlist", "chars", "vector",
            "emptyvector", "hash", "hashlist", "emptystringlist", "nada",
            "schema"]
    assert list(h.keys()) == keys
    assert h["bool"] is True
    assert h["int"] == 4
    assert h["string"] == "bla"
    assert isinstance(h["string"], str)
    assert h["stringlist"] == ["bla", "blub"]
    assert h["chars"] == b"bla"
    assert h["hash.a"] == 3
    assert h["hash.b"] == 7.1
    assert len(h["hashlist"]) == 2
    assert h["hashlist"][0]["a"] == 3
    assert len(h["hashlist"][1]) == 0
    assert_equal(h["vector"], np.arange(7))
    assert_equal(h["emptyvector"], np.array([]))
    assert h["emptystringlist"] == []

    assert isinstance(h["chars"], bytes)
    assert h["bool", "bool"] is False
    assert h["int", "float"] == np.float32(7.3)
    assert h["int", "double"] == np.float64(24)

    value = h["hash", "int8"]
    assert value == 8
    assert value.dtype == np.int8
    value = h["hash", "int16"]
    assert value == 16
    assert value.dtype == np.int16
    value = h["hash", "int32"]
    assert value == 32
    assert value.dtype == np.int32
    value = h["hash", "int64"]
    assert value == 64
    assert value.dtype == np.int64
    value = h["hash", "uint8"]
    assert value == 8
    assert value.dtype == np.uint8
    value = h["hash", "uint16"]
    assert value == 16
    assert value.dtype == np.uint16
    value = h["hash", "uint32"]
    assert value == 32
    assert value.dtype == np.uint32
    value = h["hash", "uint64"]
    assert value == 64
    assert value.dtype == np.uint64

    assert h["string", "chars"] == b"blub"
    assert isinstance(h["string", "chars"], bytes)
    assert h["chars", "string"] == "laber"
    assert h["vector", "complex"] == 1.0 + 1.0j
    assert isinstance(h["chars", "string"], str)
    assert h["schema"].name == "blub"
    sh = h["schema"].hash
    assert not sh["a"].keys()
    assert sh["a", "nodeType"] == 0


def create_hash():
    h = Hash()
    h["bool"] = True
    h["int"] = 4
    h["string"] = "bla"
    h["stringlist"] = ["bla", "blub"]
    h["chars"] = b"bla"
    h["vector"] = np.arange(7, dtype=np.int64)
    h["emptyvector"] = np.array([])
    h["hash"] = Hash("a", 3, "b", 7.1)
    h["hashlist"] = [Hash("a", 3), Hash()]
    h["emptystringlist"] = []
    h["nada"] = None

    h["bool", "bool"] = False
    h["int", "float"] = np.float32(7.3)
    h["int", "double"] = np.float64(24)

    h["hash", "int8"] = np.int8(8)
    h["hash", "int16"] = np.int16(16)
    h["hash", "int32"] = np.int32(32)
    h["hash", "int64"] = np.int64(64)
    h["hash", "uint8"] = np.uint8(8)
    h["hash", "uint16"] = np.uint16(16)
    h["hash", "uint32"] = np.uint32(32)
    h["hash", "uint64"] = np.uint64(64)

    h["string", "chars"] = b"blub"
    h["chars", "string"] = "laber"
    h["vector", "complex"] = 1.0 + 1.0j

    sh = Hash()
    sh["a"] = Hash()
    sh["a", "nodeType"] = 0
    h["schema"] = Schema("blub", hash=sh)

    return h


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
    h = create_hash()
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

    h = create_hash()
    assert h.paths() == paths


def test_schema_simple():
    s = Schema('a_schema')
    assert len(s.hash) == 0
