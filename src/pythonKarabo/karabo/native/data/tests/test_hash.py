import numpy as np
from numpy.testing import assert_array_equal

from karabo.native import Hash, HashByte, HashElement, HashList, Schema


def check_array(array, expected):
    assert_array_equal(array, expected)
    assert array.dtype == expected.dtype


def check_hash(h, special=True):
    """check that the hash *h* is the same as created by `create_hash`

    :param special: Special defines if special assertions are made.
                    This is needed, as a few operations might lose a type.
                    Default is `True`.

        Special actions:

            assert isinstance(h["emptyhashlist"], HashList)
    """

    keys = ["bool", "int", "string", "stringlist", "char", "chars",
            "vector", "emptyvector", "hash", "emptyhashlist",
            "hashlist", "emptystringlist", "nada", "schema",
            "element", "subelement"]
    assert list(h.keys()) == keys
    assert h["bool"] is True
    assert h["int"] == 4
    assert h["string"] == "bla"
    assert isinstance(h["string"], str)
    assert h["stringlist"] == ["bla", "blub"]

    # Char and chars
    assert h["char"] == "c"
    assert h["chars"] == b"bla"
    assert isinstance(h["chars"], bytes)

    # Nested values
    assert h["hash.a"] == 3
    assert h["hash.b"] == 7.1

    # Hashlist
    assert h["emptyhashlist"] == []
    if special:
        assert isinstance(h["emptyhashlist"], HashList)
    assert len(h["hashlist"]) == 2
    assert h["hashlist"][0]["a"] == 3
    assert len(h["hashlist"][1]) == 0

    # Test all vector types
    assert_array_equal(h["vector"], np.arange(7))
    check_array(h["vector", "int8"], np.arange(8, dtype=np.int8))
    check_array(h["vector", "int16"], np.arange(16, dtype=np.int16))
    check_array(h["vector", "int32"], np.arange(32, dtype=np.int32))
    check_array(h["vector", "int64"], np.arange(64, dtype=np.int64))
    check_array(h["vector", "uint8"], np.arange(8, dtype=np.uint8))
    check_array(h["vector", "uint16"], np.arange(16, dtype=np.uint16))
    check_array(h["vector", "uint32"], np.arange(32, dtype=np.uint32))
    check_array(h["vector", "uint64"], np.arange(64, dtype=np.uint64))

    # Empty vectors and lists
    check_array(h["emptyvector"], np.array([]))
    assert h["emptystringlist"] == []

    # Simple values
    assert h["bool", "bool"] is False
    assert h["int", "float"] == np.float32(7.3)
    assert h["int", "double"] == np.float64(24)

    # Simple values as attribute
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

    assert h["element"] == 5
    attrs = h["element", ...]
    assert attrs["minInc"] == 5
    assert attrs["alarmLow"] == -20

    assert isinstance(h["subelement"], Hash)
    assert isinstance(h["subelement.node"], Hash)
    assert h["subelement.node.subnode"] == 15
    attrs = h["subelement.node.subnode", ...]
    assert attrs["minInc"] == 2

    # Hash Element based getters
    value, attrs = h.getElement("element")
    assert value == 5
    assert attrs["minInc"] == 5
    assert attrs["alarmLow"] == -20
    ellipsis_attrs = h["element", ...]
    assert attrs == ellipsis_attrs

    element = h.getElement("element")
    value, attrs = element
    assert value == 5
    assert attrs["minInc"] == 5
    assert attrs["alarmLow"] == -20

    # Test noded elements
    element = h.getElement("subelement.node")
    value, attrs = element
    assert isinstance(value, Hash)
    assert value["subnode"] == 15
    h_attrs = value["subnode", ...]
    assert h_attrs["minInc"] == 2

    # More direct
    element = h.getElement("subelement.node.subnode")
    value, attrs = element
    assert value == 15
    assert attrs["minInc"] == 2


