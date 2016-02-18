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
    assert h["int", "float"] == 7.3
    assert h["int", "complex"] == 1.0 + 1.0j
    assert h["hash", "int"] == 3
    assert h["string", "chars"] == b"blub"
    assert isinstance(h["string", "chars"], bytes)
    assert h["chars", "string"] == "laber"
    assert isinstance(h["chars", "string"], str)
    assert h["schema"].name == "blub"
    sh = h["schema"].hash
    assert not sh["a"].keys()
    assert sh["a", "nodeType"] == 0


def create_api1_hash():
    from karabo.api_1 import Hash, Schema

    h = Hash()
    h.set("bool", True)
    h.set("int",  4)
    h.set("string", "bla")
    h.set("stringlist", ["bla", "blub"])
    h.set("chars", b"bla")
    h.set("vector", np.arange(7, dtype=np.int64))
    h.set("emptyvector", np.array([]))
    h.set("hash", Hash("a", 3, "b", 7.1))
    h.set("hashlist", [Hash("a", 3), Hash()])
    h.set("emptystringlist", [])
    h.set("nada", None)

    h.setAttribute("bool", "bool", False)
    h.setAttribute("int", "float", 7.3)
    h.setAttribute("hash", "int", 3)
    h.setAttribute("string", "chars", b"blub")
    h.setAttribute("chars", "string", "laber")

    sh = Hash()
    sh.set("a", Hash())
    sh.setAttribute("a", "nodeType", 0)
    s = Schema("blub")
    s.hash = sh
    h.set("schema", s)

    return h


def __api_2_style_hash(hash_klass, schema_klass):
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
    h["int", "float"] = 7.3
    h["int", "complex"] = 1.0 + 1.0j
    h["hash", "int"] = 3
    h["string", "chars"] = b"blub"
    h["chars", "string"] = "laber"

    sh = hash_klass()
    sh["a"] = hash_klass()
    sh["a", "nodeType"] = 0
    h["schema"] = schema_klass("blub", hash=sh)

    return h


def create_api2_hash():
    from karabo.api_2 import Hash, Schema

    return __api_2_style_hash(Hash, Schema)


def create_refactor_hash():
    from ..hash import Hash, Schema

    return __api_2_style_hash(Hash, Schema)
