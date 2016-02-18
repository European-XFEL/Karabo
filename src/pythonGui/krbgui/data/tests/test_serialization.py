from contextlib import closing, contextmanager
from io import BytesIO

from ..bin_reader import read_hash
from ..bin_writer import write_hash
from .utils import (create_api1_hash, create_api2_hash, create_refactor_hash,
                    check_hash, check_hash_simple)


@contextmanager
def bytes_io_context(buffer=None):
    b_io = BytesIO(buffer)
    with closing(b_io) as fp:
        yield fp


def test_refactor_round_trip():
    tst_hash = create_refactor_hash()

    with bytes_io_context() as fp:
        write_hash(fp, tst_hash)

        fp.seek(0)
        hsh = read_hash(fp)

    check_hash(hsh)


def test_api1_mixed_round_trip():
    from karabo.api_1 import BinarySerializerHash

    tst_hash = create_api1_hash()
    ser = BinarySerializerHash.create("Bin")

    buffer = ser.save(tst_hash)
    with bytes_io_context(buffer=buffer) as fp:
        check_hash_simple(read_hash(fp))

    tst_hash = create_refactor_hash()
    with bytes_io_context() as fp:
        write_hash(fp, tst_hash)
        check_hash_simple(ser.load(fp.getvalue()))


def test_api2_mixed_round_trip():
    from karabo.api_2 import BinaryWriter, BinaryParser

    tst_hash = create_api2_hash()

    w = BinaryWriter()
    buffer = w.write(tst_hash)
    with bytes_io_context(buffer) as fp:
        check_hash(read_hash(fp))

    tst_hash = create_refactor_hash()
    with bytes_io_context() as fp:
        write_hash(fp, tst_hash)
        r = BinaryParser()
        check_hash(r.read(fp.getvalue()))
