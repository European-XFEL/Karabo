from xml.etree.ElementTree import SubElement

from traits.api import Dict, Enum, Float, Int, String

from karabo.common.scenemodel.bases import BaseWidgetObjectData
from karabo.common.scenemodel.const import NS_KARABO, NS_SVG
from karabo.common.scenemodel.io_utils import (
    read_base_widget_data, read_empty_display_editable_widget,
    write_base_widget_data)
from karabo.common.scenemodel.registry import (
    register_scene_reader, register_scene_writer)


class DisplayStateColorModel(BaseWidgetObjectData):
    """ A model for DisplayStateColor
    """
    # The text shown on the widget
    text = String
    # The possible colors for the states
    colors = Dict


class EvaluatorModel(BaseWidgetObjectData):
    """ A model for Evaluator
    """
    # The expression which is evaluated
    expression = String


class FloatSpinBoxModel(BaseWidgetObjectData):
    """ A model for FloatSpinBox
    """
    # The step size
    step = Float


class MonitorModel(BaseWidgetObjectData):
    """ A model for Monitor
    """
    # A file path
    filename = String
    # An interval
    interval = Float


class SingleBitModel(BaseWidgetObjectData):
    """ A model for SingleBit
    """
    # Which bit is being edited
    bit = Int


class TableElementModel(BaseWidgetObjectData):
    """ A model for TableElement
    """
    # The schema which defines the table
    # XXX: This is stored as text for now...
    column_schema = String
    # The actual type of the widget
    klass = Enum('DisplayTableElement', 'EditableTableElement')


@register_scene_reader('DisplayStateColor', version=1)
def _display_state_color_reader(read_func, element):
    traits = read_base_widget_data(element)
    traits['text'] = element.get(NS_KARABO + 'staticText', '')
    colors = {}
    for sub in element:
        if sub.tag != (NS_KARABO + 'sc'):
            continue
        red = int(sub.get('red'))
        green = int(sub.get('green'))
        blue = int(sub.get('blue'))
        alpha = int(sub.get('alpha'))
        colors[sub.text] = (red, green, blue, alpha)
    traits['colors'] = colors
    return DisplayStateColorModel(**traits)


@register_scene_writer(DisplayStateColorModel)
def _display_state_color_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    write_base_widget_data(model, element, 'DisplayStateColor')
    element.set(NS_KARABO + 'staticText', model.text)
    for name, color in model.colors.items():
        sub = SubElement(element, NS_KARABO + "sc")
        sub.text = name
        red, green, blue, alpha = color
        sub.set('red', str(red))
        sub.set('green', str(green))
        sub.set('blue', str(blue))
        sub.set('alpha', str(alpha))
    return element


@register_scene_reader('Evaluator', version=1)
def _evaluator_reader(read_func, element):
    traits = read_base_widget_data(element)
    traits['expression'] = element.get('expression')
    return EvaluatorModel(**traits)


@register_scene_writer(EvaluatorModel)
def _evaluator_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    write_base_widget_data(model, element, 'Evaluator')
    element.set('expression', model.expression)
    return element


@register_scene_reader('FloatSpinBox', version=1)
def _float_spin_box_reader(read_func, element):
    traits = read_base_widget_data(element)
    traits['step'] = float(element.get(NS_KARABO + 'step', 1.0))
    return FloatSpinBoxModel(**traits)


@register_scene_writer(FloatSpinBoxModel)
def _float_spin_box_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    write_base_widget_data(model, element, 'FloatSpinBox')
    element.set(NS_KARABO + 'step', str(model.step))
    return element


@register_scene_reader('Monitor', version=1)
def _monitor_reader(read_func, element):
    traits = read_base_widget_data(element)
    traits['filename'] = element.get('filename', '')
    traits['interval'] = float(element.get('interval'))
    return MonitorModel(**traits)


@register_scene_writer(MonitorModel)
def _monitor_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    write_base_widget_data(model, element, 'Monitor')
    element.set('interval', str(model.interval))
    if len(model.filename) > 0:
        element.set('filename', model.filename)
    return element


@register_scene_reader('SingleBit', version=1)
def _single_bit_reader(read_func, element):
    traits = read_base_widget_data(element)
    traits['bit'] = int(element.get(NS_KARABO + 'bit', 0))
    return SingleBitModel(**traits)


@register_scene_writer(SingleBitModel)
def _single_bit_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    write_base_widget_data(model, element, 'SingleBit')
    element.set(NS_KARABO + 'bit', str(model.bit))
    return element


@register_scene_reader('DisplayTableElement', version=1)
@register_scene_reader('EditableTableElement', version=1)
def _table_element_reader(read_func, element):
    traits = read_empty_display_editable_widget(element)
    traits['column_schema'] = element.get(NS_KARABO + 'columnSchema', '')
    return TableElementModel(**traits)


@register_scene_writer(TableElementModel)
def _table_element_writer(write_func, model, parent):
    element = SubElement(parent, NS_SVG + 'rect')
    write_base_widget_data(model, element, model.klass)
    element.set(NS_KARABO + 'columnSchema', model.column_schema)
    return element
