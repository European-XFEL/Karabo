from xml.etree.ElementTree import SubElement

from traits.api import Dict, Float, Instance, Int, List, String

from karabo.common.project.bases import BaseProjectObjectModel
from .bases import BaseSceneObjectData
from .const import (NS_KARABO, NS_SVG, SCENE_MIN_WIDTH, SCENE_MIN_HEIGHT,
                    SCENE_FILE_VERSION)
from .io_utils import set_numbers
from .registry import register_scene_reader, register_scene_writer


class SceneModel(BaseProjectObjectModel):
    """ An object representing the data for a Karabo GUI scene.
    """
    # The version of the file data (from the scene file)
    file_format_version = Int(SCENE_FILE_VERSION, transient=True)
    # The title of the scene XXX: Remove when the old project is dead!
    title = String(transient=True)
    # Extra attributes from the SVG file that we want to preserve.
    extra_attributes = Dict(transient=True)
    # The width of the scene in pixels
    width = Float(SCENE_MIN_WIDTH)
    # The height of the scene in pixels
    height = Float(SCENE_MIN_HEIGHT)
    # All the objects in the scene
    children = List(Instance(BaseSceneObjectData))


class UnknownXMLDataModel(BaseSceneObjectData):
    """ A model object to hold SVG data that we don't understand.
    """
    # The xml tag
    tag = String
    # The element attributes
    attributes = Dict
    # The element data
    data = String
    # The element's children
    children = List(Instance(BaseSceneObjectData))


def _read_extra_attributes(element):
    """ Read all the attributes that we don't explicitly write.
    """
    our_names = ('height', 'width', NS_KARABO + 'version')
    attributes = {}
    for name, value in element.items():
        if name not in our_names:
            attributes[name] = value

    return attributes


@register_scene_reader('Scene', xmltag='svg', version=1)
@register_scene_reader('Scene', xmltag=NS_SVG + 'svg', version=1)
def __scene_reader(read_func, element):
    traits = {
        'file_format_version': int(element.get(NS_KARABO + 'version', 1)),
        'width': float(element.get('width', 0)),
        'height': float(element.get('height', 0)),
        'extra_attributes': _read_extra_attributes(element),
    }
    scene = SceneModel(**traits)
    for child in element:
        scene.children.append(read_func(child))

    scene.initialized = True
    return scene


@register_scene_writer(SceneModel)
def __scene_writer(write_func, scene, root):
    for child in scene.children:
        write_func(child, root)

    set_numbers(('height', 'width'), scene, root)
    for name, value in scene.extra_attributes.items():
        root.set(name, value)

    return root


@register_scene_reader('Unknown', xmltag='*')
def __unknown_xml_data_reader(read_func, element):
    return UnknownXMLDataModel(
        tag=element.tag,
        attributes=element.attrib,
        data=element.text or '',
        children=[read_func(el) for el in element]
    )


@register_scene_writer(UnknownXMLDataModel)
def __unknown_xml_data_writer(write_func, model, root):
    element = SubElement(root, model.tag)
    if model.data:
        element.text = model.data
    for name, value in model.attributes.items():
        element.set(name, value)
    for child in model.children:
        write_func(child, element)

    return element
