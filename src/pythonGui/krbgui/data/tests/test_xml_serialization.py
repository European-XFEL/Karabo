from .utils import (create_bound_api_hash, create_middlelayer_api_hash,
                    create_refactor_hash, check_hash, check_hash_simple)
from ..api import read_xml_hash, write_xml_hash


def test_xml_roundtrip():
    tst_hash = create_refactor_hash()
    xmldata = write_xml_hash(tst_hash)

    read_hash = read_xml_hash(xmldata)
    check_hash(read_hash)


def test_xml_bound_api_mixed_round_trip():
    from karabo.bound import TextSerializerHash

    ser = TextSerializerHash.create("Xml")

    tst_hash = create_bound_api_hash()
    buffer = ser.save(tst_hash)
    check_hash_simple(read_xml_hash(buffer))

    tst_hash = create_refactor_hash()
    buffer = write_xml_hash(tst_hash)
    check_hash_simple(ser.load(buffer))


def test_xml_middlelayer_api_mixed_round_trip():
    from karabo.middlelayer import Hash

    tst_hash = create_middlelayer_api_hash()
    buffer = tst_hash.encode('XML')
    check_hash(read_xml_hash(buffer))

    tst_hash = create_refactor_hash()
    buffer = write_xml_hash(tst_hash)
    check_hash(Hash.decode(buffer, 'XML'))
