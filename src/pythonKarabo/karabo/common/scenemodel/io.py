from xml.etree.ElementTree import Element, parse, tostring

from .const import NS_KARABO, SCENE_FILE_VERSION
from .model import SceneModel
from .registry import get_reader, get_writer


def read_scene(filename_or_fileobj):
    """ Read a scene and return it.
    filename_or_fileobj is either a string containing a filename, or a
    file-like object which can be read from (eg- a TextIO instance).
    If ``filename_or_fileobj`` is None, an empty MacroModel is returned.
    """
    if filename_or_fileobj is None:
        return SceneModel()

    tree = parse(filename_or_fileobj)
    root = tree.getroot()

    # Old files have no version, so '1' is the default.
    # The version number decides which readers are used for the file
    version = int(root.get(NS_KARABO + 'version', '1'))
    reader = get_reader(version)
    return reader(root)


def write_scene(scene):
    """ Write Scene object `scene` to a string.
    """
    root = Element('svg')
    # We always WRITE the most recent version.
    root.set(NS_KARABO + 'version', str(SCENE_FILE_VERSION))
    return _writer_core(scene, root)


def write_single_model(model):
    """ Write a scene model object as an SVG containing only that object.
    """
    root = Element('svg')
    return _writer_core(model, root)


def _writer_core(model, root):
    writer = get_writer()
    writer(model, root)
    return tostring(root, encoding='unicode')
