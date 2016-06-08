from xml.etree.ElementTree import SubElement

from traits.api import Enum, Int, String

from .bases import BaseWidgetObjectData
from .const import NS_KARABO, NS_SVG
from .io_utils import get_numbers, set_numbers
from .registry import register_scene_reader, register_scene_writer


class LabelModel(BaseWidgetObjectData):
    """ A fragment of text which is shown in a scene.
    """
    # The text to be displayed
    text = String
    # A string describing the font
    font = String
    # A foreground color, CSS-style
    foreground = String
    # A background color, CSS-style
    background = String
    # The line width of a frame around the text
    frame_width = Int(0)


class SceneLinkModel(BaseWidgetObjectData):
    """ A model for a scene link
    """
    # What scene is being linked to?
    target = String


class WorkflowItemModel(BaseWidgetObjectData):
    """ A model for a WorkflowItem
    """
    # The device id for this item
    device_id = String
    # A string describing the font for this item
    font = String
    # What time of item is this?
    klass = Enum('WorkflowItem', 'WorkflowGroupItem')


def _read_base_widget_data(element):
    """ Read the attributes common to all "widget" elements
    """
    return get_numbers(('x', 'y', 'width', 'height'), element)


def _write_base_widget_data(model, element, widget_class_name):
    """ Write out the attributes common to all "widget" elements
    """
    element.set(NS_KARABO + 'class', widget_class_name)
    set_numbers(('x', 'y', 'width', 'height'), model, element)


@register_scene_reader('Label', version=1)
def __label_reader(read_func, element):
    """ A reader for Label objects in Version 1 scenes
    """
    traits = _read_base_widget_data(element)
    traits['text'] = element.get(NS_KARABO + 'text')
    traits['font'] = element.get(NS_KARABO + 'font')
    traits['foreground'] = element.get(NS_KARABO + 'foreground', 'black')

    bg = element.get(NS_KARABO + 'background')
    if bg is not None:
        traits['background'] = bg
    fw = element.get(NS_KARABO + 'frameWidth')
    if fw is not None:
        traits['frame_width'] = int(fw)
    return LabelModel(**traits)


@register_scene_writer(LabelModel)
def __label_writer(write_func, model, parent):
    """ A writer for LabelModel objects
    """
    element = SubElement(parent, NS_SVG + 'rect')
    _write_base_widget_data(model, element, 'Label')

    for name in ('text', 'font', 'foreground'):
        element.set(NS_KARABO + name, getattr(model, name))

    element.set(NS_KARABO + 'frameWidth', str(model.frame_width))
    if model.background != '':
        element.set(NS_KARABO + 'background', model.background)

    return element


@register_scene_reader('SceneLink', version=1)
def __scene_link_reader(read_func, element):
    traits = _read_base_widget_data(element)
    traits['target'] = element.get(NS_KARABO + 'target')
    return SceneLinkModel(**traits)


@register_scene_writer(SceneLinkModel)
def __scene_link_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    _write_base_widget_data(model, element, 'SceneLink')
    element.set(NS_KARABO + 'target', model.target)
    return element


@register_scene_reader('WorkflowItem', version=1)
@register_scene_reader('WorkflowGroupItem', version=1)
def __workflow_item_reader(read_func, element):
    traits = _read_base_widget_data(element)
    traits['klass'] = element.get(NS_KARABO + 'class')
    traits['device_id'] = element.get(NS_KARABO + 'text')
    traits['font'] = element.get(NS_KARABO + 'font')
    return WorkflowItemModel(**traits)


@register_scene_writer(WorkflowItemModel)
def __workflow_item_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    _write_base_widget_data(model, element, model.klass)
    element.set(NS_KARABO + 'text', model.device_id)
    element.set(NS_KARABO + 'font', model.font)
    return element
