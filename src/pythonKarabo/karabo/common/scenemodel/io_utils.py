from collections.abc import Sequence

from .const import NS_KARABO
from .exceptions import SceneWriterException


def get_numbers(names, element):
    """ Read a list of integer values from an `Element` instance.
    """
    return {name: float(element.get(name)) for name in names}


def set_numbers(names, model, element, xmlnames=None):
    """ Copy a list of integer attribute values to an `Element` instance.

    This is the inverse of `get_numbers`.
    """
    EPSILON = 1e-2
    if xmlnames is None:
        xmlnames = names
    elif not isinstance(xmlnames, Sequence) or len(names) != len(xmlnames):
        msg = "XML names must match up with object names"
        raise SceneWriterException(msg)

    def _convert(num):
        if abs(num - int(num)) > EPSILON:
            return str(num)
        return str(int(num))

    for modelname, xmlname in zip(names, xmlnames):
        element.set(xmlname, _convert(getattr(model, modelname)))


def read_base_widget_data(element):
    """ Read the attributes common to all "widget" elements
    """
    traits = get_numbers(('x', 'y', 'width', 'height'), element)
    traits['parent_component'] = element.get(NS_KARABO + 'class')
    keys = element.get(NS_KARABO + 'keys', '')
    if len(keys) > 0:
        traits['keys'] = keys.split(',')
    return traits


def read_empty_display_editable_widget(element):
    """ Read the attributes common to all widgets with Display/Editable forms.
    """
    traits = read_base_widget_data(element)
    traits['klass'] = element.get(NS_KARABO + 'widget')
    return traits


def read_unknown_display_editable_widget(element):
    """ Read the attributes from an unknown data model

    This can be either a controller ``widget`` or a scene ``class`` widget
    """
    traits = read_base_widget_data(element)
    klass = element.get(NS_KARABO + 'widget')
    if klass is None:
        klass = element.get(NS_KARABO + 'class')
    traits['klass'] = klass
    return traits


def write_base_widget_data(model, element, widget_class_name):
    """ Write out the attributes common to all "widget" elements
    """
    if len(model.parent_component) == 0:
        msg = "Widget {} has no parent component!".format(widget_class_name)
        raise SceneWriterException(msg)
    element.set(NS_KARABO + 'class', model.parent_component)
    element.set(NS_KARABO + 'widget', widget_class_name)
    element.set(NS_KARABO + 'keys', ",".join(model.keys))
    set_numbers(('x', 'y', 'width', 'height'), model, element)
