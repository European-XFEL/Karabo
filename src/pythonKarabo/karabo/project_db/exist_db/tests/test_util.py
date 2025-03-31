import pytest
from lxml import etree

from karabo.project_db.util import make_str_if_needed, make_xml_if_needed


def test_make_xml_if_needed():
    xml_rep = "<test>foo</test>"
    ret = make_xml_if_needed(xml_rep)
    assert ret.tag == "test"
    assert ret.text == "foo"


def test_malformed_xml_inputs():
    xml_rep = "<test>foo</test><bad|symbols></bad|symbols>"
    with pytest.raises(ValueError):
        make_xml_if_needed(xml_rep)


def test_make_str_if_needed():
    element = etree.Element('test')
    element.text = 'foo'
    str_rep = make_str_if_needed(element)
    assert str_rep == "<test>foo</test>\n"
