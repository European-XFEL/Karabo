from ..api import (read_binary_hash, read_binary_schema, write_binary_hash,
                   write_binary_schema, Hash, Schema)
from .utils import (create_api1_hash, create_api2_hash, create_refactor_hash,
                    check_hash, check_hash_simple)


def test_refactor_round_trip():
    tst_hash = create_refactor_hash()
    buffer = write_binary_hash(tst_hash)

    hsh = read_binary_hash(buffer)
    check_hash(hsh)


def test_schema_round_trip():
    sh = Hash()
    sh["a"] = Hash()
    sh["a", "nodeType"] = 0
    tst_schema = Schema(name="foo", hash=sh)
    buffer = write_binary_schema(tst_schema)
    sch = read_binary_schema(buffer)

    assert sch.name == tst_schema.name
    assert sch.hash == tst_schema.hash


def test_api1_mixed_round_trip():
    from karabo.api_1 import BinarySerializerHash

    tst_hash = create_api1_hash()
    ser = BinarySerializerHash.create("Bin")

    buffer = ser.save(tst_hash)
    check_hash_simple(read_binary_hash(buffer))

    tst_hash = create_refactor_hash()
    buffer = write_binary_hash(tst_hash)
    check_hash_simple(ser.load(buffer))


def test_api2_mixed_round_trip():
    from karabo.api_2 import BinaryWriter, BinaryParser

    tst_hash = create_api2_hash()

    w = BinaryWriter()
    buffer = w.write(tst_hash)
    check_hash(read_binary_hash(buffer))

    tst_hash = create_refactor_hash()
    buffer = write_binary_hash(tst_hash)
    r = BinaryParser()
    check_hash(r.read(buffer))
