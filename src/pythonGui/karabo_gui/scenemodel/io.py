from xml.etree.ElementTree import Element, parse, tostring

from .const import NS_KARABO
from .registry import get_reader, get_writer


def read_scene(filename_or_fileobj):
    """ Read a scene and return it.
    filename_or_fileobj is either a string containing a filename, or a
    file-like object which can be read from (eg- a TextIO instance).
    """
    tree = parse(filename_or_fileobj)
    root = tree.getroot()

    version = int(root.get(NS_KARABO + 'version', '1'))
    reader = get_reader(version)
    return reader(root)


def write_scene(scene):
    """ Write Scene object `scene` to a string.
    """
    root = Element('svg')
    root.set(NS_KARABO + 'version', '1')
    return _writer_core(scene, root)


def write_single_model(model):
    """ Write a scene model object as an SVG containing only that object.
    """
    root = Element('svg')
    return _writer_core(model, root)


def _writer_core(model, root):
    writer = get_writer()
    writer(model, root)
    return tostring(root)
