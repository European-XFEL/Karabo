from unittest import skip
from .utils import (create_bound_api_hash, create_middlelayer_api_hash,
                    create_refactor_hash, check_hash, check_hash_simple)
from ..xml_reader import decodeXML
from ..xml_writer import encodeXML


def test_xml_roundtrip():
    tst_hash = create_refactor_hash()
    xmldata = encodeXML(tst_hash)
    read_hash = decodeXML(xmldata)
    check_hash(read_hash)


@skip("Will be moved to integration tests")
def test_xml_bound_api_mixed_round_trip():
    from karabo.bound import TextSerializerHash

    ser = TextSerializerHash.create("Xml")

    tst_hash = create_bound_api_hash()
    buffer = ser.save(tst_hash)
    check_hash_simple(decodeXML(buffer))

    tst_hash = create_refactor_hash()
    buffer = encodeXML(tst_hash)
    check_hash_simple(ser.load(buffer))


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