def test_hash_mutable():
    """Test a Hash with mutable objects"""
    h = Hash()
    h.setElement("element", 5, {"minInc": 5, "alarmLow": -20})
    h.setElement("subelement.node.subnode", 15, {"minInc": 2})
    h.setElement("vector", [], {"maxSize": 5})

    # 1. Mutable check! Change element no node
    element = h.getElement("element")
    element = "Wrong"

    element = h.getElement("element")
    value, attrs = element
    assert value == 5
    assert attrs["minInc"] == 5

    # 2. Mutable check! Change value and attrs
    element = h.getElement("element")
    value, attrs = element
    value = "Wrong"
    attrs = {"minInc": 25}

    element = h.getElement("element")
    value, attrs = element
    assert value == 5
    assert attrs["minInc"] == 5

    # 3. Mutable check! Change element node
    element = h.getElement("subelement.node")
    element = "Wrong"

    element = h.getElement("subelement.node")
    value, attrs = element
    assert isinstance(value, Hash)
    assert value["subnode"] == 15

    h_attrs = value["subnode", ...]
    assert h_attrs["minInc"] == 2

    # 4. Mutable check. Modify attribute dict directly!
    value, attrs = h.getElement("element")
    assert value == 5
    assert attrs == {"alarmLow": -20, "minInc": 5}
    value = 255
    attrs.update({"minInc": 537})

    element = h.getElement("element")
    value, attrs = element
    assert value == 5
    assert attrs["minInc"] == 537

    # 5. Mutable check. Modify value vector directly!
    value, _ = h.getElement("vector")
    assert value == []
    value.append(255)

    element = h.getElement("vector")
    value, _ = element
    assert value == [255]

    # 6. Work with HashElement
    ele = HashElement(*h.getElement("vector"))
    assert isinstance(ele, HashElement)
    value, attrs = ele
    assert value == [255]
    assert attrs["maxSize"] == 5

    # Modifying the HashElement can change as well!
    ele.data.append(15)
    ele.attrs["maxSize"] = 25
    value = h["vector"]
    max_size = h["vector", "maxSize"]
    assert value == [255, 15]
    assert max_size == 25

    # No changes as expected!
    ele.data = 5
    ele.attrs = {}
    value = h["vector"]
    attrs = h["vector", ...]
    assert value == [255, 15]
    assert attrs == {"maxSize": 25}


def create_hash():
    h = Hash()
    h["bool"] = True
    h["int"] = 4
    h["string"] = "bla"
    h["stringlist"] = ["bla", "blub"]
    h["char"] = HashByte("c")
    h["chars"] = b"bla"
    h["vector"] = np.arange(7, dtype=np.int64)
    h["emptyvector"] = np.array([])
    h["hash"] = Hash("a", 3, "b", 7.1)
    h["emptyhashlist"] = HashList()
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

    h["vector", "int8"] = np.arange(8, dtype=np.int8)
    h["vector", "int16"] = np.arange(16, dtype=np.int16)
    h["vector", "int32"] = np.arange(32, dtype=np.int32)
    h["vector", "int64"] = np.arange(64, dtype=np.int64)
    h["vector", "uint8"] = np.arange(8, dtype=np.uint8)
    h["vector", "uint16"] = np.arange(16, dtype=np.uint16)
    h["vector", "uint32"] = np.arange(32, dtype=np.uint32)
    h["vector", "uint64"] = np.arange(64, dtype=np.uint64)

    sh = Hash()
    sh["a"] = Hash()
    sh["a", "nodeType"] = 0
    h["schema"] = Schema("blub", hash=sh)

    h.setElement("element", 5, {"minInc": 5, "alarmLow": -20})
    h.setElement("subelement.node.subnode", 15, {"minInc": 2})

    return h


