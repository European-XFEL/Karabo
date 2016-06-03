from traits.api import HasTraits, Instance, Int, List

from .bases import BaseSceneObjectData
from .const import NS_SVG, SCENE_MIN_WIDTH, SCENE_MIN_HEIGHT
from .registry import register_scene_reader, register_scene_writer


class SceneModel(HasTraits):
    """ An object representing the data for a Karabo GUI scene.
    """
    # The width of the scene in pixels
    width = Int(SCENE_MIN_WIDTH)
    # The height of the scene in pixels
    height = Int(SCENE_MIN_HEIGHT)
    # All the objects in the scene
    children = List(Instance(BaseSceneObjectData))


@register_scene_reader('Scene', xmltag='svg', version=1)
@register_scene_reader('Scene', xmltag=NS_SVG + 'svg', version=1)
def __scene_reader(read_func, element):

    width = int(element.get('width', 0))
    height = int(element.get('height', 0))

    scene = SceneModel(width=width, height=height)
    for child in element:
        scene.children.append(read_func(child))

    return scene


@register_scene_writer(SceneModel)
def __scene_writer(write_func, scene, root):
    for child in scene.children:
        write_func(child, root)

    root.set('width', str(scene.width))
    root.set('height', str(scene.height))

    return root
