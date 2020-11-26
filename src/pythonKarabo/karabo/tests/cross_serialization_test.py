from karabo.native.karabo_hash import (
    encodeXML, decodeXML, encodeBinary, decodeBinary)

import numpy as np
from numpy.testing import assert_equal


def check_hash_simple(h):
    """check that the hash *h* is the same as created by `create_hash`

    This method is simple enough that it works for both C++ and
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


def check_hash(h):
    """check that the hash *h* is the same as created by `create_hash`

    This method does advanced checking only available for
    Python-only hashes.
    """
    check_hash_simple(h)
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


def create_middlelayer_api_hash():
    from karabo.middlelayer import Hash, Schema

    return _pure_py_style_hash(Hash, Schema)


def create_refactor_hash():
    from karabo.native.karabo_hash import Hash, Schema

    return _pure_py_style_hash(Hash, Schema)


def create_bound_api_hash():
    from karabo.bound import Hash, Schema

    h = Hash()
    h.set("bool", True)
    h.set("int", 4)
    h.set("string", "bla")
    h.set("stringlist", ["bla", "blub"])
    h.set("chars", b"bla")
    h.set("vector", list(range(7)))
    h.set("emptyvector", [])
    h.set("hash", Hash("a", 3, "b", 7.1))
    h.set("hashlist", [Hash("a", 3), Hash()])
    h.set("emptystringlist", [])
    h.set("nada", None)

    h.setAttribute("bool", "bool", False)
    h.setAttribute("int", "float", 7.3)
    h.setAttribute("int", "double", 24)
    h.setAttribute("hash", "int8", 8)
    h.setAttribute("hash", "int16", 16)
    h.setAttribute("hash", "int32", 32)
    h.setAttribute("hash", "int64", 64)
    h.setAttribute("hash", "uint8", 8)
    h.setAttribute("hash", "uint16", 16)
    h.setAttribute("hash", "uint32", 32)
    h.setAttribute("hash", "uint64", 64)

    h.setAttribute("string", "chars", b"blub")
    h.setAttribute("chars", "string", "laber")
    h.setAttribute("vector", "complex", 1.0 + 1.0j)

    sh = Hash()
    sh.set("a", Hash())
    sh.setAttribute("a", "nodeType", 0)
    s = Schema("blub")
    s.hash = sh
    h.set("schema", s)

    return h


def _pure_py_style_hash(hash_klass, schema_klass):
    h = hash_klass()
    h["bool"] = True
    h["int"] = 4
    h["string"] = "bla"
    h["stringlist"] = ["bla", "blub"]
    h["chars"] = b"bla"
    h["vector"] = np.arange(7, dtype=np.int64)
    h["emptyvector"] = np.array([])
    h["hash"] = hash_klass("a", 3, "b", 7.1)
    h["hashlist"] = [hash_klass("a", 3), hash_klass()]
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

    sh = hash_klass()
    sh["a"] = hash_klass()
    sh["a", "nodeType"] = 0
    h["schema"] = schema_klass("blub", hash=sh)

    return h


def test_xml_middlelayer_api_mixed_round_trip():
    from karabo.native import (
        encodeXML as encodeXMLOld,
        decodeXML as decodeXMLOld)

    # Old encoder, new decoder
    tst_hash = create_middlelayer_api_hash()
    buffer = encodeXMLOld(tst_hash)
    check_hash(decodeXML(buffer))

    # New encoder, old decoder
    tst_hash = create_refactor_hash()
    buffer = encodeXML(tst_hash)
    check_hash(decodeXMLOld(buffer))


def test_xml_bound_api_mixed_round_trip():
    from karabo.bound import TextSerializerHash

    ser = TextSerializerHash.create("Xml")

    tst_hash = create_bound_api_hash()
    buffer = ser.save(tst_hash)
    check_hash_simple(decodeXML(buffer))

    tst_hash = create_refactor_hash()
    buffer = encodeXML(tst_hash)
    check_hash_simple(ser.load(buffer))


def test_bin_bound_api_mixed_round_trip():
    from karabo.bound import BinarySerializerHash

    tst_hash = create_bound_api_hash()
    ser = BinarySerializerHash.create("Bin")

    buffer = ser.save(tst_hash)
    check_hash_simple(decodeBinary(buffer))

    tst_hash = create_refactor_hash()
    buffer = encodeBinary(tst_hash)
    check_hash_simple(ser.load(buffer))


def test_bin_python_roundtrip():
    from karabo.native import (
        decodeBinary as decodeBinaryOld,
        encodeBinary as encodeBinaryOld)

    # old binary encoder, new decoder
    tst_hash = create_middlelayer_api_hash()
    buffer = encodeBinaryOld(tst_hash)
    check_hash(decodeBinary(buffer))

    # new binary encoder, old decoder
    tst_hash = create_refactor_hash()
    buffer = encodeBinary(tst_hash)
    check_hash(decodeBinaryOld(buffer))


def test_hash_native_roundtrip():
    tst_hash = create_refactor_hash()
    buffer = encodeBinary(tst_hash)
    hsh = decodeBinary(buffer)
    check_hash(hsh)


def test_schema_native_round_trip():
    from karabo.native.karabo_hash import Hash, Schema
    sh = Hash()
    sh["a"] = Hash()
    sh["a", "nodeType"] = 0
    tst_schema = Schema(name="foo", hash=sh)

    # XXX: Have to piggy back!
    h = Hash('sch', tst_schema)
    buffer = encodeBinary(h)
    sch = decodeBinary(buffer)
    sch = sch['sch']
    assert sch.name == tst_schema.name
    assert sch.hash == tst_schema.hash
