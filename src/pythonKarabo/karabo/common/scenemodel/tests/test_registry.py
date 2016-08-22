from xml.etree.ElementTree import fromstring

from ..registry import get_reader, register_scene_reader

XML_DATA = "<xml><testy /></xml>"


@register_scene_reader('Root', xmltag='xml', version=1)
def _root_reader(read_func, element):
    ret = None
    for child in element:
        ret = read_func(child)
    return ret


@register_scene_reader('Test', xmltag='testy', version=3)
def _version_three_testy_reader(read_func, element):
    return 3


@register_scene_reader('Test', xmltag='testy', version=1)
def _version_one_testy_reader(read_func, element):
    return 1


@register_scene_reader('Test', xmltag='testy', version=2)
def _version_two_testy_reader(read_func, element):
    return 2


@register_scene_reader('First', xmltag='first', version=1)
@register_scene_reader('Second', xmltag='second', version=1)
def _doubly_registered_reader(read_func, element):
    return element.get('type')


def test_get_reader_simple():
    root = fromstring(XML_DATA)

    for version in (1, 2, 3):
        reader = get_reader(version)
        assert reader(root) == version


def test_get_reader_version_edges():
    root = fromstring(XML_DATA)

    # 'testy' has no version 4 reader... make sure we get version 3
    reader = get_reader(4)
    assert reader(root) == 3


def test_multiple_decorators():
    reader = get_reader(1)

    root = fromstring("""<second type="Second"/>""")
    assert reader(root) == 'Second'

    root = fromstring("""<first type="First"/>""")
    assert reader(root) == 'First'
