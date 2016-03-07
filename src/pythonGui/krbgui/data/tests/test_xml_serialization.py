from .utils import (create_api1_hash, create_api2_hash, create_refactor_hash,
                    check_hash, check_hash_simple)
from ..xml_reader import read_xml_hash
from ..xml_writer import write_xml_hash


def test_xml_roundtrip():
    tst_hash = create_refactor_hash()
    xmldata = write_xml_hash(tst_hash)

    read_hash = read_xml_hash(xmldata)
    check_hash(read_hash)


def test_xml_api1_mixed_round_trip():
    from karabo.api_1 import TextSerializerHash

    ser = TextSerializerHash.create("Xml")

    tst_hash = create_api1_hash()
    buffer = ser.save(tst_hash)
    check_hash_simple(read_xml_hash(buffer))

    tst_hash = create_refactor_hash()
    buffer = write_xml_hash(tst_hash)
    check_hash_simple(ser.load(buffer))


def test_xml_api2_mixed_round_trip():
    from karabo.api_2 import Hash

    tst_hash = create_api2_hash()
    buffer = tst_hash.encode('XML')
    check_hash(read_xml_hash(buffer))

    tst_hash = create_refactor_hash()
    buffer = write_xml_hash(tst_hash)
    check_hash(Hash.decode(buffer, 'XML'))
