from unittest import skip

from ..hash import Hash, Schema
from ..bin_reader import decodeBinary
from ..bin_writer import encodeBinary
from .utils import (create_bound_api_hash, create_middlelayer_api_hash,
                    create_refactor_hash, check_hash, check_hash_simple)


def test_refactor_round_trip():
    tst_hash = create_refactor_hash()
    buffer = encodeBinary(tst_hash)

    hsh = decodeBinary(buffer)
    check_hash(hsh)


def test_schema_round_trip():
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


@skip("Will be moved to integration tests")
def test_bound_api_mixed_round_trip():
    from karabo.bound import BinarySerializerHash

    tst_hash = create_bound_api_hash()
    ser = BinarySerializerHash.create("Bin")

    buffer = ser.save(tst_hash)
    check_hash_simple(decodeBinary(buffer))

    tst_hash = create_refactor_hash()
    buffer = encodeBinary(tst_hash)
    check_hash_simple(ser.load(buffer))


def test_middlelayer_api_bin_round_trip():
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
