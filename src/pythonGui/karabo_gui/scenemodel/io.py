from xml.etree.ElementTree import Element, parse, tostring

from .const import NS_KARABO
from .registry import get_reader, get_writer


def read_scene(filename):
    """ Read a scene from the file name `filename` and return it.
    """
    tree = parse(filename)
    root = tree.getroot()

    version = int(root.get(NS_KARABO + 'version', '1'))
    reader = get_reader(version)
    return reader(root)


def write_scene(scene):
    """ Write Scene object `scene` to a string.
    """
    root = Element('svg')
    root.set(NS_KARABO + 'version', '1')

    writer = get_writer()
    writer(scene, root)
    return tostring(root)