def test_constructors():
    h = Hash()
    assert len(h) == 0

    h = Hash("a", 1)
    assert len(h) == 1
    assert h["a"] == 1

    h = Hash("a", 1, "b", 2.0)
    assert len(h) == 2
    assert h["a"] == 1
    assert h["b"] == 2.0

    h = Hash("a.b.c", 1, "b.c", 2.0, "c", 3.7, "d.e", "4",
             "e.f.g.h", [5, 5, 5, 5, 5], "F.f.f.f.f", Hash("x.y.z", 99))
    assert len(h) == 6
    assert h["a.b.c"] == 1
    assert h["b.c"] == 2.0
    assert h["c"] == 3.7
    assert h["d.e"] == "4"
    assert h["e.f.g.h"][0] == 5
    assert len(h["e.f.g.h"]) == 5
    assert h["F.f.f.f.f"]["x.y.z"] == 99
    assert h["F.f.f.f.f.x.y.z"] == 99
    assert h["F"]["f"]["f"]["f"]["f"]["x"]["y"]["z"] == 99

    del h["a.b.c"]
    assert not ("a.b.c" in h)

    h = Hash({"foo": 42, "bar": np.pi})
    assert h["foo"] == 42
    assert h["bar"] == np.pi


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


def test_deepcopy():
    from copy import deepcopy
    h = create_hash()

    c = h.deepcopy()
    # We lose type HashList in copy
    check_hash(c, special=False)

    d = deepcopy(h)
    check_hash(d, special=False)
    # Test equal values and attrs
    assert h.fullyEqual(c)
    assert h.fullyEqual(d)

    # From here, start modifying
    # Add empty Hash
    h["emptyNewHash"] = Hash()
    c = h.deepcopy()
    d = deepcopy(h)
    assert h.fullyEqual(c)
    assert h.fullyEqual(d)
    assert isinstance(c["emptyNewHash"], Hash)
    assert isinstance(d["emptyNewHash"], Hash)
    assert c["emptyNewHash"].empty()
    assert d["emptyNewHash"].empty()
    assert not c["emptyNewHash", ...]
    assert not d["emptyNewHash", ...]

    # Add empty Hash and attrs
    h["emptyNewHash"] = Hash()
    h["emptyNewHash", "minInc"] = 5
    c = h.deepcopy()
    d = deepcopy(h)
    assert h.fullyEqual(c)
    assert h.fullyEqual(d)
    assert isinstance(c["emptyNewHash"], Hash)
    assert isinstance(d["emptyNewHash"], Hash)
    # Empty only counts for keys
    assert c["emptyNewHash"].empty()
    assert d["emptyNewHash"].empty()
    assert c["emptyNewHash", "minInc"] == 5
    assert d["emptyNewHash", "minInc"] == 5


def test_hash_fully_equal():
    # test empty Hash comparison
    h = create_hash()
    hh = create_hash()
    assert h.fullyEqual(hh)
    h["emptyNewHash"] = Hash()
    assert not h.fullyEqual(hh)

    # test empty Hash comparison other way
    h = create_hash()
    hh = create_hash()
    assert h.fullyEqual(hh)
    hh["emptyNewHash"] = Hash()
    assert not h.fullyEqual(hh)

    # test empty hash both sides
    h = create_hash()
    h["hca"] = Hash()
    hh = create_hash()
    hh["hca"] = Hash()
    assert h.fullyEqual(hh)

    # Now modify an attribute
    hh["hca", "minInc"] = 5
    assert not h.fullyEqual(hh)


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
    a.merge(b, attribute_policy="overwrite")

    assert a["foo", ...] == b["foo", ...]


def test_paths_and_keys():
    paths = ["bool", "int", "string", "stringlist", "char", "chars",
             "vector", "emptyvector", "hash.a", "hash.b", "hash",
             "emptyhashlist", "hashlist", "emptystringlist", "nada",
             "schema", "element", "subelement.node.subnode",
             "subelement.node", "subelement"]
    h = create_hash()
    assert h.paths(intermediate=True) == paths
    # Test the default
    assert h.paths() == paths

    no_intermediate = ["bool", "int", "string", "stringlist", "char", "chars",
                       "vector", "emptyvector", "hash.a", "hash.b",
                       "emptyhashlist", "hashlist", "emptystringlist", "nada",
                       "schema", "element", "subelement.node.subnode"]
    assert h.paths(intermediate=False) == no_intermediate


def test_schema_simple():
    s = Schema("a_schema")
    assert len(s.hash) == 0


def test_hash_element():
    h = HashElement(5, {"minInc": 2})
    value, attrs = h
    assert value == 5
    assert attrs == {"minInc": 2}