from ..api import read_binary_hash, write_binary_hash
from .utils import (create_api1_hash, create_api2_hash, create_refactor_hash,
                    check_hash, check_hash_simple)


def test_refactor_round_trip():
    tst_hash = create_refactor_hash()
    buffer = write_binary_hash(tst_hash)

    hsh = read_binary_hash(buffer)
    check_hash(hsh)


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
