from contextlib import contextmanager
import os
from tempfile import mkstemp
from xml.etree.ElementTree import fromstring

from ..model import SceneModel
from ..io import read_scene, write_scene


def assert_base_traits(model):
    assert model.x == 0
    assert model.y == 0
    assert model.width == 100
    assert model.height == 100
    assert model.keys == ['device_id.prop']


def base_widget_traits(parent=None):
    traits = {
        'keys': ['device_id.prop'],
        'x': 0, 'y': 0, 'height': 100, 'width': 100
    }
    if parent is not None:
        traits['parent_component'] = parent
    return traits


def single_model_from_data(svg_data):
    """ Given some SVG data, read a scene model from it an return the first
    child.
    """
    with temp_file(svg_data) as fn:
        return read_scene(fn).children[0]


def single_model_round_trip(model):
    """ Given a scene model object, write it to XML and read it back to examine
    the round trip reader/writer behavior.
    """
    scene = SceneModel(children=[model])
    xml = write_scene(scene)
    with temp_file(xml.decode('utf-8')) as fn:
        rt_scene = read_scene(fn)
    return rt_scene.children[0]


@contextmanager
def temp_cwd(path):
    """ Change the current working directory temporarily using a context
    manager.
    """
    orig_path = os.getcwd()
    try:
        os.chdir(path)
        yield
    finally:
        os.chdir(orig_path)


@contextmanager
def temp_file(contents):
    """ Create a temporary file in a context manager. Returns the path of the
    file.
    """
    fd, filename = mkstemp()
    try:
        with open(filename, 'w') as fp:
            fp.write(contents)
        yield filename
    finally:
        os.close(fd)
        os.unlink(filename)


def xml_is_equal(xmlstr0, xmlstr1):
    """ Compare two chunks of XML
    """
    root0 = fromstring(xmlstr0)
    root1 = fromstring(xmlstr1)

    def compare_attrs(el0, el1):
        for k, v in el0.items():
            attr = el1.get(k)
            if attr is None or v != attr:
                return False  # pragma: no cover

        return True

    def compare_tree(elem0, elem1):
        for child0, child1 in zip(list(elem0), list(elem1)):
            if child0.tag != child1.tag:
                return False  # pragma: no cover
            if not compare_attrs(child0, child1):
                return False  # pragma: no cover
            if not compare_tree(child0, child1):
                return False  # pragma: no cover
        return True

    return compare_attrs(root0, root1) and compare_tree(root0, root1)
